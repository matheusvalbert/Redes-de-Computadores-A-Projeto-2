#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio_ext.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/netdevice.h>
#include <signal.h>
#include <pthread.h>
#include <stdio_ext.h>

struct recebido {
	
	char nome[20], mensagem[100];
	int flag;
};

struct agenda {

	char nome[20], numero[20];
};

struct contato {

	char numero[20], ip[20];
	int porta;
};

struct infocliente {

	struct sockaddr_in client;
	int ns;
};

struct grupos {

	char nome[20], pessoas[20][20], mensagens[100][100], msgEnv[100][100], imagens[100][100], imgEnv[100][100];
	int numero, numeroMsg, numeroImagem;
};


struct recebido mensagensRecebidas[200];
struct recebido imagensRecebidas[200];
int imagensRecebidasCount = 0;
int mensagensRecebidasCount = 0;
pthread_mutex_t mutexNumero, mutexMensagens;
char numero[20];
void INThandler(int);
int p2ps;
struct agenda contatos[20];
int numeroContatos = 0;
struct grupos classificacaoGrupo[20];
int numeroDeGrupos = 0;

void enviarInformacoes(int s) {

	struct ifconf ifconf;
 	struct ifreq ifr[50];
  	int ifs;
  	int i;

  	pthread_mutex_lock(&mutexNumero);

  	ifconf.ifc_buf = (char *) ifr;
  	ifconf.ifc_len = sizeof ifr;

  	if (ioctl(p2ps, SIOCGIFCONF, &ifconf) == -1) {
    	perror("ioctl");
    	exit (0);
  	}

  	ifs = ifconf.ifc_len / sizeof(ifr[0]);

    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in *ips = (struct sockaddr_in *) &ifr[1].ifr_addr;

    if (!inet_ntop(AF_INET, &ips->sin_addr, ip, sizeof(ip))) {
      	perror("inet_ntop");
      	exit(0);
    }

    //printf("IP = %s, Porta: ", ip);

	struct sockaddr_in _self;
    int len = sizeof (_self);

	getsockname (p2ps, (struct sockaddr *) &_self, &len);
    //printf ("%d\n", ntohs (_self.sin_port));

    struct contato cont;

    strcpy(cont.numero, numero);
    strcpy(cont.ip, ip);
    cont.porta = ntohs(_self.sin_port);

    //printf("Numero: %s - IP: %s - Porta: %d\n", cont.numero, cont.ip, cont.porta);

    int op = 1;

	if (send(s, &op, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}    

    if (send(s, &cont, sizeof(cont), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	pthread_mutex_unlock(&mutexNumero);
}

void *tratamento(void *informacoes) {

	struct sockaddr_in client;
	struct infocliente info;
	int ns;
	info = *(struct infocliente*) informacoes;
	client = info.client;
	ns = info.ns;

	int len1;
    int len2;
    char numeroEnviar[20], mensagemParaContato[20], mensagemGrupo[100], nomeGrupo[20], nCont[20], imagemGrupo[100];
    int funcao;

    if (recv(ns, &funcao, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	if(funcao == 1) {

	   	 if (recv(ns, &len2, sizeof(int), 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

		
		if (recv(ns, numeroEnviar, len2, 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, &len1, sizeof(int), 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

	   	if (recv(ns, mensagemParaContato, len1, 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

		strcpy(mensagensRecebidas[mensagensRecebidasCount].nome, numeroEnviar);
		strcpy(mensagensRecebidas[mensagensRecebidasCount].mensagem, mensagemParaContato);
		mensagensRecebidas[mensagensRecebidasCount].flag = 0;
		mensagensRecebidasCount++;
	}
	else if(funcao == 2) {

		if (recv(ns, &len1, sizeof(int), 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

	    if (recv(ns, mensagemParaContato, len1, 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, &classificacaoGrupo[numeroDeGrupos].numero, sizeof(int), 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		for (int i = 0; i < classificacaoGrupo[numeroDeGrupos].numero; i++) {

			len2 = strlen(classificacaoGrupo[numeroDeGrupos].pessoas[i]);

			if (recv(ns, &len2, sizeof(int), 0) == -1) {
				
				perror("Recv()");
				exit(6);
			}

			if (recv(ns, &classificacaoGrupo[numeroDeGrupos].pessoas[i], len2, 0) == -1) {
				
				perror("Recv()");
				exit(6);
			}
		}

		strcpy(classificacaoGrupo[numeroDeGrupos].nome, mensagemParaContato);
		numeroDeGrupos++;
		classificacaoGrupo[numeroDeGrupos].numeroMsg = 0;
		classificacaoGrupo[numeroDeGrupos].numeroImagem = 0;
	}

	else if(funcao == 3) {


		if (recv(ns, &len2, sizeof(int), 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, mensagemGrupo, len2, 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}


		if (recv(ns, &len2, sizeof(int), 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, nomeGrupo, len2, 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, &len2, sizeof(int), 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, nCont, len2, 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		for(int i = 0; i < numeroDeGrupos; i++) {

			if(strcmp(nomeGrupo, classificacaoGrupo[i].nome) == 0) {
				strcpy(classificacaoGrupo[i].mensagens[classificacaoGrupo[i].numeroMsg], mensagemGrupo);
				strcpy(classificacaoGrupo[i].msgEnv[classificacaoGrupo[i].numeroMsg], nCont);
				classificacaoGrupo[i].numeroMsg++;
			}
		}
	}
	else if(funcao == 4) {
		unsigned char buffer[1024];
		char nomeArquivo[50];
		if (recv(ns, &len2, sizeof(int), 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, nomeArquivo, len2, 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, &len1, sizeof(int), 0) == -1) {

			perror("Recv()");
			exit(6);
		}
		FILE *ptr;

		ptr = fopen(nomeArquivo,"wb");

		int nvezes = len1/1024;

		while(nvezes != 0) {

			if (recv(ns, buffer, 1024*sizeof(char), 0) == -1) {

				perror("Recv()");
				exit(6);
			}
			fwrite(buffer,1024,1,ptr);
			nvezes--;
		}

		if(len1%1024 != 0) {

			if (recv(ns, buffer, (len1%1024)*sizeof(char), 0) == -1) {

				perror("Recv()");
				exit(6);
			}
			fwrite(buffer,len1%1024,1,ptr);
		}
		strcpy(imagensRecebidas[imagensRecebidasCount].nome, numeroEnviar);
		strcpy(imagensRecebidas[imagensRecebidasCount].mensagem, nomeArquivo);
		imagensRecebidas[imagensRecebidasCount].flag = 0;
		imagensRecebidasCount++;
		fclose(ptr);

	}
	else if(funcao == 5) {
		
		if (recv(ns, &len2, sizeof(int), 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, nomeGrupo, len2, 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, &len2, sizeof(int), 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, nCont, len2, 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}


		unsigned char buffer[1024];
		char nomeArquivo[100];
		if (recv(ns, &len2, sizeof(int), 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, nomeArquivo, len2, 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, &len1, sizeof(int), 0) == -1) {

			perror("Recv()");
			exit(6);
		}
		FILE *ptr;

		ptr = fopen(nomeArquivo,"wb");

		int nvezes = len1/1024;

		while(nvezes != 0) {

			if (recv(ns, buffer, 1024*sizeof(char), 0) == -1) {

				perror("Recv()");
				exit(6);
			}
			fwrite(buffer,1024,1,ptr);
			nvezes--;
		}

		if(len1%1024 != 0) {

			if (recv(ns, buffer, (len1%1024)*sizeof(char), 0) == -1) {

				perror("Recv()");
				exit(6);
			}
			fwrite(buffer,len1%1024,1,ptr);
		}
		fclose(ptr);

		for(int i = 0; i < numeroDeGrupos; i++) {

			if(strcmp(nomeGrupo, classificacaoGrupo[i].nome) == 0) {

				strcpy(classificacaoGrupo[i].imagens[classificacaoGrupo[i].numeroImagem], nomeArquivo);
				strcpy(classificacaoGrupo[i].imgEnv[classificacaoGrupo[i].numeroImagem], nCont);
				classificacaoGrupo[i].numeroImagem++;
			}
		}
	}

	close(ns);
}
 
void p2p(int s_server) {

	pthread_t tratarClientes;
	int ns;
	struct sockaddr_in client;
	struct sockaddr_in server;
	struct infocliente informacoes;
	int namelen, tc, i = 0;
	void *ret;
	signal(SIGINT, INThandler);
	
	if ((p2ps = socket(PF_INET, SOCK_STREAM, 0)) < 0) {

		perror("Socket()");
		exit(2);
	}

	server.sin_family = AF_INET;   
   	server.sin_port   = 0;       
   	server.sin_addr.s_addr = INADDR_ANY;

		 
    if (bind(p2ps, (struct sockaddr *)&server, sizeof(server)) < 0) {
       		
       	perror("Bind()");
       	exit(3);
   	}

	if (listen(p2ps, 1) != 0) {

		perror("Listen()");
       	exit(4);
   	}
	namelen = sizeof(client);

	enviarInformacoes(s_server);

	while(1) {

		if ((ns = accept(p2ps, (struct sockaddr *) &client, (socklen_t *) &namelen)) == -1) {

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

void *p2pEnvio(void *arg) {

	int s = *((int *) arg);
	free(arg);

	p2p(s);
}

void contato() {

	char nomeContato[20], numeroContato[20];

	printf("Digite o nome: \n");
	scanf("%s", nomeContato);

	printf("Digite o numero: \n");
	scanf("%s", numeroContato);

	strcpy(contatos[numeroContatos].nome, nomeContato);
	strcpy(contatos[numeroContatos].numero, numeroContato);

	numeroContatos++;
}

void enviaMensagem(char ip[], int porta, char mensagemParaContato[]) {

	unsigned short port;             
	struct hostent *hostnm;    
    struct sockaddr_in server;
    int s;

	hostnm = gethostbyname(ip);
    if (hostnm == (struct hostent *) 0) {

        fprintf(stderr, "Gethostbyname failed\n");
        exit(2);
    }

    port = (unsigned short) porta;

    /*
     * Define o endereco IP e a porta do servidor
     */
    server.sin_family      = AF_INET;
    server.sin_port        = htons(port);
    server.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);

    /*
     * Cria um socket TCP (stream)
     */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {

        perror("Socket()");
        exit(3);
   	}

    /* 
	 * Estabelece conexao com o servidor 
	 */
    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {

        perror("Connect()");
        exit(4);
    }

    int len1 = strlen(mensagemParaContato);
    int len2 = strlen(numero);
    int funcao = 1;

    if (send(s, &funcao, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}
	

    if (send(s, &len2, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	if (send(s, numero, len2, 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	if (send(s, &len1, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

    if (send(s, mensagemParaContato, len1, 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	close(s);
}

void enviarMensagemContato(int s) {

	int numeroDoContato, op = 2, numLen;
	char mensagemParaContato[100], num[20];
	struct contato cont;
	char ip[20];
	int porta;

	printf("Contatos:\n");
	for (int i = 0; i < numeroContatos; ++i) {

		printf("%d - Nome: %s\n", i+1, contatos[i].nome);
	}

	printf("Digite o numero do contato\n");
	scanf("%d", &numeroDoContato);

	printf("Digite a mensagem: \n");
	__fpurge(stdin);
	fgets(mensagemParaContato, sizeof(mensagemParaContato), stdin);

	printf("\nNome contato: %d\nMensagem: %s\n", numeroDoContato, mensagemParaContato);

	if (send(s, &op, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	strcpy(num, contatos[numeroDoContato - 1].numero);

	numLen = strlen(num);

	if (send(s, &numLen, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	if (send(s, num, numLen, 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	if (recv(s, &numLen, sizeof(int), 0) == -1) {
			
		perror("Send()");
		exit(6);
	}

	if(numLen != -1) {

		if (recv(s, ip, numLen, 0) == -1) {
		
			perror("Recv()");
			exit(6);
		}

		if (recv(s, &porta, sizeof(int), 0) == -1) {
		
			perror("Recv()");
			exit(6);
		}

		enviaMensagem(ip, porta, mensagemParaContato);

		strcpy(mensagensRecebidas[mensagensRecebidasCount].nome, num);
		strcpy(mensagensRecebidas[mensagensRecebidasCount].mensagem, mensagemParaContato);
		mensagensRecebidas[mensagensRecebidasCount].flag = 1;
		mensagensRecebidasCount++;
	}
	else
		printf("O contato: %s nao esta conectado no momento.\n", contatos[numeroDoContato - 1].nome);
}



void visualizarMensagemContato() {

	int printAux[200];
	char nomeAux[20];

	for (int i = 0; i < mensagensRecebidasCount; i++) {

		printAux[i] = 0;
	}

	for (int i = 0; i < numeroContatos; i++) {

		for (int j = 0; j < mensagensRecebidasCount; j++) {

			if(strcmp(contatos[i].numero, mensagensRecebidas[j].nome) == 0) {

				strcpy(mensagensRecebidas[j].nome, contatos[i].nome);
			}
		}
	}

	for (int i = 0; i < mensagensRecebidasCount; ++i) {

		if(printAux[i] == 0) {

			strcpy(nomeAux, mensagensRecebidas[i].nome);
			printf("Chat com %s:\n", nomeAux);
		}

		for(int j = 0; j < mensagensRecebidasCount; j++) {

			if(strcmp(mensagensRecebidas[j].nome, nomeAux) == 0 && printAux[j] == 0) {
				if(mensagensRecebidas[j].flag == 0)
					printf("%s: %s\n", mensagensRecebidas[j].nome, mensagensRecebidas[j].mensagem);
				else
					printf("Eu: %s\n", mensagensRecebidas[j].mensagem);
				printAux[j] = 1;

			}
		}
		
		//printf("Nome: %s - mensagem: %s - flag: %d\n", mensagensRecebidas[i].nome, mensagensRecebidas[i].mensagem, mensagensRecebidas[i].flag);
	}
}

void enviarImagem(char ip[], int porta, char imagemParaContato[]) {

	unsigned short port;             
	struct hostent *hostnm;    
    struct sockaddr_in server;
    int s;

	hostnm = gethostbyname(ip);
    if (hostnm == (struct hostent *) 0) {

        fprintf(stderr, "Gethostbyname failed\n");
        exit(2);
    }

    port = (unsigned short) porta;

    /*
     * Define o endereco IP e a porta do servidor
     */
    server.sin_family      = AF_INET;
    server.sin_port        = htons(port);
    server.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);

    /*
     * Cria um socket TCP (stream)
     */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {

        perror("Socket()");
        exit(3);
   	}

    /* 
	 * Estabelece conexao com o servidor 
	 */
    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {

        perror("Connect()");
        exit(4);
    }

    int size;
	unsigned char buffer[1024];
    int funcao = 4;

    if (send(s, &funcao, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	size = strlen(imagemParaContato);

	if (send(s, &size, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	if (send(s, imagemParaContato, size, 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}
	FILE *ptr;
	ptr = fopen(imagemParaContato,"rb");
	if(ptr == NULL){

		perror("Open()");
		exit(0);
	}

	fseek(ptr, 0, SEEK_END);
	size = (int)ftell(ptr);
	rewind(ptr);

	if (send(s, &size, sizeof(size), 0) < 0) {

		perror("Send()");
		exit(5);
	}

	int nvezes = size/1024;

	while(nvezes != 0) {

		fread(buffer,1024,1,ptr);
		if (send(s, buffer, 1024*sizeof(char), 0) < 0) {

			perror("Send()");
			exit(5);
		}
		nvezes--;
	}

	if(size%1024 != 0) {

		fread(buffer,size%1024,1,ptr);
		if (send(s, buffer, (size%1024)*sizeof(char), 0) < 0) {

			perror("Send()");
			exit(5);
		}
	}

	fclose(ptr);
	close(s);

}

void enviarImagemContato(int s)
{
	int numeroDoContato, op = 2, numLen;
	char imagemParaContato[50], num[20];
	struct contato cont;
	char ip[20];
	int porta;

	printf("Contatos:\n");
	for (int i = 0; i < numeroContatos; ++i) {

		printf("%d - Nome: %s\n", i+1, contatos[i].nome);
	}

	printf("Digite o numero do contato\n");
	scanf("%d", &numeroDoContato);

	printf("Digite o nome da imagem: \n");
	__fpurge(stdin);
	scanf("%s", imagemParaContato);	
	//fgets(imagemParaContato, sizeof(imagemParaContato), stdin);

	printf("\nNome contato: %d\nMensagem: %s\n", numeroDoContato, imagemParaContato);
	if (send(s, &op, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	strcpy(num, contatos[numeroDoContato - 1].numero);

	numLen = strlen(num);

	if (send(s, &numLen, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	if (send(s, num, numLen, 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	if (recv(s, &numLen, sizeof(int), 0) == -1) {
			
		perror("Send()");
		exit(6);
	}

	if(numLen != -1) {

		if (recv(s, ip, numLen, 0) == -1) {
		
			perror("Recv()");
			exit(6);
		}

		if (recv(s, &porta, sizeof(int), 0) == -1) {
		
			perror("Recv()");
			exit(6);
		}
		ip[numLen] = '\0';
		enviarImagem(ip, porta, imagemParaContato);

		strcpy(imagensRecebidas[imagensRecebidasCount].nome, num);
		strcpy(imagensRecebidas[imagensRecebidasCount].mensagem, imagemParaContato);
		imagensRecebidas[imagensRecebidasCount].flag = 1;
		imagensRecebidasCount++;
	}
	else
		printf("O contato: %s nao esta conectado no momento.\n", contatos[numeroDoContato - 1].nome);
}

void visualizarImagemContato() {

	int printAux[200];
	char nomeAux[20];

	for (int i = 0; i < mensagensRecebidasCount; i++) {

		printAux[i] = 0;
	}

	for (int i = 0; i < numeroContatos; i++) {

		for (int j = 0; j < mensagensRecebidasCount; j++) {

			if(strcmp(contatos[i].numero, mensagensRecebidas[j].nome) == 0) {

				strcpy(mensagensRecebidas[j].nome, contatos[i].nome);
			}
		}
	}

	for (int i = 0; i < mensagensRecebidasCount; ++i) {

		if(printAux[i] == 0) {

			strcpy(nomeAux, mensagensRecebidas[i].nome);
			printf("Imagens trocadas com %s:\n", nomeAux);
		}

		for(int j = 0; j < mensagensRecebidasCount; j++) {

			if(strcmp(mensagensRecebidas[j].nome, nomeAux) == 0 && printAux[j] == 0) {
				if(mensagensRecebidas[j].flag == 0)
					printf("%s: %s\n", mensagensRecebidas[j].nome, mensagensRecebidas[j].mensagem);
				else
					printf("Eu: %s\n", mensagensRecebidas[j].mensagem);
				printAux[j] = 1;

			}
		}
		
		//printf("Nome: %s - mensagem: %s - flag: %d\n", mensagensRecebidas[i].nome, mensagensRecebidas[i].mensagem, mensagensRecebidas[i].flag);
	}
}

void enviarNomeGrupo(char nomeGrupo[], char ip[], int porta) {


	unsigned short port;             
	struct hostent *hostnm;    
    struct sockaddr_in server;
    int s;

	hostnm = gethostbyname(ip);
    if (hostnm == (struct hostent *) 0) {

        fprintf(stderr, "Gethostbyname failed\n");
        exit(2);
    }

    port = (unsigned short) porta;

    /*
     * Define o endereco IP e a porta do servidor
     */
    server.sin_family      = AF_INET;
    server.sin_port        = htons(port);
    server.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);

    /*
     * Cria um socket TCP (stream)
     */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {

        perror("Socket()");
        exit(3);
   	}

    /* 
	 * Estabelece conexao com o servidor 
	 */
    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {

        perror("Connect()");
        exit(4);
    }

	int len, op = 2;

	if (send(s, &op, sizeof(int), 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	len = strlen(nomeGrupo);

	if (send(s, &len, sizeof(int), 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	if (send(s, nomeGrupo, len, 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	if (send(s, &classificacaoGrupo[numeroDeGrupos].numero, sizeof(int), 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	for (int i = 0; i < classificacaoGrupo[numeroDeGrupos].numero; i++) {

		len = strlen(classificacaoGrupo[numeroDeGrupos].pessoas[i]);

		if (send(s, &len, sizeof(int), 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

		if (send(s, &classificacaoGrupo[numeroDeGrupos].pessoas[i], len, 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}
	}
}

void criarGrupo(int s) {

	int nPessoas, numeroPessoa, numLen, numLen2, op = 2, porta, j = 0, vetor[20];
	char nomeGrupo[20], ip[20], num[20];

	printf("Digite o nome do grupo: ");

	__fpurge(stdin);
	fgets(nomeGrupo, sizeof(nomeGrupo), stdin);

	printf("Numero de pessoas no grupo: ");

	scanf("%d", &nPessoas);

	printf("Pessoas:\n");

	for (int i = 0; i < numeroContatos; i++) {
		
		printf("%d - %s\n", i+1, contatos[i].nome);
	}

	printf("Digite os numeros das pessoas que deseja adicionar ao grupo: ");

	for (int i = 0; i < nPessoas; i++) {
		
		scanf("%d", &numeroPessoa);

		vetor[i] = numeroPessoa;

		strcpy(classificacaoGrupo[numeroDeGrupos].pessoas[j], contatos[numeroPessoa - 1].numero);

		j++;
		classificacaoGrupo[numeroDeGrupos].numero = nPessoas + 1;
	}

	strcpy(classificacaoGrupo[numeroDeGrupos].pessoas[j], numero);

	for (int i = 0; i < nPessoas; i++) {

		if (send(s, &op, sizeof(int), 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

		strcpy(num, contatos[vetor[i] - 1].numero);

		numLen = strlen(num);

		if (send(s, &numLen, sizeof(int), 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

		if (send(s, num, numLen, 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

		if (recv(s, &numLen2, sizeof(int), 0) == -1) {
				
			perror("Send()");
			exit(6);
		}

		if (recv(s, ip, numLen2, 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

		if (recv(s, &porta, sizeof(int), 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}
		ip[numLen2] = '\0';

		enviarNomeGrupo(nomeGrupo, ip, porta);
	}

	strcpy(classificacaoGrupo[numeroDeGrupos].nome, nomeGrupo);
	numeroDeGrupos++;
}

void enviarGrupo(char ip[], int porta, char mensagemGrupo[], char nomeGrupo[]) {

	unsigned short port;             
	struct hostent *hostnm;    
   	struct sockaddr_in server;
    	int s;

	hostnm = gethostbyname(ip);
    	if (hostnm == (struct hostent *) 0) {

      	  fprintf(stderr, "Gethostbyname failed\n");
     	   exit(2);
   	}

    port = (unsigned short) porta;

    /*
     * Define o endereco IP e a porta do servidor
     */
    server.sin_family      = AF_INET;
    server.sin_port        = htons(port);
    server.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);

    /*
     * Cria um socket TCP (stream)
     */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {

        perror("Socket()");
        exit(3);
   	}

    /* 
	 * Estabelece conexao com o servidor 
	 */
    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {

        perror("Connect()");
        exit(4);
    }

	int len, op = 3;

	if (send(s, &op, sizeof(int), 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	len = strlen(mensagemGrupo);

	if (send(s, &len, sizeof(int), 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	if (send(s, mensagemGrupo, len, 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	len = strlen(nomeGrupo);

	if (send(s, &len, sizeof(len), 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	if (send(s, nomeGrupo, len, 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	len = strlen(numero);

	if (send(s, &len, sizeof(len), 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	if (send(s, numero, len, 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}
}

void enviarMensagemGrupo(int s) {

	int numero, numLen, numLen2, porta, op = 2;
	char ip[20], num[20], mensagemGrupo[100];
	int flag = 0, vetor[20];
	printf("grupos:\n");

	for(int i = 0; i < numeroDeGrupos; i++) {

		printf("%d - %s\n", i+1, classificacaoGrupo[i].nome);
	}

	printf("Digite o numero do grupo: ");

	scanf("%d", &numero);

	printf("Digite a mensagem: \n");

	__fpurge(stdin);
	fgets(mensagemGrupo, sizeof(mensagemGrupo), stdin);

	for (int i = 0; i < classificacaoGrupo[numero - 1].numero; i++) {

		if (send(s, &op, sizeof(int), 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

		strcpy(num, classificacaoGrupo[numero - 1].pessoas[i]);

		numLen = strlen(num);

		if (send(s, &numLen, sizeof(int), 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

		if (send(s, num, numLen, 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

		if (recv(s, &numLen2, sizeof(int), 0) == -1) {
				
			perror("Send()");
			exit(6);
		}

		if(numLen2 != -1) {

			if (recv(s, ip, numLen2, 0) == -1) {
			
				perror("Recv()");
				exit(6);
			}

			if (recv(s, &porta, sizeof(int), 0) == -1) {
			
				perror("Recv()");
				exit(6);
			}
			ip[numLen2] = '\0';

			enviarGrupo(ip, porta, mensagemGrupo, classificacaoGrupo[numero - 1].nome);
		}
		else {
			vetor[flag] = i;
			flag++;
		}
	}
	if(flag > 0) {
		printf("O(s) contato(s) nao conectado(s) sao:\n");
		for(int i = 0; i < flag; i++) {
			printf("%s\n", classificacaoGrupo[numero - 1].pessoas[vetor[i]]);
		}
	}
}

void enviarImagemGr(char ip[], int porta, char imagemGrupo[], char nomeGrupo[]) {

	unsigned short port;             
	struct hostent *hostnm;    
   	struct sockaddr_in server;
    	int s, len;

	hostnm = gethostbyname(ip);
    	if (hostnm == (struct hostent *) 0) {

      	  fprintf(stderr, "Gethostbyname failed\n");
     	   exit(2);
   	}

    port = (unsigned short) porta;

    /*
     * Define o endereco IP e a porta do servidor
     */
    server.sin_family      = AF_INET;
    server.sin_port        = htons(port);
    server.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);

    /*
     * Cria um socket TCP (stream)
     */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {

        perror("Socket()");
        exit(3);
   	}

    /* 
	 * Estabelece conexao com o servidor 
	 */
    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {

        perror("Connect()");
        exit(4);
    }

    int size;
	unsigned char buffer[1024];
    int funcao = 5;

    if (send(s, &funcao, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	len = strlen(nomeGrupo);

	if (send(s, &len, sizeof(len), 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	if (send(s, nomeGrupo, len, 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	len = strlen(numero);

	if (send(s, &len, sizeof(len), 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	if (send(s, numero, len, 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	size = strlen(imagemGrupo);

	if (send(s, &size, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	if (send(s, imagemGrupo, size, 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}
	FILE *ptr;
	ptr = fopen(imagemGrupo,"rb");
	if(ptr == NULL){

		perror("Open()");
		exit(0);
	}

	fseek(ptr, 0, SEEK_END);
	size = (int)ftell(ptr);
	rewind(ptr);

	if (send(s, &size, sizeof(size), 0) < 0) {

		perror("Send()");
		exit(5);
	}

	int nvezes = size/1024;

	while(nvezes != 0) {

		fread(buffer,1024,1,ptr);
		if (send(s, buffer, 1024*sizeof(char), 0) < 0) {

			perror("Send()");
			exit(5);
		}
		nvezes--;
	}

	if(size%1024 != 0) {

		fread(buffer,size%1024,1,ptr);
		if (send(s, buffer, (size%1024)*sizeof(char), 0) < 0) {

			perror("Send()");
			exit(5);
		}
	}

	fclose(ptr);
	close(s);

}

void enviarImagemGrupo(int s,char numEnvio[]) {

	int numero, numLen, numLen2, porta, op = 2;
	char ip[20], imagemGrupo[100], num[20];
	int flag = 0, vetor[20];
	printf("grupos:\n");

	for(int i = 0; i < numeroDeGrupos; i++) {

		printf("%d - %s\n", i+1, classificacaoGrupo[i].nome);
	}

	printf("Digite o numero do grupo: ");

	scanf("%d", &numero);

	printf("Digite o nome da Imagem: \n");

	__fpurge(stdin);
	scanf("%s", imagemGrupo);

	for (int i = 0; i < classificacaoGrupo[numero - 1].numero; i++) {
		if(strcmp(classificacaoGrupo[numero - 1].pessoas[i],numEnvio) != 0) {

			if (send(s, &op, sizeof(int), 0) == -1) {
				
				perror("Recv()");
				exit(6);
			}

			strcpy(num, classificacaoGrupo[numero - 1].pessoas[i]);

			numLen = strlen(num);

			if (send(s, &numLen, sizeof(int), 0) == -1) {
				
				perror("Recv()");
				exit(6);
			}

			if (send(s, num, numLen, 0) == -1) {
				
				perror("Recv()");
				exit(6);
			}

			if (recv(s, &numLen2, sizeof(int), 0) == -1) {
					
				perror("Send()");
				exit(6);
			}

			if(numLen2 != -1) {

				if (recv(s, ip, numLen2, 0) == -1) {
				
					perror("Recv()");
					exit(6);
				}

				if (recv(s, &porta, sizeof(int), 0) == -1) {
				
					perror("Recv()");
					exit(6);
				}
				ip[numLen2] = '\0';

				enviarImagemGr(ip, porta, imagemGrupo, classificacaoGrupo[numero - 1].nome);
			}
			else {
				vetor[flag] = i;
				flag++;
			}
		}
		else {

			strcpy(classificacaoGrupo[numero - 1].imagens[classificacaoGrupo[numero - 1].numeroImagem], imagemGrupo);
			strcpy(classificacaoGrupo[numero - 1].imgEnv[classificacaoGrupo[numero - 1].numeroImagem], numEnvio);
			classificacaoGrupo[numero - 1].numeroImagem++;
		}
	}
	if(flag > 0) {
		printf("O(s) contato(s) nao conectado(s) e(sao):\n");
		for(int i = 0; i < flag; i++) {
			printf("%s\n", classificacaoGrupo[numero - 1].pessoas[vetor[i]]);
		}
	}
}

void visualizarMensagemGrupo() {

	for (int k = 0; k < numeroDeGrupos; k++) {

		for (int i = 0; i < classificacaoGrupo[k].numero; i++) {

			for (int j = 0; j < classificacaoGrupo[k].numeroMsg; j++) {

				if(strcmp(contatos[i].numero, classificacaoGrupo[k].msgEnv[j]) == 0) {

					strcpy(classificacaoGrupo[k].msgEnv[j], contatos[i].nome);
				}
			}
		}
	}

	for (int i = 0; i < numeroDeGrupos; ++i) {

		printf("Grupo: %s\n", classificacaoGrupo[i].nome);

		for(int j = 0; j < classificacaoGrupo[i].numeroMsg; j++) {

			if(strcmp(classificacaoGrupo[i].msgEnv[j], numero) == 0)
				printf("Eu: %s\n", classificacaoGrupo[i].mensagens[j]);				
			else
				printf("%s: %s\n", classificacaoGrupo[i].msgEnv[j], classificacaoGrupo[i].mensagens[j]);
		}
	}
}

void visualizarImagemGrupo(char num[]) {

	for (int k = 0; k < numeroDeGrupos; k++) {

		for (int i = 0; i < classificacaoGrupo[k].numero; i++) {

			for (int j = 0; j < classificacaoGrupo[k].numeroImagem; j++) {

				if(strcmp(contatos[i].numero, classificacaoGrupo[k].imgEnv[j]) == 0) {

					strcpy(classificacaoGrupo[k].imgEnv[j], contatos[i].nome);
				}
			}
		}
	}

	for (int i = 0; i < numeroDeGrupos; ++i) {

		printf("Grupo: %s\n", classificacaoGrupo[i].nome);

		for(int j = 0; j < classificacaoGrupo[i].numeroImagem; j++) {

			if(strcmp(classificacaoGrupo[i].imgEnv[j], numero) == 0)
				printf("Eu: %s\n", classificacaoGrupo[i].imagens[j]);				
			else
				printf("%s: %s\n", classificacaoGrupo[i].imgEnv[j], classificacaoGrupo[i].imagens[j]);
		}
	}
}

void desconectar(int s, char numero[]) {
	
	int len, op = 3;	
	
	if (send(s, &op, sizeof(int), 0) == -1) {
			
			perror("Recv()");
			exit(6);
	}
	len = strlen(numero);

	if (send(s, &len, sizeof(int), 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	if (send(s, numero, len, 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}
}

void *interface(void *arg1) {

	int s1 = *((int *) arg1);
	free(arg1);
    	int opcao;
    	char ip[20], porta[20];
	int flag = 0;

	printf("Digite o numero de telefone: ");

	scanf("%s", numero);

	printf("O numero eh: %s\n", numero);

	pthread_mutex_unlock(&mutexNumero);


	while(flag == 0) {

		printf("1 - Adicionar contato\n");
		printf("2 - Criar grupo\n");
		printf("3 - Enviar mensagem\n");
		printf("4 - Visualizar mensagens recebidas\n");
		printf("5 - Enviar imagem\n");
		printf("6 - Visualizar imagens recebidas\n");
		printf("7 - Enviar mensagem grupo\n");
		printf("8 - Visualizar mensagem grupo\n");
		printf("9 - Enviar imagem grupo\n");
		printf("10 - Visualizar imagem grupo\n");
		printf("0 - Sair\n");

		printf("Digite um numero: ");

		scanf("%i", &opcao);

		switch(opcao) {

			case 1:
				contato();
				break;
			case 2:
				criarGrupo(s1);
				break;
			case 3:
				enviarMensagemContato(s1);
				break;
			case 4:
				visualizarMensagemContato();
				break;
			case 5:
				enviarImagemContato(s1);
				break;
			case 6:
				visualizarImagemContato();
				break;
			case 7:
				enviarMensagemGrupo(s1);
				break;
			case 8:
				visualizarMensagemGrupo();
				break;
			case 9:
				enviarImagemGrupo(s1,numero);
				break;
			case 10:
				visualizarImagemGrupo(numero);
				break;
			case 0:
				desconectar(s1,numero);
				flag = 1;
				break;
			default:	
				printf("Opcao Invalida\n");
				break;
		}
	}
}

int main(int argc, char **argv) {

	pthread_t tratarClientes[2];
	unsigned short port;             
	struct hostent *hostnm;    
    struct sockaddr_in server; 
    int tc, s;
    int *arg = malloc(sizeof(*arg));
    int *arg1 = malloc(sizeof(*arg1));

    if (pthread_mutex_init(&mutexNumero, NULL) != 0) {

    	printf("falha iniciacao semaforo\n");
       	return 1; 
    }

    if (pthread_mutex_init(&mutexMensagens, NULL) != 0) {

    	printf("falha iniciacao semaforo\n");
       	return 1; 
    }

    pthread_mutex_lock(&mutexNumero);
    pthread_mutex_lock(&mutexMensagens);

	hostnm = gethostbyname(argv[1]);
    if (hostnm == (struct hostent *) 0) {

        fprintf(stderr, "Gethostbyname failed\n");
        exit(2);
    }
    port = (unsigned short) atoi(argv[2]);

    /*
     * Define o endereco IP e a porta do servidor
     */
    server.sin_family      = AF_INET;
    server.sin_port        = htons(port);
    server.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);

    /*
     * Cria um socket TCP (stream)
     */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {

        perror("Socket()");
        exit(3);
   	}

    /* 
	 * Estabelece conexao com o servidor 
	 */
    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {

        perror("Connect()");
        exit(4);
    }

    *arg = s;
    *arg1 = s;

    tc = pthread_create(&tratarClientes[0], NULL, p2pEnvio, arg);
    if (tc) {
     	printf("ERRO: impossivel criar um thread consumidor\n");
      	exit(-1);
    }


    tc = pthread_create(&tratarClientes[1], NULL, interface, arg1);
    if (tc) {
     	printf("ERRO: impossivel criar um thread consumidor\n");
      	exit(-1);
    }
	pthread_join(tratarClientes[1], NULL);
    	pthread_cancel(tratarClientes[0]);
	return 0;
}

void INThandler(int sig) {

	int i = 0;
	void *ret;
	pthread_mutex_destroy(&mutexNumero);
	pthread_mutex_destroy(&mutexMensagens);
	close(p2ps);
	pthread_exit(NULL);
	exit(0);
}
