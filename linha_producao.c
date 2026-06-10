#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct atividade //fila encadeada simples para as atividades
{
    char nome[50];
    float falha;
    int duracao;
    int cap_ativ;
    int tem_produto;
    struct atividade *next;
} atividade;

typedef struct fila_atividades //estrutura para manusear atividades
{
    atividade *start;
    atividade *end;
} fila_atividades;

typedef struct etapa //fila encadeada dupla para as etapas
{
    int id;
    char nome[100];
    int cap_eta;
    int tem_produto;
    fila_atividades atividades;
    struct etapa *next;
    struct etapa *prev;
} etapa;

typedef struct log_produto //estrutura para registrar os eventos de cada produto
{
    char registro[150];
    struct log_produto *prox;
} log_produto;

typedef struct produto //estrutura para guardar os produtos
{
    int id;
    etapa *etapa_atual;
    atividade *ativ_atual;
    int criacao;
    int restante;
    int status;
    log_produto *log_comeco;
    log_produto *log_fim;
} produto;

typedef struct ativos //fila de registro central pra saber quais produtos estão atualmente rodando na esteira
{
    produto *prod;
    struct ativos *next;
} ativos;

typedef struct produtos_log_tree //arvore de logs pra salvar no arquivo
{
    produto *prod;
    struct produtos_log_tree *left;
    struct produtos_log_tree *right;
} produtos_log_tree;

void inserir_atividade(fila_atividades *fila, const char *nome, float falha, int duracao, int cap)
{
    atividade *novo = (atividade *)malloc(sizeof(atividade));
    strcpy(novo->nome, nome);
    novo->falha = falha;
    novo->duracao = duracao;
    novo->cap_ativ = cap;
    novo->tem_produto = 0;
    novo->next = NULL;

    if(!(fila->start))
    {
        fila->start = novo;
        fila->end = novo;
    }
    else
    {
        fila->end->next = novo;
        fila->end = novo;
    }
}

void inserir_etapa(etapa **head, int id, char *nome, int cap)
{
    etapa *novo = (etapa *)malloc(sizeof(etapa));
    strcpy(novo->nome, nome);
    novo->id = id;
    novo->cap_eta = cap;
    novo->tem_produto = 0;
    novo->atividades.start = NULL;
    novo->atividades.end = NULL;
    novo->next = NULL;
    novo->prev = NULL;

    if(!(*head))
    {
        *head = novo;
    }
    else
    {
        etapa *aux = *head;
        while(aux->next)
        {
            aux = aux->next;
        }

        aux->next = novo;
        novo->prev = aux;
    }
}

void inserir_ativos(ativos **head, produto *p)
{
    ativos *novo = (ativos *)malloc(sizeof(ativos));
    novo->prod = p;
    novo->next = *head;
    (*head) = novo;
}

void remover_ativos(ativos **head, int id)
{
    ativos *aux = *head, *ant = NULL;
    while(aux && aux->prod->id != id)
    {
        ant = aux;
        aux = aux->next;
    }

    if(!(aux))
        {if(!(ant))
        {
            (*head) = aux->next;
            free(aux);
        }
        else
        {
            ant->next = aux->next;
            free(aux);
        }
        free(aux);
    }
}

produtos_log_tree *inserir_arvore(produtos_log_tree *raiz, produto *p)
{
    if(!(raiz))
    {
        produtos_log_tree *novo = (produtos_log_tree *)malloc(sizeof(produtos_log_tree));
        novo->prod = p;
        novo->left = NULL;
        novo->right = NULL;
        return novo;
    }

    if(p->criacao < raiz->prod->criacao)
    {
        raiz->left = inserir_arvore(raiz->left, p);
    } 
    else
    {
        raiz->right = inserir_arvore(raiz->right, p);
    }

    return raiz;
}

void imprimir_relatorio(produtos_log_tree *raiz) // Algoritmo In-order para ler e imprimir árvore
{
    if(!raiz) return;
    
    imprimir_relatorio(raiz->left);
    printf("Id: %d\n", raiz->prod->id);
    imprimir_relatorio(raiz->right);
}

void main()
{
    etapa *fila_etapas = NULL;
    atividade *fila_atividades = NULL;
    ativos *fila_ativos = NULL;
    produtos_log_tree *arvore_produtos = NULL;

    inserir_etapa(&fila_etapas, 1, "Criacao", 5);
    inserir_atividade(&(fila_etapas->atividades), "Material", 0.1, 2, 3);
    inserir_atividade(&(fila_etapas->atividades), "Linha", 0.1, 3, 2);

    int tempo = 100;
    for(int i = 0; i < 100; i++)
    {
        //O produto é criado
        produto *novo_produto = (produto *)malloc(sizeof(produto));
        novo_produto->id = i;
        novo_produto->criacao = i;
        novo_produto->ativ_atual = fila_etapas->atividades.start;
        novo_produto->etapa_atual = fila_etapas;
        novo_produto->restante = novo_produto->ativ_atual->duracao - i;

        inserir_ativos(&fila_ativos, novo_produto);

        //O produto é processado
        ativos *aux = fila_ativos;
        while(aux)
        {
            ativos *failsafe = aux->next; //Ponteiro pra garantia (Perda de memória)
            
            (aux->prod->restante)--; //O tempo avança

            if(!(aux->prod->restante))
            {
                if(aux->prod->ativ_atual->next) //Avança a atividade e atualiza o tempo
                {
                    aux->prod->ativ_atual = aux->prod->ativ_atual->next;
                    aux->prod->restante = aux->prod->ativ_atual->duracao;
                } 
                else 
                {
                    if(aux->prod->etapa_atual->next) //Verifica se existe uma próxima etapa
                    {
                        aux->prod->etapa_atual = aux->prod->etapa_atual; //O produto avança para a nova etapa
                        aux->prod->ativ_atual = aux->prod->etapa_atual->atividades.start; //O produto entra na primeira atividade dessa nova etapa
                        aux->prod->restante = aux->prod->ativ_atual->duracao; //Cronômetro copia o tempo dessa primeira atividade
                    }
                    else
                    {
                        //Final: O produto sai da esteira e vai pra arvore
                        arvore_produtos = inserir_arvore(arvore_produtos, aux->prod);
                        remover_ativos(&fila_ativos, aux->prod->id);
                    }
                }
            }
            aux = failsafe;
        }
    }

    imprimir_relatorio(arvore_produtos);
}