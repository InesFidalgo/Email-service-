#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include "cabeca.h"
#define SERVER_PORT 9002
#define BUF_SIZE 1024


void process_client(int fd);
void erro(char *msg);
int procuraFicheiro(userNode* util,FILE* user,FILE* pass);
int recebeDados(userNode* util,int n);

void escreveFicheiroU(char* nomeFich,listaU l);
listaU leFicheiroU(char* nomeFich);

void escreveFicheiroMsg(listaU l);
void escreveFicheiroMsgUser (listaU u);
listaM leFicheiroMsg(char* nomeFich);
void leFicheiroMsgUsers(listaU l);

listaU insereListaU(listaU p,char* nome, char* pass);
void printListaU(listaU p);
listaU procuraUser(listaU p,char* username);
void apagaInbox(listaU user);
listaM apagaMsg(listaU user,char* assunto);
void insereMsg(listaU user,char* dest,char* msg,char* assunto);
void imprimeMsgsUser(listaU u);
int contaMsg(listaU u);

listaU allusers;

int main() {
    int fd, client;
    struct sockaddr_in addr, client_addr;
    int client_addr_size;

    allusers=leFicheiroU("utilizadores1.usr");
    leFicheiroMsgUsers(allusers);

    bzero((void *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(SERVER_PORT);

    if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        erro("na funcao socket");
    if ( bind(fd,(struct sockaddr*)&addr,sizeof(addr)) < 0)
        erro("na funcao bind");
    if( listen(fd, 5) < 0)
        erro("na funcao listen");

    while (1) {
        client_addr_size = sizeof(client_addr);
        client = accept(fd,(struct sockaddr *)&client_addr,(socklen_t *__restrict)&client_addr_size);
        if (client > 0) {
            if (fork() == 0) {
            close(fd);
            process_client(client);
            exit(0);
            }
        close(client);
        }
    }
    return 0;
}

void process_client(int client_fd)
{
    int registra=1;
    int resposta; /*vai mandar ao cliente a informação se foi aceite ou não*/
    int master; /*vai receber do cliente o que ele quer fazer: escrever, ler, apagar msg, apagar inbox ou sair*/
    int idMsg; /*vai receber o ID da msg que o cliente quer ler*/
    int zero=htonl(0); /*variável auxiliar*/
    char assunto[20];
    int i;


    listaU atual=malloc(sizeof(userNode));
    listaU destino=malloc(sizeof(userNode));
    noEnvia* recebe=malloc(sizeof(noEnvia));
    listaM ajuda;
    msgNode* mensg=malloc(sizeof(msgNode));

label:

    read(client_fd,recebe, sizeof(noEnvia));
    if((atual=procuraUser(allusers,recebe->username))!=NULL && strcmp(atual->password,recebe->password)==0)
    {
        resposta=htonl(0);
        write(client_fd,&resposta,sizeof(int));

        goto labelMaster;
    }
    else{
        resposta=htonl(1);
        write(client_fd,&resposta,sizeof(int));

        read(client_fd,&registra,sizeof(int));

        if(registra==htonl(0)){
            insereListaU(allusers,recebe->username,recebe->password);
            escreveFicheiroU("utilizadores1.usr",allusers);
            atual=procuraUser(allusers,recebe->username);
        }
        else
            goto label;
    }
labelMaster:
    /*o cliente quer escrever*/
    read(client_fd,&master,sizeof(int));
   if(master==htonl(1)){
        read(client_fd,mensg,sizeof(msgNode));
        destino=procuraUser(allusers,mensg->destinatario);
        if (destino!=NULL){
            insereMsg(destino,atual->username,mensg->msg,mensg->assunto);
            escreveFicheiroMsgUser(destino);
            imprimeMsgsUser(destino);
            printf("Mensagem escrita com sucesso\n");
        }
        goto labelMaster;
    }
    /*o cliente quer ler*/
    else if(master==htonl(2)){
        read(client_fd,&idMsg,sizeof(int));

        if(ntohl(idMsg)>contaMsg(atual))
            write(client_fd,&zero,sizeof(int));
        else
        {
            ajuda=atual->inbox;
            for(i=1;i<ntohl(idMsg);i++)
            {
                ajuda=ajuda->next;
            }
            mensg=ajuda;
            write(client_fd,&idMsg,sizeof(int));
            write(client_fd,mensg,sizeof(msgNode));
            printf("Mensagem lida com sucesso\n");
        }
        goto labelMaster;
    }
    /*o cliente que apagar uma mensagem*/
    else if(master==htonl(3))
    {
        read(client_fd,assunto,sizeof(char[20]));
        apagaMsg(atual,assunto);
        printf("Mensagem apagada com sucesso\n");
        goto labelMaster;
    }
    else if(master==htonl(4))
    {
        escreveFicheiroMsg(allusers);
        escreveFicheiroU("utilizadores1.usr",allusers);
        goto label;
    }

    fflush(stdout);
    close(client_fd);
}

void erro(char *msg)
{

    printf("Erro: %s\n", msg);
    exit(-1);
}



/*###########USERS############*/
void printListaU(listaU p)
{
    while(p!=NULL)
    {
        printf("%s %s\n",p->username,p->password);
        p=p->next;
    }
}

listaU procuraUser(listaU p,char* username)
{
    while(p!=NULL && strcmp(p->username,username)!=0)
    {
        p=p->next;
    }
    return p; /*caso exista retorna o ponteiro para o user. Senão, devolve NULL*/
}


listaU insereListaU(listaU p,char* nome,char* pass)
{
    listaU user=malloc(sizeof(userNode));
    strcpy(user->username,nome);
    strcpy(user->password,pass);
    user->inbox=NULL;
    user->next=p;
    p=user;
    return p;
}

void escreveFicheiroU(char* nomeFich,listaU l)
{
    listaU temp=l;
    FILE *fp=fopen(nomeFich,"wb");
    if(fp!=NULL)
    {
        while(temp!=NULL)
        {
            fwrite(temp,sizeof(userNode),1,fp); /*escreve no ficheiro cada nó de utilizador (1 a 1)*/
            temp=temp->next;
        }
    }
    fclose(fp);
}

listaU leFicheiroU(char* nomeFich)
{
    listaU lista=NULL;
    listaU temp=NULL;
    listaU ult=NULL;
    int cnt;

    FILE *fp = fopen (nomeFich, "rb");
    if (fp!=NULL){
        while (!feof(fp)){
            temp=malloc(sizeof(userNode));
            cnt=fread((void*)temp,sizeof(userNode),1,fp);
            if (cnt!=1)
                break;
            if (lista==NULL){
                lista=temp;
                lista->next=NULL;
                ult=lista;
            }
            else {
                temp->next=NULL;
                ult->next=temp;
                ult=temp;
           }
        }
        fclose(fp);
    }
    return lista;
}




/*#############MSG###########*/

void apagaInbox(listaU user)
{
    user->inbox=NULL;
}

listaM apagaMsg(listaU user,char* assunto)
{
    listaU pont;
    listaM temp;
    pont=procuraUser(allusers,user->username);
    temp=pont->inbox;
    while(temp!=NULL && strcmp(temp->assunto,assunto)!=0)
    {
        temp=temp->next;
    }
    return temp; /*caso exista retorna o ponteiro para o user. Senão, devolve NULL*/

}

void insereMsg(listaU user,char* emissor,char* msg,char* assunto)
{
    listaM mensg=malloc(sizeof(msgNode));
    strcpy(mensg->destinatario,user->username);
    strcpy(mensg->emissor,emissor);
    strcpy(mensg->msg,msg);
    strcpy(mensg->assunto,assunto);
    mensg->next=user->inbox;
    user->inbox=mensg;
}

void imprimeMsgsUser(listaU u) /*Imprime mensagens de um utilizador;*/
{
    listaU pont=u;
    listaM temp;
    int i=0;
    if(pont!=NULL)
    {
        temp=pont->inbox;
        if(temp==NULL)
            printf("A inbox esta vazia");
        while(temp!=NULL)
        {
            i++;
            printf("Mensagem numero %d, Assunto: %s\n",i,temp->assunto);
            temp=temp->next;
        }
    }
    else
        printf("Nao foi encontrado utilizador.");
}

int contaMsg(listaU u)
{
    listaU pont=u;
    listaM temp;
    int i=0;
    if(pont!=NULL)
    {
        temp=pont->inbox;
        if(temp==NULL)
            return i;
        while(temp!=NULL)
        {
            i++;
            temp=temp->next;
        }
    }
    return i;
}

void leFicheiroMsgUsers(listaU l){
    char aux[20];

    while (l != NULL){      /*Para cada utilizador*/
        strcpy(aux,l->username);
        l->inbox=leFicheiroMsg(strcat(aux,".msg"));
        l=l->next;
    }
}

listaM leFicheiroMsg(char* nomeFich)
{
    listaM lista=NULL;
    listaM temp=NULL;
    listaM ult=NULL;
    int cnt;

    FILE *fp = fopen (nomeFich, "rb");

    if (fp!=NULL){
        while (!feof(fp)){
            temp=malloc(sizeof(msgNode));
            cnt=fread((void*)temp,sizeof(userNode),1,fp);
            if (cnt!=1)
                break;
            if (lista==NULL){
                lista=temp;
                lista->next=NULL;
                ult=lista;
            }
            else {
                temp->next=NULL;
                ult->next=temp;
                ult=temp;
            }
        }
        fclose(fp);
    }
    return lista;
}
void escreveFicheiroMsg(listaU l){
    while (l != NULL){                  /*Para cada utilizador*/
        escreveFicheiroMsgUser(l);
        l=l->next;
    }
}

//Escreve Mensagens de 1 utilizador no ficheiro
void escreveFicheiroMsgUser (listaU u){
    listaM inb;
    char aux[20];
    strcpy(aux,u->username);
    FILE *fp=fopen(strcat(aux,".msg"),"wb"); /*Cria o ficheiro de mensagens do utilizador*/
    if(fp!=NULL)
    {
        inb=u->inbox;
        while(inb!=NULL)
        {
            fwrite(inb,sizeof(msgNode),1,fp); /*escreve no ficheiro cada no de mensagem (1 a 1)*/
            inb=inb->next;
        }
    }
    fclose(fp);
}
