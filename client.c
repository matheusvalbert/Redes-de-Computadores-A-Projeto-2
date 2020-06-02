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

	char nome[20], pessoas[20][20];
	int numero;
};


struct recebido mensagensRecebidas[200];
struct recebido imagensRecebidas[200];
pthread_mutex_t mutexNumero, mutexGrupos, mutexContatos;
char numero[20];
void INThandler(int);
int p2ps;
struct agenda contatos[20];
int numeroContatos = 0;
struct grupos classificacaoGrupo[20];
int numeroDeGrupos = 0;
int socketcontrol = 0;

int escreverDados(char numero[])
{

	pthread_mutex_lock(&mutexContatos);
	pthread_mutex_lock(&mutexGrupos);
	struct agenda c[20];
	int numContatos = 0;
	struct grupos cGrupo[20];
	int numGrupos = 0;
	FILE *file=NULL;
	char numerostr[25];
	strcpy(numerostr,numero);
	file = fopen(numerostr,"wb");
	if(file == NULL)
 	{
    		perror("Nao foi possivel criar o arquivo ou acessa-lo");
   		exit(1);
  	}
  	fwrite(&numeroContatos, sizeof(int), 1, file);
  	fwrite(contatos, sizeof(contatos), numeroContatos, file);
  	fwrite(&numeroDeGrupos, sizeof(int), 1, file);
  	fwrite(classificacaoGrupo, sizeof(classificacaoGrupo), numeroDeGrupos, file);
  	fclose(file);
  	
  	pthread_mutex_unlock(&mutexGrupos);
  	pthread_mutex_unlock(&mutexContatos);
}

void leituraDados(char numero[]) {
	FILE *file=NULL;
	char numerostr[25];
	strcpy(numerostr,numero);
	file = fopen(numerostr,"rb");
	if(file != NULL)
	{
		fread(&numeroContatos, sizeof(int), 1, file);
		fread(contatos, sizeof(contatos), numeroContatos, file);
		fread(&numeroDeGrupos, sizeof(int), 1, file);
		fread(classificacaoGrupo, sizeof(classificacaoGrupo), numeroDeGrupos, file);
	 	fclose(file);
	}
}

void enviarInformacoes(int s) {

	struct ifconf ifconf;
 	struct ifreq ifr[50];
  	int ifs;
  	int i;
  	int flag = 0;

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


	struct sockaddr_in _self;
    	int len = sizeof (_self);

	getsockname (p2ps, (struct sockaddr *) &_self, &len);

    	struct contato cont;

    	strcpy(cont.numero, numero);
  	strcpy(cont.ip, ip);
 	cont.porta = ntohs(_self.sin_port);


  	int op = 1;

	if (send(s, &op, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}    

   	if (send(s, &cont, sizeof(cont), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	if (recv(s, &flag, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	if(flag == -1) {
		printf("Numero ja em uso.\n");
		exit(0);
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
    	int tamanhoNomeGrupo;
    	char numeroEnviar[20], mensagemParaContato[20], mensagemGrupo[100], nomeGrupo[20], nCont[20], imagemGrupo[100];
    	int funcao;
	int i, flag;
	unsigned char buffer[1024];
	char nomeArquivo[100];
	int nvezes;
	FILE *arquivo; 
	
    	if (recv(ns, &funcao, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	if(funcao == 1) {//Area de tratamento para receber mensagens de Contatos

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
		flag = 0;
		pthread_mutex_lock(&mutexContatos);
		for(i = 0; i < numeroContatos; i++) {
			if(strcmp(numeroEnviar,contatos[i].numero) == 0) {
				printf("Mensagem recebida de %s: %s\n", contatos[i].nome, mensagemParaContato);
				flag = 1;
			}
		}
		pthread_mutex_unlock(&mutexContatos);
		usleep(100);
		if(flag == 0)
			printf("Mensagem recebida de %s: %s\n", numeroEnviar, mensagemParaContato);
	}
	else if(funcao == 2) {//Area de tratamento para adição de numeros aos grupos

		if (recv(ns, &len1, sizeof(int), 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

	   	if (recv(ns, mensagemParaContato, len1, 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}
		mensagemParaContato[len1]='\0';
		pthread_mutex_lock(&mutexGrupos);
		if (recv(ns, &classificacaoGrupo[numeroDeGrupos].numero, sizeof(int), 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		for (i = 0; i < classificacaoGrupo[numeroDeGrupos].numero; i++) {

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
		pthread_mutex_unlock(&mutexGrupos);
		pthread_mutex_lock(&mutexNumero);
		escreverDados(numero);
		pthread_mutex_unlock(&mutexNumero);
	}

	else if(funcao == 3) {//Area de tratamento para receber mensagens de Grupos


		if (recv(ns, &len2, sizeof(int), 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, mensagemGrupo, len2, 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}


		if (recv(ns, &tamanhoNomeGrupo, sizeof(int), 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, nomeGrupo, tamanhoNomeGrupo, 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}
		
		nomeGrupo[len2]='\0';
		if (recv(ns, &len2, sizeof(int), 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, nCont, len2, 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		flag = 0;
		pthread_mutex_lock(&mutexContatos);
		for(i = 0; i < numeroContatos; i++) {
			if(strcmp(nCont,contatos[i].numero) == 0) {
				printf("Mensagem recebida de: %s no grupo: ", contatos[i].nome);
				flag = 1;
			}
		}
		pthread_mutex_unlock(&mutexContatos);
		usleep(100);
		if(flag == 0)
			printf("Mensagem recebida de: %s no grupo: ", nCont);
		for(i = 0; i < tamanhoNomeGrupo; i++)
			printf("%c", nomeGrupo[i]);
		printf(": %s\n", mensagemGrupo);
	}
	else if(funcao == 4) {//Area de tratamento para receber imagens de Contatos

		if (recv(ns, &len2, sizeof(int), 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}

		
		if (recv(ns, nCont, len2, 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}
		
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

		arquivo = fopen(nomeArquivo,"wb");

		nvezes = len1/1024;

		while(nvezes != 0) {

			if (recv(ns, buffer, 1024*sizeof(char), 0) == -1) {

				perror("Recv()");
				exit(6);
			}
			fwrite(buffer,1024,1,arquivo);
			nvezes--;
		}

		if(len1%1024 != 0) {

			if (recv(ns, buffer, (len1%1024)*sizeof(char), 0) == -1) {

				perror("Recv()");
				exit(6);
			}
			fwrite(buffer,len1%1024,1,arquivo);
		}
		
		flag = 0;
		pthread_mutex_lock(&mutexContatos);
		usleep(100);
		for(i = 0; i < numeroContatos; i++) {
			if(strcmp(numeroEnviar,contatos[i].numero) == 0) {
				printf("Imagem recebida de %s: %s\n", contatos[i].nome, nomeArquivo);
				flag = 1;
			}
		}
		pthread_mutex_unlock(&mutexContatos);
		if(flag == 0)
			printf("Imagem recebida de %s: %s\n", nCont, nomeArquivo);
	}
	else if(funcao == 5) {//Area de tratamento para receber imagens de Grupos
		
		if (recv(ns, &tamanhoNomeGrupo, sizeof(int), 0) == -1) {
				
			perror("Recv()");
			exit(6);
		}

		if (recv(ns, nomeGrupo, tamanhoNomeGrupo, 0) == -1) {
				
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

		nvezes = len1/1024;

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
		flag = 0;
		pthread_mutex_lock(&mutexContatos);
		for(i = 0; i < numeroContatos; i++) {
			if(strcmp(nCont,contatos[i].numero) == 0) {
				printf("Imagem recebida de: %s no grupo: ", contatos[i].nome);
				flag = 1;
			}
		}
		usleep(100);
		pthread_mutex_unlock(&mutexContatos);
		
		if(flag == 0)
			printf("Imagem recebida de: %s no grupo: ", nCont);
		for(i = 0; i < tamanhoNomeGrupo; i++)
			printf("%c", nomeGrupo[i]);
		printf(": %s\n", nomeArquivo);
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

void contato() {//Função para adicionar um novo contato

	char nomeContato[20], numeroContato[20];

	printf("Digite o nome: \n");
	scanf("%s", nomeContato);

	printf("Digite o numero: \n");
	scanf("%s", numeroContato);

	pthread_mutex_lock(&mutexContatos);

	strcpy(contatos[numeroContatos].nome, nomeContato);
	strcpy(contatos[numeroContatos].numero, numeroContato);

	numeroContatos++;
	
	pthread_mutex_unlock(&mutexContatos);
}

void enviaMensagem(char ip[], int porta, char mensagemParaContato[], char numCliente[]) {//Função que realiza o envio dos dados ao contato

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
    	int len2 = strlen(numCliente);
    	int funcao = 1;

    	if (send(s, &funcao, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}
	

    	if (send(s, &len2, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	if (send(s, numCliente, len2, 0) == -1) {
		
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

void enviarMensagemContato(int s, char numCliente[]) {//Envio dos dados do contato e mensagem a função de envio de mensagens

	int numeroDoContato = 0, op = 2, numLen;
	char mensagemParaContato[100], num[20], nome[20];
	struct contato cont;
	char ip[20];
	int porta;

	printf("Contatos:\n");
	pthread_mutex_lock(&mutexContatos);
	for (int i = 0; i < numeroContatos; ++i) {

		printf("%d - Nome: %s\n", i+1, contatos[i].nome);
	}
	while(numeroDoContato < 1 || numeroDoContato > numeroContatos) {
		printf("Digite o numero do contato:\n");
		scanf("%d", &numeroDoContato);
		if(numeroDoContato < 1 || numeroDoContato > numeroContatos)
			printf("Numero invalido\n");
	}
	printf("Digite a mensagem: \n");
	__fpurge(stdin);
	fgets(mensagemParaContato, sizeof(mensagemParaContato), stdin);

	strcpy(num, contatos[numeroDoContato - 1].numero);
	strcpy(nome, contatos[numeroDoContato - 1].nome);

	printf("\nNome contato: %s - Mensagem: %s\n", nome, mensagemParaContato);

	if (send(s, &op, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	pthread_mutex_unlock(&mutexContatos);

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

		enviaMensagem(ip, porta, mensagemParaContato, numCliente);

	}
	else
		printf("O contato: %s nao esta conectado no momento.\n", nome);
}

void enviarImagem(char ip[], int porta, char imagemParaContato[], char numCliente[]) {//Envio da imagem ao contato

	unsigned short port;             
	struct hostent *hostnm;    
   	struct sockaddr_in server;
    	int s;
	int size;
	unsigned char buffer[1024];
	int funcao = 4;

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

	if (send(s, &funcao, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}
	size = strlen(numCliente);
	
	if (send(s, &size, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}

	if (send(s, numCliente, size, 0) == -1) {
		
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

void enviarImagemContato(int s, char numCliente[])//Envio dos dados de imagem ao contato
{
	int numeroDoContato = 0, op = 2, numLen;
	char imagemParaContato[50], num[20], nome[20];
	struct contato cont;
	char ip[20];
	int porta;

	printf("Contatos:\n");
	pthread_mutex_lock(&mutexContatos);
	for (int i = 0; i < numeroContatos; ++i) {

		printf("%d - Nome: %s\n", i+1, contatos[i].nome);
	}
	while(numeroDoContato < 1 || numeroDoContato > numeroContatos) {
		printf("Digite o numero do contato:\n");
		scanf("%d", &numeroDoContato);
		if(numeroDoContato < 1 || numeroDoContato > numeroContatos)
			printf("Numero invalido\n");
	}
	printf("Digite o nome da imagem: \n");
	__fpurge(stdin);
	scanf("%s", imagemParaContato);	

	strcpy(nome, contatos[numeroDoContato - 1].nome);
	strcpy(num, contatos[numeroDoContato - 1].numero);

	printf("Nome contato: %s - Nome da Imagem: %s\n", nome, imagemParaContato);
	
	if (send(s, &op, sizeof(int), 0) == -1) {
		
		perror("Recv()");
		exit(6);
	}
	pthread_mutex_unlock(&mutexContatos);
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
		enviarImagem(ip, porta, imagemParaContato, numCliente);

	}
	else
		printf("O contato: %s nao esta conectado no momento.\n", nome);
}

void enviarNomeGrupo(char nomeGrupo[], char ip[], int porta) {//Envio do nome do grupo e os numeros dos integrantes


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
	
	pthread_mutex_lock(&mutexGrupos);

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
	
	pthread_mutex_unlock(&mutexGrupos);
}

void criarGrupo(int s, char numCliente[]) {//Função de criação de grupos

	int nPessoas, numeroPessoa, numLen, numLen2, op = 2, porta, j = 0, vetor[20];
	char nomeGrupo[20], ip[20], num[20];
	
	printf("Digite o nome do grupo: ");

	__fpurge(stdin);
	fgets(nomeGrupo, sizeof(nomeGrupo), stdin);

	printf("Numero de pessoas no grupo: ");

	scanf("%d", &nPessoas);

	printf("Pessoas:\n");
	pthread_mutex_lock(&mutexContatos);
	for (int i = 0; i < numeroContatos; i++) {
		
		printf("%d - %s\n", i+1, contatos[i].nome);
	}
	pthread_mutex_unlock(&mutexContatos);
	printf("Digite os numeros das pessoas que deseja adicionar ao grupo: ");
	pthread_mutex_lock(&mutexGrupos);
	for (int i = 0; i < nPessoas; i++) {
		
		scanf("%d", &numeroPessoa);

		vetor[i] = numeroPessoa;

		strcpy(classificacaoGrupo[numeroDeGrupos].pessoas[j], contatos[numeroPessoa - 1].numero);

		j++;
		classificacaoGrupo[numeroDeGrupos].numero = nPessoas + 1;
	}

	strcpy(classificacaoGrupo[numeroDeGrupos].pessoas[j], numCliente);
	pthread_mutex_unlock(&mutexGrupos);
	for (int i = 0; i < nPessoas; i++) {

		if (send(s, &op, sizeof(int), 0) == -1) {
			
			perror("Recv()");
			exit(6);
		}
		pthread_mutex_lock(&mutexContatos);
		strcpy(num, contatos[vetor[i] - 1].numero);
		pthread_mutex_unlock(&mutexContatos);
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
	pthread_mutex_lock(&mutexGrupos);
	strcpy(classificacaoGrupo[numeroDeGrupos].nome, nomeGrupo);
	numeroDeGrupos++;
	pthread_mutex_unlock(&mutexGrupos);
}

void enviarGrupo(char ip[], int porta, char mensagemGrupo[], char nomeGrupo[], char numCliente[]) {//Função para envio dos dados aos usuario do grupo

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

	len = strlen(numCliente);

	if (send(s, &len, sizeof(len), 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	if (send(s, numCliente, len, 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}
}

void enviarMensagemGrupo(int s, char numCliente[]) {//Função para receber os dados da mensagem e os usuarios do grupo

	int numero = 0, numLen, numLen2, porta, op = 2;
	char ip[20], num[20], mensagemGrupo[100], nomeGrupo[20];
	int flag = 0, vetor[20];
	printf("grupos:\n");
	pthread_mutex_lock(&mutexGrupos);
	for(int i = 0; i < numeroDeGrupos; i++) {

		printf("%d - %s\n", i+1, classificacaoGrupo[i].nome);
	}
	
	while(numero < 1 || numero > numeroDeGrupos) {
		printf("Digite o numero do grupo:\n");
		scanf("%d", &numero);
		if(numero < 1 || numero > numeroDeGrupos)
			printf("Numero invalido\n");
	}

	strcpy(nomeGrupo,classificacaoGrupo[numero - 1].nome);
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

			enviarGrupo(ip, porta, mensagemGrupo, nomeGrupo, numCliente);
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
	pthread_mutex_unlock(&mutexGrupos);
}

void enviarImagemGr(char ip[], int porta, char imagemGrupo[], char nomeGrupo[], char numCliente[]) {//Função para envio das imagens ao outro cliente

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

	len = strlen(numCliente);

	if (send(s, &len, sizeof(len), 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	if (send(s, numCliente, len, 0) == -1) {
			
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

void enviarImagemGrupo(int s, char numCliente[]) {//Função chamada no tratamento para enviar os dados de envio ao outro usuario

	int numero = 0, numLen, numLen2, porta, op = 2;
	char ip[20], imagemGrupo[100], num[20];
	int flag = 0, vetor[20];
	printf("grupos:\n");
	pthread_mutex_lock(&mutexGrupos);
	for(int i = 0; i < numeroDeGrupos; i++) {

		printf("%d - %s\n", i+1, classificacaoGrupo[i].nome);
	}

	while(numero < 1 || numero > numeroDeGrupos) {
		printf("Digite o numero do grupo:\n");
		scanf("%d", &numero);
		if(numero < 1 || numero > numeroDeGrupos)
			printf("Numero invalido\n");
	}

	printf("Digite o nome da Imagem: \n");

	__fpurge(stdin);
	scanf("%s", imagemGrupo);

	for (int i = 0; i < classificacaoGrupo[numero - 1].numero; i++) {
		if(strcmp(classificacaoGrupo[numero - 1].pessoas[i],numCliente) != 0) {

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
				//Envio dos dados a outra função para envio da imagem
				enviarImagemGr(ip, porta, imagemGrupo, classificacaoGrupo[numero - 1].nome,numCliente);
			}
			else {
				vetor[flag] = i;
				flag++;
			}
		}
	}
	if(flag > 0) {
		printf("O(s) contato(s) nao conectado(s) e(sao):\n");
		for(int i = 0; i < flag; i++) {
			printf("%s\n", classificacaoGrupo[numero - 1].pessoas[vetor[i]]);
		}
	}
	pthread_mutex_unlock(&mutexGrupos);
}

void desconectar(int s, char numCliente[]) {//Funcao para desconexão com o servidor
	
	int len, op = 3;	
	
	if (send(s, &op, sizeof(int), 0) == -1) {
			
			perror("Recv()");
			exit(6);
	}
	len = strlen(numCliente);

	if (send(s, &len, sizeof(int), 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}

	if (send(s, numCliente, len, 0) == -1) {
			
		perror("Recv()");
		exit(6);
	}
}

void *interface(void *arg1) {//Interface para desmonstrar as funcoes ao usuario

	int s1 = *((int *) arg1);
	free(arg1);
    	int opcao;
    	char ip[20], porta[20];
	int flag = 0;
	char num[20];
	printf("Digite o numero de telefone: ");

	scanf("%s", numero);

	printf("O numero eh: %s\n", numero);
	strcpy(num,numero);
	socketcontrol = s1;
	pthread_mutex_unlock(&mutexNumero);
	usleep(150);
	leituraDados(num);


	while(flag == 0) {

		printf("1 - Adicionar contato\n");
		printf("2 - Criar grupo\n");
		printf("3 - Enviar mensagem\n");
		printf("4 - Enviar imagem\n");
		printf("5 - Enviar mensagem grupo\n");
		printf("6 - Enviar imagem grupo\n");
		printf("0 - Sair\n");

		printf("Digite um numero:\n");

		scanf("%i", &opcao);

		switch(opcao) {

			case 1:
				contato();
				escreverDados(num);
				break;
			case 2:
				criarGrupo(s1, num);
				escreverDados(num);
				break;
			case 3:
				enviarMensagemContato(s1, num);
				break;
			case 4:
				enviarImagemContato(s1, num);
				break;
			case 5:
				enviarMensagemGrupo(s1,num);
				break;
			case 6:
				enviarImagemGrupo(s1,num);
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
    	if (pthread_mutex_init(&mutexGrupos, NULL) != 0) {

    		printf("falha iniciacao semaforo\n");
       		return 1; 
    	}
    
    	if (pthread_mutex_init(&mutexContatos, NULL) != 0) {

    		printf("falha iniciacao semaforo\n");
       		return 1; 
    	}

    	pthread_mutex_lock(&mutexNumero);

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

void INThandler(int sig) {//Tratamento de finalizacao de codigo

	void *ret;
	desconectar(socketcontrol, numero);
	pthread_mutex_destroy(&mutexNumero);
	pthread_mutex_destroy(&mutexGrupos);
	pthread_mutex_destroy(&mutexContatos);
	close(p2ps);
	pthread_exit(NULL);
	exit(0);
}
