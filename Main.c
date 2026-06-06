#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


typedef struct produto
{
    int id;
    char nome[100]; 
} produto;
//------------------------------------------
//FILA DE PRODUTOS
//------------------------------------------
typedef struct fila {
    int vazao;
    produto *p;
    struct fila *prox;
    struct fila *ant;
}fila;

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


typedef struct etapa
{
    fila **f;
    char nome[100];
    int id;
    int tam;
    int cap;
    struct etapa *prox;
    struct etapa *ant;
} etapa;

typedef struct ativ
{
    int cap;
    struct ativ *prox;
} ativ;


void inserir_etapa(etapa **head, char nome[100], int id, int cap)
{
    //-----------------------
    // Essa função gasta O(n), mas é possivel reduzir para constante
    //-----------------------
    
    //CRIA A ETAPA
    etapa *novo = (etapa*)malloc(sizeof(etapa));
    strcpy(novo->nome, nome);
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

void main()
{
    etapa *e1 = NULL;

    inserir_etapa(&e1,"Construcao",1,10);
    inserir_etapa(&e1,"acabamento",2,5);
    show_etapa(&e1);

}