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
	char str_hash[500];
	char mensagem[1000];	///@FIXME: assume que dados cabem em 1000 bytes
	json_parser json;
	json_pair pairs[200];
	
	pthread_setspecific(dados_thread, tmp);
	pthread_cleanup_push((void *)th_limpeza, tmp);
	printf("Thread criada, fd = %d, n = %d\n", tsd->fd_con, tsd->n_thread);
	
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
	
	json.start = resposta;
	json.pairs = pairs;
	json.n_pairs = 200;
	
	recebe_dados(&l, &json);
	
	if ((hash = json_get_str(&json, "hash")) == NULL) {
		printf("Chave \"hash\" não encontrada\n");
		finaliza("{\"msg\":\"JSON: chave \\\"hash\\\" não encontrada\"}");
	}
	
	#ifndef NAO_CHECA_SENHA
	if (strlen(hash) != 64) {
		printf("Hash != 64 bytes!\n");
		finaliza("{\"msg\":\"JSON: chave \\\"hash\\\" inválida\"}");
	}
	#endif
	
	usuario = json_get_str(&json, "usuario");
	
	if (usuario == NULL) {
		printf("Chave \"usuario\" não encontrada\n");
		finaliza("{\"msg\":\"JSON: chave \\\"usuario\\\" não encontrada\"}");
	}
	
	if (json_get_null(&json, "cadastro") != JSON_INVALID) {	// Existe o par cadastro
		int cod = abs((int) random()), entrada_usuario;
		printf("Novo usuário, criando cadastro\nCódigo: %d\n", cod);
		#ifndef NAO_ENVIA_EMAIL
		if (envia_email(usuario, cod) == -1)
			finaliza("{\"msg\":\"Falha no envio de e-mail de confirmação\"}");
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
				finaliza("{\"msg\":\"JSON: chave \\\"codigo\\\" não encontrada\"}");
			
			if (entrada_usuario != cod)
				envia_fixo("{\"ok\":false}");
			
		} while (entrada_usuario != cod);
		
		add_user(usuario_salvo, hash_salvo);
		
	} else {
		#ifndef NAO_CHECA_SENHA
		for (;;) {
			const char *hash_senha = get_user(usuario);
			if (hash_senha == NULL) {
				finaliza("{\"msg\":\"Usuário não cadastrado\"}");
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
	comparador_t comparador;
	comparador.id = tsd->n_thread;
	
	for (i = 0; i < MAX_PARES; i++)
		tsd->pares[i] = -1;
	
	if (pontos != NULL) {
		
		int prox_ponto, posicao;
		char *placa_recebida, placa[9] = {0};
		int lugares = json_get_int(&json, "lugares");
		if (lugares == JSON_INVALID) {
			printf("Chave \"lugares\" não encontrada, assumindo 1\n");
			lugares = 1;
		} else
			printf("%d lugares\n", lugares);
		adiciona_fila(tsd->n_thread, FILA_DA_CARONA);
		placa_recebida = json_get_str(&json, "placa");
		if (placa_recebida == NULL)
			finaliza("{\"msg\":\"Chave \\\"placa\\\" não encontrada\", \"fim\":null}");
		
		strncpy(placa, placa_recebida, 8);
		
		tsd->placa = placa;
		tsd->pos_atual = 0;
		
		printf("Placa %s\n", tsd->placa);
		
		for (i = 0; i < (int) sizeof(caminhos[0]); i++) {
			prox_ponto = json_array_i(pontos, i);
			printf("Ponto %d\n", prox_ponto);
			caminhos[tsd->n_thread][i] = prox_ponto;
			if (prox_ponto == JSON_INVALID)
				break;
			posicoes_livres[tsd->n_thread][i] = lugares;
		}
		
		if (prox_ponto != -1)	// ainda não achamos o fim
			finaliza("{\"msg\":\"Muitos pontos!\", \"fim\":null}");
		
		envia_fixo("{\"ok\":true}");
		
		// busca compatíveis:
		compara_da_carona(&comparador);
		
		if (comparador.melhor != -1) {
			// pareia:
			tsd->pares[0] = comparador.melhor;
			tsd_array[comparador.melhor].pares[0] = tsd->n_thread;
			muda_tipo(comparador.melhor, FILA_RECEBE_CARONA_PAREADO);
			//muda_tipo(tsd->n_thread, FILA_DA_CARONA_PAREADO);
			
			// envia mensagens:
			sprintf(mensagem, "{\"parar\":%d}", comparador.parar);
			envia_str(mensagem);
			
			sprintf(mensagem, "{\"placa\":\"%s\"}", tsd->placa);	///@FIXME: alguns caracteres quebram o pacote (", \...)
			if (envia_str_outro(comparador.melhor, mensagem)) {	// falha no envio
				pthread_kill(threads[comparador.melhor], 3);	// remove cliente defeituoso
				finaliza("{\"msg\":\"Falha de comunicação com quem recebe carona\"}");
			}
		}
		
		for (;;) {
			recebe_dados(&l, &json);
			if ((posicao = json_get_int(&json, "proximo")) == JSON_INVALID)
				finaliza("{\"msg\":\"Chave \\\"proximo\\\" não encontrada\", \"fim\":null}");
			// checa se índice é válido:
			if (posicao >= i)
				finaliza("{\"msg\":\"Chave \\\"proximo\\\" inválida\", \"fim\":null}");
				
			if (posicao > tsd->pos_atual)		// apenas avança
				tsd->pos_atual = posicao;
			
			// avisa par:
			for (i = 0; i < MAX_PARES; i++)
				if (tsd->pares[i] != -1)
					if (tsd->pos_atual == tsd_array[tsd->pares[i]].pos_atual) {
						printf("Próximo de %d!\n", tsd->pares[i]);
						caronas_total++;
						envia_fixo_outro(tsd->pares[i], "{\"chegando\":null}");
					}
		}
		
		sprintf(mensagem, "{\"msg\":\"Thread %d receberá carona!\"}", comm[tsd->n_thread]);
		finaliza(mensagem);
		
		
	} else {
		if ((tsd->inicio = json_get_int(&json, "inicio")) == JSON_INVALID)
			finaliza("{\"msg\":\"Mensagem faltando ponto inicial\"}");
		if ((tsd->fim = json_get_int(&json, "fim")) == JSON_INVALID)
			finaliza("{\"msg\":\"Mensagem faltando ponto final\"}");
		
		envia_fixo("{\"ok\":true}");
		
		printf("Cruzando usuários...\n");
		
		adiciona_fila(tsd->n_thread, FILA_RECEBE_CARONA);
		
		compara_recebe_carona(&comparador);
		
		if (comparador.melhor != -1) {
			printf("Melhor carro: %d\n", comparador.melhor);
			tsd->pares[0] = comparador.melhor;
			tsd_array[comparador.melhor].pares[par_vazio(comparador.melhor)] = tsd->n_thread;
			tsd->pos_atual = comparador.parar;	// índice da posição de quem recebe carona
			//muda_tipo(comparador.melhor, FILA_DA_CARONA_PAREADO);
			muda_tipo(tsd->n_thread, FILA_RECEBE_CARONA_PAREADO);
			sprintf(mensagem, "{\"parar\":%d}", comparador.parar);
			if (envia_str_outro(comparador.melhor, mensagem)) {	// falha no envio
				pthread_kill(threads[comparador.melhor], 3);	// remove cliente defeituoso
				finaliza("{\"msg\":\"Falha de comunicação com quem dá carona\"}");
			}
			
			sprintf(mensagem, "{\"placa\":\"%s\"}", tsd_array[comparador.melhor].placa);	///@FIXME: alguns caracteres quebram o pacote (", \...)
			envia_str(mensagem);
		} else {
			printf("Ninguém compatível\n");
		}
		
		for (;;) {
			recebe_dados(&l, &json);	// apenas para checar se a conexão continua viva
			///@TODO: mandar mensagem a quem dá carona se a conexão for perdida (cancelar carona)
		}
	}
	
	pthread_cleanup_pop(1);
	return NULL;
}
