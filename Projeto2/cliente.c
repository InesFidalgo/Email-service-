#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include "cabeca.h"

void erro(char *msg);
void imprimeMsg(listaM m,int id);

int main(int argc, char *argv[]) {
    char endServer[100];
    int fd;
    int aceitacao=-2;
    int registra=1;
    int enviaRegistra;
    int ver;
    int decisao;
    int auxDecisao;
    int id;
    char assunto[20];

    noEnvia* noAux = malloc(sizeof(noEnvia));
    msgNode* aLer = malloc(sizeof(msgNode));
    struct sockaddr_in addr;
    struct hostent *hostPtr;

    if (argc != 3) {
    printf("cliente <host> <port> \n");
    exit(-1);
}

    strcpy(endServer, argv[1]);
    if ((hostPtr = gethostbyname(endServer)) == 0)
        erro("Nao consegui obter endereço");
    bzero((void *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short) atoi(argv[2]));
    if((fd = socket(AF_INET,SOCK_STREAM,0)) == -1)
        erro("socket");
    if( connect(fd,(struct sockaddr *)&addr,sizeof (addr)) < 0)
        erro("Connect");
label1:
    do{
        char aux[20];
        do{
            printf("Login: ");
            scanf("%s",aux);
            if(strlen(aux)>20)
                printf("Username nao valido, demasiado longo\n");
        }while(strlen(aux)>20);
        strcpy(noAux->username,aux);

        do{
            printf("Password: ");
            scanf("%s",aux);
            if(strlen(aux)>20)
                printf("Password nao valida, demasiado longa\n");
        }while(strlen(aux)>20);

        strcpy(noAux->password,aux);
        write(fd,noAux,sizeof(noEnvia));

        read(fd,&aceitacao,sizeof(int));

        if(aceitacao!=htonl(0)){

            printf("Quer registar-se? Sim - 0 ; Nao - 1\n");
            scanf("%d",&registra);
            if(registra==1)
            {
                enviaRegistra=htonl(1);
                write(fd,&enviaRegistra,sizeof(int));
                printf("Exit? YES=1 NO=0\n");
                scanf("%d",&ver);
                if(ver==0)
                    goto label1;
                else
                    goto Fim;
            }
        }
    }while(aceitacao!=0 && registra!=0);

label2:
    printf("Que pretende fazer? Escrever mensagens-1 ; Ler mensagens-2 ; Apagar Uma Mensagem-3 ; Sair-4\n");

    scanf("%d",&decisao);
    auxDecisao=htonl(decisao);;
    write(fd,&auxDecisao,sizeof(int));

    if(decisao==1){
        msgNode* msgEnvia=malloc(sizeof(msgNode));

        do{
            printf("Destinatário: ");
            scanf("%s",msgEnvia->destinatario);
        }while(strlen(msgEnvia->destinatario)>20);

        strcpy(msgEnvia->emissor,noAux->username);
        do{
            printf("Assunto: ");
            scanf("%s",msgEnvia->assunto);
        }while(strlen(msgEnvia->assunto)>20);

        do{
            printf("Mensagem: \n");
            scanf("%s",msgEnvia->msg);
            if(strlen(msgEnvia->msg)>100)
                printf("Mensagem demasiado grande.\n");
        }while(strlen(msgEnvia->msg)>100);

        msgEnvia->next=NULL;
        write(fd,msgEnvia,sizeof(msgNode));
        goto label2;
    }
    else if(decisao==2)
    {
        printf("Qual o ID da mensagem que quer ler? ");
        scanf("%d",&id);
        id=htonl(id);
        write(fd,&id,sizeof(int));
        read(fd,&id,sizeof(int));
        if(ntohl(id)==0)
            printf("Mensagem não existe\n");
        else{
            read(fd,aLer,sizeof(msgNode));
        }
        if(aLer!=NULL)
            imprimeMsg(aLer,ntohl(id));
        else
            printf("A inbox esta vazia\n");
        goto label2;
    }
    else if(decisao==3)
    {
        printf("Que mensagem que apagar? ");
        scanf("%s",assunto);
        write(fd,assunto,sizeof(assunto[20]));
    }

    printf("\nExit? YES=1 NO=0\n");
    scanf("%d",&ver);
    if(ver==0)
        goto label2;

Fim:
    close(fd);
    exit(0);
}
void erro(char *msg)
{
    printf("Erro: %s\n", msg);
    exit(-1);
}
void imprimeMsg(listaM m,int id) /*Imprime a mensagem numero id de um utilizador;*/
{
    listaM temp=m;
    printf("Mensagem numero %d;\nEmissor: %s\nAssunto: %s\nMensagem: %s\n",id,temp->emissor,temp->assunto,temp->msg);
}
