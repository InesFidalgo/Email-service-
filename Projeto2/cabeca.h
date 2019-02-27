
typedef struct mensagem *listaM;
typedef struct mensagem{/*tem de ser vetores e n�o ponteiros para char porque se o ficheiro guardar ponteiros d� crash*/
	char destinatario[20];
	char emissor[20];
	char assunto[20];
	char msg[100];
	listaM next;
}msgNode;/*lista ligada de mensagens de um utilizador (inbox)*/

typedef struct utilizador *listaU;
typedef struct utilizador
{
	char username[20];
	char password[20];
	listaM inbox;
	listaU next;
}userNode;/*lista ligada composta por todos os utilizadores com acesso � inbox de cada um*/

typedef struct envia{
    char username[20];
    char password[20];
}noEnvia; /*n� que serve apenas para enviar o utilizador e a pass para o servidor*/
