#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct ativ
{
    int cap;
    struct ativ *prox;
} ativ;

typedef struct produto
{
    int id;
    char nome[100]; 
} produto;

typedef struct fila {
    int vazao;
    produto *p;
    struct fila *prox;
    struct fila *ant;
}fila;

typedef struct simulacao{
    char nome[100];
    int semente;
    int limite_tempo;
    int quant;
    int taxa;
    char nome_prod[50];

}simulacao;
typedef struct etapa
{
    fila **f;
    char nome[100];
    int id;
    int tam;
    int taxa_falha;
    int cap;
    struct etapa *prox;
    struct etapa *ant;
} etapa;

void inserir_etapa(etapa **head, char nome[100], int id, int cap,int tam,int taxa_falha);

void load_file(char file[50],simulacao **s,etapa **head){
    char string[100];
    FILE *f = fopen(file,"r");

    if (f == NULL){
        printf("Não foi possivel abrir o arquivo");
        return;
    }
    while(fgets(string,100,f)){
        char* str = strtok(string, " ");
        if (!str) continue;
        
        if (strcmp(str,"SIMULACAO") == 0 ){
            for(int i=0;i<3;i++){
                // adiciona os valores relacionados a SIMULACAO do arquivo para a struct simulacao
                if (i == 0 ) strcpy((*s)->nome,strtok(NULL," \n")); 
                if (i == 1 ) (*s)->semente = atoi(strtok(NULL," \n")); 
                if (i == 2 ) (*s)->limite_tempo =atoi(strtok(NULL," \n")); 
            }
        }
        else if (strcmp(str,"PRODUTOS") == 0){
            for (int i =0;i<3;i++){
                // adiciona os valores relacionados a PRODUTOS do arquivo para a struct simulacao
                if (i == 0) (*s)->quant = atoi(strtok(NULL," \n")); 
                if (i == 1) (*s)->taxa = atoi(strtok(NULL," \n")); 
                if (i == 2) strcpy((*s)->nome_prod,strtok(NULL," \n"));
            }
        }
        else if (strcmp(str,"ETAPA") == 0){
            char *id = strtok(NULL," \n");
            char *quant = strtok(NULL," \n");
            char *cap = strtok(NULL," \n");
            char *taxa = strtok(NULL," \n");
            char *nome = strtok(NULL," \n");
            inserir_etapa(head,nome,atoi(id),atoi(cap),atoi(quant),atoi(taxa));

        }
    }
    
    fclose(f);
}

//------------------------------------------
//FILA DE PRODUTOS
//------------------------------------------

//Inserir na fila
void put_fila(fila **f,produto *p){
    fila *aux =(fila*)malloc(sizeof(fila));
    aux->prox = NULL;
    aux->ant = NULL;
    aux->p = p;
    if (!*f){
        *f = aux;
        return;
    }
    (*f)->ant = aux;
    aux->prox = *f;
    *f = aux; 
}
//Mostrar na Fila
void show_fila(fila **f){
    fila *aux = *f;
    while(aux){
        printf("----\n id:%d \n nome:%s\n---- ",aux->p->id,aux->p->nome);
        aux = aux->prox;
    }
}
//Apagar da fila (retorna o item apagado)
produto *pop_fila(fila **f){
    fila *aux = *f;
    if (*f == NULL){
        return NULL; // erro por tentar deletar elementos  de uma fila vazia
    }
    if((*f)->prox == NULL){
        produto *p = aux->p;
        *f = NULL;
        free(aux);
        return p;
    }

    while(aux && aux->prox){
        aux = aux->prox;
    }
    produto *p = aux->p;
    aux->ant->prox = NULL;
    aux->ant = NULL;
    free(aux);
    return p;
}          

void inserir_etapa(etapa **head, char nome[100], int id, int cap,int tam,int taxa_falha)
{
    //-----------------------
    // Essa função gasta O(n), mas é possivel reduzir para constante
    //-----------------------
    
    //CRIA A ETAPA
    etapa *novo = (etapa*)malloc(sizeof(etapa));
    strcpy(novo->nome, nome);
    novo->tam = tam;
    novo->taxa_falha = taxa_falha;
    novo->prox = NULL;
    novo->ant = NULL;
    novo->id = id;
    novo->cap = cap;
    novo->f = NULL;
    etapa *temp = *head;
    //INSERE A ETAPA
    if(!*head){
        *head = novo;
        return;
    }
    while (temp && temp->prox){
        temp = temp->prox;
    }
    temp->prox = novo;
    novo->ant = temp;

}
void show_etapa(etapa **head){
    etapa *aux = *head;
    while (aux){
        printf("----\nETAPA:%d\nnome:%s\ncapacidade:%d\n----",aux->id,aux->nome,aux->cap);
        aux = aux->prox;
    }
}

int main()
{
    etapa *e = NULL;
    simulacao *s = malloc(sizeof(simulacao));
    load_file("input.txt",&s,&e);
    show_etapa(&e);
    free(s);
    return 0;

}