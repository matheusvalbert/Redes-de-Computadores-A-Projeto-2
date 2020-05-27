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
struct contato cont[20];
int contCount = 0;
void INThandler(int);
int s;

void *tratamento(void *informacoes) {

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

		if(opcao == 1){

			if (recv(ns, &cont[contCount], sizeof(cont), 0) == -1) {
			
				perror("Recv()");
				exit(6);
			}

			printf("Numero: %s - IP: %s - Porta: %d\n", cont[contCount].numero, cont[contCount].ip, cont[contCount].porta);
			contCount++;
		}
		else if(opcao == 2) {
			
			env = -1;
			if (recv(ns, &numLen, sizeof(int), 0) == -1) {
			
				perror("Recv()");
				exit(6);
			}

			if (recv(ns, num, numLen, 0) == -1) {
				
				perror("Recv()");
				exit(6);
			}

			for (i = 0; i < contCount; ++i) {

				if(strcmp(cont[i].numero, num) == 0) {

					env = i;
					printf("Env %i\n",env);
				}
			}
			if(env != -1) {
				strcpy(ip, cont[env].ip);
				porta = cont[env].porta;
				numLen = strlen(ip);
					
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
				printf("Enviados os dados sobre num %s\n", num);
			}
			if(env == -1) {
			
				numLen = -1;
				if (send(ns, &numLen, sizeof(int), 0) == -1) {
			
					perror("Send()");
					exit(6);
				}
			} 
	
		}
		else if(opcao == 3) {
			 
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
			printf("Numero removido: %s\n", num);
			/*for(i=0;i<contCount;i++) {
				printf("%i - %s\n",i,cont[i].numero);
			}*/
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

        printf("falha iniciacao semaforo\n");
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
    		/*
		 * Cria uma thread para atender o cliente
		 */
		informacoes.ns = ns;
		informacoes.client = client;
    	
    	tc = pthread_create(&tratarClientes, NULL, tratamento, &informacoes);
    	if (tc) {

     		printf("ERRO: impossivel criar um thread consumidor\n");
      		exit(-1);
    	}
		/*
		 * A thread principal dorme por um pequeno período de tempo
		 * para a thread de tratamento retirar as informacoes do client
		 */
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
