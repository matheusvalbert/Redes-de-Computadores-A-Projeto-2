#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

struct contato {

	char numero[20], ip[20];
	int porta;	
};

struct infocliente {

	struct sockaddr_in client;
	int ns;
};

pthread_mutex_t mutex;
struct contato cont[50];
int contCount = 0;
void INThandler(int);
int s;

void *tratamento(void *informacoes) {
	struct contato contato_temp;
	struct sockaddr_in client;
	struct infocliente info;
	int ns;
	info = *(struct infocliente*) informacoes;
	client = info.client;
	ns = info.ns;
	int opcao;
	int op, numLen, env;
	char num[20];
	char ip[20];
	int porta;
	int i, flag;
	
	while(1) {


		if (recv(ns, &opcao, sizeof(int), 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

		if(opcao == 1){//Conexão de usuario

			pthread_mutex_lock(&mutex);
			flag = 1;
			if (recv(ns, &contato_temp, sizeof(contato_temp), 0) == -1) {
			
				perror("Recv()");
				exit(6);
			}
			for(i = 0; i < contCount; i++) {
				if(strcmp(contato_temp.numero, cont[i].numero) == 0) {
					flag = -1;
					break;
				}
			}
			if (send(ns, &flag, sizeof(int), 0) == -1) {
			
				perror("Send()");
				exit(6);
			}
	
			if(flag == 1) {
				strcpy(cont[contCount].numero, contato_temp.numero);
				strcpy(cont[contCount].ip, contato_temp.ip);
				cont[contCount].porta = contato_temp.porta;
				printf("Numero conectado: %s - IP: %s - Porta: %d\n", cont[contCount].numero, cont[contCount].ip, cont[contCount].porta);
				contCount++;
			}
			pthread_mutex_unlock(&mutex);
			if(flag == -1)
				break;
		}
		else if(opcao == 2) {//Envio de dados sobre um numero
			
			env = -1;
			if (recv(ns, &numLen, sizeof(int), 0) == -1) {
			
				perror("Recv()");
				exit(6);
			}

			if (recv(ns, num, numLen, 0) == -1) {
				
				perror("Recv()");
				exit(6);
			}
			pthread_mutex_lock(&mutex);
			for (i = 0; i < contCount; ++i) {

				if(strcmp(cont[i].numero, num) == 0) {

					env = i;
				}
			}
			if(env != -1) {
				strcpy(ip, cont[env].ip);
				porta = cont[env].porta;
				numLen = strlen(ip);
				
				pthread_mutex_unlock(&mutex);
				if (send(ns, &numLen, sizeof(int), 0) == -1) {
			
					perror("Send()");
					exit(6);
				}			

				if (send(ns, ip, numLen, 0) == -1) {
			
					perror("Send()");
					exit(6);
				}

				if (send(ns, &porta, sizeof(int), 0) == -1) {
			
					perror("Send()");
					exit(6);
				}
				printf("Enviados os dados sobre o numero: %s\n", num);
			}
			if(env == -1) {
				pthread_mutex_unlock(&mutex);
				numLen = -1;
				if (send(ns, &numLen, sizeof(int), 0) == -1) {
			
					perror("Send()");
					exit(6);
				}
			} 
		}
		else if(opcao == 3) {//Remoção do usuário do servidor
			 
			if (recv(ns, &numLen, sizeof(int), 0) == -1) {
			
				perror("Recv()");
				exit(6);
			}

			if (recv(ns, num, numLen, 0) == -1) {
				
				perror("Recv()");
				exit(6);
			}	
			i = 0;
			flag = 0;
			pthread_mutex_lock(&mutex);
			while(i < contCount) {
				
				if(flag == 0 && strcmp(cont[i].numero,num) == 0) {
					flag = 1;				
				}
				if(flag == 1 && i < contCount - 1) {
					strcpy(cont[i].numero,cont[i + 1].numero);
					strcpy(cont[i].ip,cont[i + 1].ip);
					cont[i].porta = cont[i + 1].porta;
				}
				i++;
			}
			contCount--;
			pthread_mutex_unlock(&mutex);
			printf("Numero removido do servidor: %s\n", num);
			
			break;
		}
	}
}

int main(int argc, char **argv) {

	pthread_t tratarClientes;
	unsigned short port;
	int ns;
	struct sockaddr_in client;
	struct sockaddr_in server;
	struct infocliente informacoes;
	int namelen, tc, i = 0;
	void *ret;
	signal(SIGINT, INThandler);

	if (pthread_mutex_init(&mutex, NULL) != 0) {

		printf("Falha de iniciacao de mutex\n");
	       	return 1; 
    	}
	
	if (argc != 2) {

		fprintf(stderr, "Use: %s porta\n", argv[0]);
		exit(1);
	}

	port = (unsigned short) atoi(argv[1]);
	
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {

		perror("Socket()");
		exit(2);
	}

	server.sin_family = AF_INET;   
   	server.sin_port   = htons(port);       
   	server.sin_addr.s_addr = INADDR_ANY;

		 
    	if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
       		
	       	perror("Bind()");
	       	exit(3);
   	}

	if (listen(s, 1) != 0) {

		perror("Listen()");
       		exit(4);
   	}
	namelen = sizeof(client);

	while(1) {

		if ((ns = accept(s, (struct sockaddr *) &client, (socklen_t *) &namelen)) == -1) {

			perror("Accept()");
			exit(5);
		}

		informacoes.ns = ns;
		informacoes.client = client;
    	
	    	tc = pthread_create(&tratarClientes, NULL, tratamento, &informacoes);
	    	if (tc) {

	     		printf("ERRO: impossivel criar um thread consumidor\n");
	      		exit(-1);
	    	}

    		usleep(250);
  	}
}

void INThandler(int sig) {

	int i = 0;
	void *ret;
	pthread_mutex_destroy(&mutex);
	close(s);
	pthread_exit(NULL);
	exit(0);
}
