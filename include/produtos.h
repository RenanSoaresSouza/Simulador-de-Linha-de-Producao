
#ifndef SIM_PRODUTO
#define SIM_PRODUTO

typedef struct etapa etapa;
typedef struct atividade atividade;
typedef struct no_pilha no_pilha;
typedef struct ativos ativos;

typedef struct produto {
    int id;
    int tick_saida;
    etapa *etapa_atual;
    atividade *ativ_atual;
    int criacao;          
    int entrada_linha;    
    int restante;         
    int falhas_totais;    
    int tempo_espera;     
    int tick_entrou_na_fila_da_atividade;
    int tick_entrada_etapa; 
    int tentativa_atual;    
    no_pilha *historico;  
} produto;

void liberar_produtos(ativos *lista);

#endif