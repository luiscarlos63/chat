#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <pthread.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

void panic(char *msg);
#define panic(m)	{perror(m); abort();}
#define MAX_CLIENTS 10


//structs
typedef struct client_s{
	int sd_id;
	char name[20];
	pthread_t th_id;
}client_t;

typedef struct message_s
{
	client_t cli;
	char buff[256];
}message_t;


struct node_s
{
    message_t message;
    struct node_s *next;
};
typedef struct node_s node_t;

typedef struct message_queue_s
{
    int count;
    node_t *front;
    node_t *rear;
}message_queue_t;

//prototipos
void *threadfuntion(void *arg);

void insert_client(client_t cli);
client_t remove_client(client_t cli);

void send_message_handler(char* buffer, client_t cli);
void initialize(message_queue_t *q);
int isempty(message_queue_t *q);
void enqueue(message_queue_t *q, message_t message);
message_t dequeue(message_queue_t *q);

//variaveis globais
client_t client_arry[MAX_CLIENTS];
message_queue_t message_queue;

int main(int count, char *args[])
{	
	struct sockaddr_in serv_addr, cli_addr;
	int listen_sd, port;
	

	initialize(&message_queue);


	//inicalizar a lista de clientes
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		client_arry[i].sd_id = 0;
	}
	

	// ------------------------------------------------------------------------------------------
	if ( count != 2 )
	{
		printf("usage: %s <protocol or portnum>\n", args[0]);
		exit(0);
	}

	/*---Get server's IP and standard service connection--*/
	if ( !isdigit(args[1][0]) )
	{
		struct servent *srv = getservbyname(args[1], "tcp");
		if ( srv == NULL )
			panic(args[1]);
		printf("%s: port=%d\n", srv->s_name, ntohs(srv->s_port));
		port = srv->s_port;
	}
	else
		port = htons(atoi(args[1]));

	/*--- create socket ---*/
	listen_sd = socket(PF_INET, SOCK_STREAM, 0);
	if ( listen_sd < 0 )
		panic("socket");

	/*--- bind port/address to socket ---*/
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = port;
	serv_addr.sin_addr.s_addr = INADDR_ANY;                   /* any interface */
	if ( bind(listen_sd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != 0 )
		panic("bind");
	// ------------------------------------------------------------------------------------------



	/*--- make into listener with 10 slots ---*/
	if ( listen(listen_sd, 10) != 0 )
		panic("listen")
	else	/*--- begin waiting for connections ---*/
	{	
		while (1)                         /* process all incoming clients */
		{
			client_t cli;
			int n = sizeof(cli_addr);
			cli.sd_id = accept(listen_sd, (struct sockaddr*)&cli_addr, &n);     /* accept connection */
			if(cli.sd_id!=-1)
			{
				pthread_t child;
				insert_client(cli);
				printf("New connection\n");
				pthread_create(&child, 0, threadfuntion, &cli);       /* start thread */
				pthread_detach(child);                      /* don't track it */
			}
		}
	}
}




void *threadfuntion(void *arg)                    
{	
	client_t cli = *(client_t*)arg;            /* get & convert the socket */
	char buffer[256];


	recv(cli.sd_id,buffer,sizeof(buffer),0);
	strcpy(cli.name, buffer);
	printf("%s: ola meus putos\n", cli.name);
	
	send_message_handler(buffer, cli);				//esta funçao ira mandar a 

	

	send(cli.sd_id,buffer,sizeof(buffer),0);

	// shutdown(cli.sd_id,SHUT_RD);
	// shutdown(cli.sd_id,SHUT_WR);
	// shutdown(cli.sd_id,SHUT_RDWR);
	// 		                  /* close the client's channel */
	return 0;                           /* terminate the thread */
}


// +++++++++++++++++++++++++++++++++++++++++ FUNÇOES PARA METER E TIRAR clientes da lista +++++++++++++++++++++++
void insert_client(client_t cli)
{
	int i = 0;
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(client_arry[i].sd_id == 0)
		{
			client_arry[i] = cli;
			printf("client inserted: %d\n", cli.sd_id);
			break;
		}
	}
	if(i >= MAX_CLIENTS)
	{
		printf("erro: a lista esta cheia\n");
	}
}

client_t remove_clients(client_t cli)
{
	int i = 0;
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(client_arry[i].sd_id == cli.sd_id)
		{
			client_arry[i].sd_id = 0;
			return cli;
		}
	}
}


//--------------------------------	FUNÇOES para meter a mensagem numa "queqe" e MAIS ----------------------
void send_message_handler(char* buffer, client_t cli)
{
	message_t message;
	
	strcpy(message.buff, buffer);
	message.cli = cli;

	enqueue(&message_queue, message);
}

void initialize(message_queue_t *q)
{
    q->count = 0;
    q->front = NULL;
    q->rear = NULL;
}

int isempty(message_queue_t *q)
{
    return (q->rear == NULL);
}

void enqueue(message_queue_t *q, message_t message)
{
    if (q->count < MAX_CLIENTS)
    {
        node_t *tmp;
        tmp = malloc(sizeof(node_t));
        tmp->message = message;
        tmp->next = NULL;
        if(!isempty(q))
        {
            q->rear->next = tmp;
            q->rear = tmp;
        }
        else
        {
            q->front = q->rear = tmp;
        }
        q->count++;
    }
    else
    {
        printf("List is full\n");
    }
}

message_t dequeue(message_queue_t *q)
{
    node_t *tmp;
    message_t message = q->front->message;
    tmp = q->front;
    q->front = q->front->next;
    q->count--;
    free(tmp);
    return(message);
}


