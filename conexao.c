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


void* th_conecao_cliente(void *tmp) {
	tsd_t *tsd = tmp;	// o argumento é um ponteiro para qqr área definida pelo programa,
						// então precisamos marcar o tipo de ponteiro recebido ou usar casts
	leitura_t l;
	char resposta[2000];
	const char *hash, *usuario;
	
	pthread_setspecific(dados_thread, tmp);
	fd[tsd->n_thread] = tsd->fd_con;
	pthread_cleanup_push((void *)th_limpeza, tmp);
	printf("Thread criada, fd = %d, n = %d\n", tsd->fd_con, tsd->n_thread);
	char str_hash[500];
	char mensagem[1000];	///@FIXME: assume que dados cabem em 1000 bytes
	
	adquire_mutex();
	sprintf(str_hash, "Carona Comunitária USP\n%s\n%d clientes já conectados, %d atualmente, %d caronas dadas",
				MSG_NOVIDADES, clientes_total, clientes_agora, caronas_total);
	
	sprintf(mensagem, "{\"login\":\"%s\"}""", str_hash);
	envia(mensagem, strlen(mensagem) + 1);
	printf("%d - Mensagem:\n%s\n", tsd->n_thread, mensagem);
  
	
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
		finaliza("{\"msg\":\"JSON: chave \\\"hash\\\" não encontrada\",\"fim\":null}");
	}
	
	#ifndef NAO_CHECA_SENHA
	if (strlen(hash) != 64) {
		printf("Hash != 64 bytes!\n");
		finaliza("{\"msg\":\"JSON: chave \\\"hash\\\" inválida\",\"fim\":null}");
	}
	#endif
	
	usuario = json_get_str(&json, "usuario");
	
	if (usuario == NULL) {
		printf("Chave \"usuario\" não encontrada\n");
		finaliza("{\"msg\":\"JSON: chave \\\"usuario\\\" não encontrada\",\"fim\":null}");
	}
	
	if (json_get_null(&json, "cadastro") != JSON_INVALID) {	// Existe o par cadastro
		int cod = abs((int) random()), entrada_usuario;
		printf("Novo usuário, criando cadastro\nCódigo: %d\n", cod);
		#ifndef NAO_ENVIA_EMAIL
		if (envia_email(usuario, cod) == -1)
			finaliza("{\"msg\":\"Falha no envio de e-mail de confirmação\",\"fim\":null}");
		#endif
		envia_fixo("{\"ok\":true}");
		
		char usuario_salvo[250], hash_salvo[65];
		strncpy(usuario_salvo, usuario, sizeof(usuario_salvo));
		usuario_salvo[sizeof(usuario_salvo) - 1] = 0;
		strncpy(hash_salvo, hash, sizeof(hash_salvo));
		hash_salvo[sizeof(hash_salvo) - 1] = 0;
		
		do {
			recebe_dados(&l, &json);
			
			entrada_usuario = json_get_int(&json, "codigo");
			
			if (entrada_usuario == JSON_INVALID)
				finaliza("{\"msg\":\"JSON: chave \\\"codigo\\\" não encontrada\",\"fim\":null}");
			
			if (entrada_usuario != cod)
				envia_fixo("{\"ok\":false}");
			
		} while (entrada_usuario != cod);
		
		add_user(usuario_salvo, hash_salvo);
		
	} else {
		#ifndef NAO_CHECA_SENHA
		for (;;) {
			const char *hash_senha = get_user(usuario);
			if (hash_senha == NULL) {
				finaliza("{\"msg\":\"Usuário não cadastrado\",\"fim\":null}");
			}
			
			if (senha_correta(hash_senha, str_hash, hash))
				break;
			envia_fixo("{\"ok\":false}");
			recebe_dados(&l, &json);		// tentar novamente
		}
		#endif
	}
	
	envia_fixo("{\"ok\":true}");	// autenticação OK
	
	recebe_dados(&l, &json);
	
	const char *pontos = json_get_array(&json, "pontos");
	
	int i;
	if (pontos != NULL) {
		
		int prox_ponto;
		char *placa_recebida;
		adiciona_fila(tsd->n_thread, FILA_DA_CARONA);
		placa_recebida = json_get_str(&json, "placa");
		if (placa_recebida == NULL)
			finaliza("{\"msg\":\"Chave \\\"placa\\\" não encontrada\", \"fim\":null}");
		placa[tsd->n_thread] = placa_recebida;
		pos_atual[tsd->n_thread] = 0;
		for (i = 0; i < (int) sizeof(caminhos[0]); i++) {
			if ((prox_ponto = json_array_i(pontos, i)) == JSON_INVALID)
				break;
			printf("Ponto %d\n", prox_ponto);
			caminhos[tsd->n_thread][i] = prox_ponto;
		}
		
		envia_fixo("{\"ok\":true}");
		
		for (;;) {
			recebe_dados(&l, &json);
			if ((pos_atual[tsd->n_thread] = json_get_int(&json, "proximo")) == JSON_INVALID)
				finaliza("{\"msg\":\"Chave \\\"proximo\\\" não encontrada\", \"fim\":null}");
		}
		
		sprintf(mensagem, "{\"msg\":\"Thread %d receberá carona!\"}", comm[tsd->n_thread]);
		finaliza(mensagem);
		
		
	} else {
		int inicio, fim;
		if ((inicio = json_get_int(&json, "inicio")) == JSON_INVALID)
			finaliza("{\"msg\":\"Mensagem faltando ponto inicial\",\"fim\":null}");
		if ((fim = json_get_int(&json, "fim")) == JSON_INVALID)
			finaliza("{\"msg\":\"Mensagem faltando ponto final\",\"fim\":null}");
		
		envia_fixo("{\"ok\":true}");
		
		printf("Cruzando usuários...\n");
		
		adiciona_fila(tsd->n_thread, FILA_RECEBE_CARONA);
		
		for (;;) {
			int compativeis = 0, id_menor = -1, d_menor = 1000, d_atual, comparar_com = -1;
			
			while ((comparar_com = prox_fila(comparar_com)) != -1) {
				printf("Comparando com %d\n", comparar_com);
				d_atual = distancia(comparar_com, inicio, fim);
				if (d_atual != -1) {  // caminho compatível
					compativeis++;
					if (d_atual < d_menor) {
						d_menor = d_atual;
						id_menor = comparar_com;
					}
				}
			}
			
			if (compativeis) {
				printf("%d compatíveis, id %d é o melhor\n", compativeis, id_menor);
				printf("FAZER ALGO AQUI\n");
				break;
			} else {
				printf("Ninguém compatível\n");
			}
			
			recebe_dados(&l, &json);
		}
	}
	
	pthread_cleanup_pop(1);
	return NULL;
}

