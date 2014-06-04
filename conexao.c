/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Tratamento de conexões e threads, funções principais
*******************************************************************************/

#include "conexao.h"
#include "json.h"
#include "conexao_helper.h"

///@TODO: arrumar isso, tem muita coisa alocada junto
int s, clientes_agora = 0, clientes_total = 0, caronas_total = 0;
int caminhos[MAX_CLIENTES][30], esperando_dar_carona[MAX_CLIENTES] = {0}, pos_atual[MAX_CLIENTES] = {0};
uint32_t conectados[MAX_CLIENTES] = {0};	// lista de IPs já conectados
pthread_mutex_t mutex_modifica_thread = PTHREAD_MUTEX_INITIALIZER,
				mutex_recebe_carona[MAX_CLIENTES] = {PTHREAD_MUTEX_INITIALIZER};
int comm[MAX_CLIENTES], fd[MAX_CLIENTES];

pthread_key_t dados_thread;
int pilha_threads_livres[MAX_CLIENTES];
char *usuario_da_carona, *placa[MAX_CLIENTES];


void* th_conecao_cliente(void *tmp) {
	tsd_t *tsd = tmp;	// o argumento é um ponteiro para qqr área definida pelo programa,
								// então precisamos marcar o tipo de ponteiro recebido ou usar casts
	leitura_t l;
	pthread_setspecific(dados_thread, tmp);
  fd[tsd->n_thread] = tsd->fd_con;
	pthread_cleanup_push((void *)th_limpeza, tmp);
	printf("Thread criada, fd = %d, n = %d\n", tsd->fd_con, tsd->n_thread);
	char str_hash[500];
	char mensagem[1000];	///@FIXME: assume que dados cabem em 1000 bytes
	
	pthread_mutex_lock(&processando);
	sprintf(str_hash, "Carona Comunitária USP\n%s\n%d clientes já conectados, %d atualmente, %d caronas dadas",
				MSG_NOVIDADES, clientes_total, clientes_agora, caronas_total);
	
	sprintf(mensagem, "{\"login\":\"%s\"}""", str_hash);
	envia(mensagem, strlen(mensagem) + 1);
	printf("Mensagem:\n%s\n", mensagem);
  
	char resposta[2000];
	const char *hash, *usuario;
	
	///@FIXME: aceita mensagens até 1024 bytes, senão as corta
	l.area = resposta;
	l.fim_pacote = resposta - 1;
	l.fim_msg = resposta - 1;
	l.tamanho_area = sizeof(resposta);
	
	json_parser json;
	json_pair pairs[200];
	json.start = resposta;
	json.pairs = pairs;
	json.n_pairs = 200;
	
  recebe_dados(&l, &json);
	
	if ((hash = json_get_str(&json, "hash")) == NULL) {
		printf("Chave \"hash\" não encontrada\n");
		pthread_exit(NULL);
	}
	
	#ifndef NAO_CHECA_SENHA
	if (strlen(hash) != 64) {
		printf("Hash != 64 bytes!\n");
		pthread_exit(NULL);
	}
	#endif
	
	usuario = json_get_str(&json, "usuario");
	
	if (usuario == NULL) {
		printf("Chave \"usuario\" não encontrada\n");
		pthread_exit(NULL);
	}
	
	if (json_get_null(&json, "cadastro") != JSON_INVALID) {	// Existe o par cadastro
		int cod = abs((int) random()), entrada_usuario;
		printf("Novo usuário, criando cadastro\nCódigo: %d\n", cod);
		#ifndef NAO_ENVIA_EMAIL
		if (envia_email(usuario, cod) == -1)
			finaliza("{\"msg\":\"Falha no envio de e-mail de confirmação\",\"fim\":null}");
		#endif
		envia_fixo("{\"ok\":true}");
		
		char usuario_salvo[250], hash_salvo[33];
		strncpy(usuario_salvo, usuario, sizeof(usuario_salvo));
		usuario_salvo[sizeof(usuario_salvo) - 1] = 0;
		strncpy(hash_salvo, hash, sizeof(hash_salvo));
		hash_salvo[sizeof(hash_salvo) - 1] = 0;
		
		do {
      recebe_dados(&l, &json);
			
			entrada_usuario = json_get_int(&json, "codigo");
			
			if (entrada_usuario == JSON_INVALID)
				finaliza("{\"msg\":\"JSON: chave \"codigo\" não encontrada\",\"fim\":null}");
			
			if (entrada_usuario != cod)
				envia_fixo("{\"ok\":false}");
			
		} while (entrada_usuario != cod);
		
		add_user(usuario_salvo, hash_salvo);
		
	} else {
		for (;;) {
      const char *hash_senha = get_user(usuario);
      if (hash_senha == NULL) {
        finaliza("{\"msg\":\"Usuário não cadastrado\",\"fim\":null}");
      }
      
      
      #ifndef NAO_CHECA_SENHA
      if (senha_correta(hash_senha, str_hash, hash) == 0)
        envia_fixo("{\"ok\":false}");
      else
      #endif
        break;
    }
	}
	
	envia_fixo("{\"ok\":true}");	// autenticação OK
	
  recebe_dados(&l, &json);
	
	const char *pontos = json_get_array(&json, "pontos");
  pthread_mutex_lock(&processando);
	
	int i, j, k;
	
	if (pontos != NULL) {
		
		int prox_ponto;
    placa[tsd->n_thread] = json_get_str(&json, "placa");
    pos_atual[tsd->n_thread] = 0;
		for (i = 0; i < (int) sizeof(caminhos[0]); i++) {
			if ((prox_ponto = json_array_i(pontos, i)) == JSON_INVALID)
				break;
			printf("Ponto %d\n", prox_ponto);
			caminhos[tsd->n_thread][i] = prox_ponto;
		}
		
    esperando_dar_carona[tsd->n_thread] = 1;    ///@TODO: # assentos livres, pode ser > 1 por carro
    
    for (;;) {
      recebe_dados(&l, &json);
      if ((pos_atual[tsd->n_thread] = json_get_int(&json, "proximo")) == JSON_INVALID) {
        finaliza("{\"msg\":\"Chave \"proximo\" não encontrada\", \"fim\":null}");
      }
    }
    
		sprintf(mensagem, "{\"msg\":\"Thread %d receberá carona!\"}", comm[tsd->n_thread]);
		finaliza(mensagem);
		
		
	} else {
		int inicio, fim;
		if ((inicio = json_get_int(&json, "inicio")) == JSON_INVALID)
			finaliza("{\"msg\":\"Mensagem faltando ponto inicial\",\"fim\":null}");
		if ((fim = json_get_int(&json, "fim")) == JSON_INVALID)
			finaliza("{\"msg\":\"Mensagem faltando ponto final\",\"fim\":null}");
		printf("Cruzando usuários...\n");
		
		adiciona_fila(tsd->n_thread);
		
		for (;;) {
			int compativeis = 0;
			for (i = 0; i < MAX_CLIENTES; i++) {
				if (esperando_dar_carona[i]) {
					for (j = 0; j < (int) sizeof(caminhos[0]); j++) {
						if (caminhos[i][j] == inicio) {
							for (k = j + 1; k < (int) sizeof(caminhos[0]); k++) {
								if (caminhos[i][k] == fim) {
									
									printf("Compatível: %d, distância %d\n", i, k - j);
									compativeis++;
									break;
									/*
									comm[i] = tsd->n_thread;
									pthread_cond_signal(&comunica_thread_da_carona[i]);
									sprintf(mensagem, "{\"msg\":\"Thread %d dará carona!\"}", i);
									envia_str(mensagem);
									*/
								}
							}
						}
					}
        }
			}
			
			if (compativeis) {
				printf("%d compatíveis\n", compativeis);
				remove_fila(tsd->n_thread);
				break;
			}
			
			//pthread_mutex_lock(&mutex_comunicacao_recebe_carona[tsd->n_thread]);
			//pthread_cond_wait(&comunica_thread_recebe_carona[tsd->n_thread], &mutex_comunicacao_recebe_carona[tsd->n_thread]);
			//pthread_mutex_unlock(&mutex_comunicacao_recebe_carona[tsd->n_thread]);
		}
	}
	
	pthread_cleanup_pop(1);
	return NULL;
}
