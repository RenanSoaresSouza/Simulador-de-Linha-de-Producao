#include "utils.h"
#include "etapas.h"
#include "fila.h"
#include "ativos.h"
#include "produtos.h"
#include "descarte.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void *xmalloc(size_t size) {
    void *p = malloc(size);

    if (p == NULL) {
        printf("Erro de memoria\n");
        exit(EXIT_FAILURE);
    }

    return p;
}

void imprimir_painel(int tick_atual, etapa *fila_etapas, fila_entrada *f_entrada, ativos *fila_ativos) {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif

    printf("========================================================\n");
    printf("     FABRICA AO VIVO | TICK: %d \n", tick_atual);
    printf("========================================================\n");
    printf(" COMANDOS: [P] Pausar/Continuar | [S ou ESC] Pular pro Fim\n");
    printf("--------------------------------------------------------\n");

    int qtd_entrada = 0;
    no_fila *aux_f = f_entrada->start;
    while(aux_f) { qtd_entrada++; aux_f = aux_f->next; }
    printf(" [Caixa de Entrada]: %d produtos aguardando vaga\n\n", qtd_entrada);

    etapa *aux_e = fila_etapas;
    while(aux_e) {
        printf(" [%s] (Ocupacao: %d/%d)\n", aux_e->nome, aux_e->ocupacao_atual, aux_e->cap_eta);
        
        atividade *aux_a = aux_e->atividades.start;
        while(aux_a) {
            printf("    -> %s:\n", aux_a->nome);
            
            int encontrou = 0;
            ativos *aux_atv = fila_ativos;
            while(aux_atv) {
                produto *p = aux_atv->prod;
                if(p->etapa_atual->id == aux_e->id && p->ativ_atual->id == aux_a->id) {
                    if (p->restante == 0) {
                        printf("       - Produto #%d (AGUARDANDO GARGALO)\n", p->id);
                    } else {
                        printf("       - Produto #%d (Resta: %ds)\n", p->id, p->restante);
                    }
                    encontrou = 1;
                }
                aux_atv = aux_atv->next;
            }
            if(!encontrou) printf("       (vazio)\n");
            
            aux_a = aux_a->next;
        }
        printf("\n");
        aux_e = aux_e->next;
    }
    
    printf("--------------------------------------------------------\n");
    printf("   Concluidos: %d   |   Descartados (Mortos): %d\n", total_concluidos_sistema, total_falhas_sistema);
    printf("========================================================\n");
}

// ==========================================
// 4. ENTRADA DE CONFIGURAÇÃO (PARSER)
// ==========================================
void carregar_arquivo(char *nome_file, etapa **fila_etapas, int *qtd_total, int *vazao, char *nome, int *semente, int *duracao) {
    FILE *file = fopen(nome_file, "r");
    if (!file) { printf("Erro ao abrir o arquivo %s\n", nome_file); exit(1); }

    char string[50];
    etapa *etapa_atual = NULL;

    while (fscanf(file, "%s", string) != EOF) {
        if (!(strcmp(string, "SIMULACAO"))) {
            char nome_simulacao[50];
            fscanf(file, "%s %d %d", nome_simulacao, semente, duracao);
            srand(*semente);
        } else if (!(strcmp(string, "PRODUTOS"))) {
            fscanf(file, "%d %d %s", qtd_total, vazao, nome);
        } else if (!(strcmp(string, "ETAPA"))) {
            int id, atividades, cap;
            float falha_ini;
            char nome_etapa[50];
            fscanf(file, "%d %d %d %f %s", &id, &atividades, &cap, &falha_ini, nome_etapa);

            inserir_etapa(fila_etapas, id, cap, falha_ini, nome_etapa, atividades);
            etapa *aux_busca = *fila_etapas;
            while (aux_busca->next) aux_busca = aux_busca->next;
            etapa_atual = aux_busca;
        } else if (!(strcmp(string, "ATIVIDADE"))) {
            int id, duracao_at;
            float chance;
            char nome_atividade[50];
            fscanf(file, "%d %d %f %s", &id, &duracao_at, &chance, nome_atividade);

            if (etapa_atual) inserir_atividade(&(etapa_atual->atividades), nome_atividade, chance, duracao_at, id);
        }
    }
    fclose(file);
}