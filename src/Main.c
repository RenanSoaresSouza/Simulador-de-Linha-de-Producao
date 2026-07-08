#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "atividades.h"
#include "etapas.h"
#include "pilha.h"
#include "utils.h"
#include "produtos.h"
#include "ativos.h"
#include "fila.h"
#include "descarte.h"
#include "arvore.h"
#include "utils.h"

//bibliotecas usadas em windows/linux dependendo do OS
#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
#else
    #include <unistd.h>
    #include <termios.h>
    #include <fcntl.h>
    
    // código para ler o teminal no linux e Mac sem travar o terminal
    int kbhit(void) {
        struct termios oldt, newt;
        int ch;
        int oldf;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, oldf);
        if(ch != EOF) {
            ungetc(ch, stdin);
            return 1;
        }
        return 0;
    }
    
    char getch(void) {
        char ch;
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ch;
    }
#endif

no_descarte *pilha_descarte_global = NULL;
int total_falhas_sistema = 0;
int total_concluidos_sistema = 0;

// ==========================================
// 5. DA_MAIN_E_EXECUÇÃO_DO_TEMPO
// ==========================================
int main() {
    etapa *fila_etapas = NULL;
    ativos *fila_ativos = NULL;
    ativos *fila_concluidos = NULL; 
    produtos_log_tree *arvore_produtos = NULL;
    fila_entrada f_entrada = {NULL, NULL};

    int qtd_produtos = 0, taxa_vazao = 0, semente = 0, duracao_simulacao = 0;
    char nome_produto[50] = "";
    char arquivo[50];
    int produtos_criados_total = 0;

    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
    
    printf("======================================\n");
    printf("   BEM-VINDO AO SIMULADOR INDUSTRIAL  \n");
    printf("======================================\n");
    printf("Digite o nome do arquivo de entrada (ex: input.txt): ");
    scanf("%s", arquivo);
    carregar_arquivo(arquivo, &fila_etapas, &qtd_produtos, &taxa_vazao, nome_produto, &semente, &duracao_simulacao);
    
    printf("\nCarregando planta da fabrica...\n");
    #ifdef _WIN32
        Sleep(1500); 
    #else
        usleep(1500000);
    #endif

    int tick_final = duracao_simulacao; 
    int painel_ativo = 1; // Controla se o dashboard e os 'Sleeps' estão ligados

    // Loop do Pulso Temporal (Clock Ativo de 1s)
    for (int i = 0; i < duracao_simulacao; i++) {
        
        // 1. Ingestão na Fila FIFO de Entrada
        for (int k = 0; k < taxa_vazao && produtos_criados_total < qtd_produtos; k++) {
            if (fila_etapas == NULL || fila_etapas->atividades.start == NULL) {
                printf("Erro: nenhuma etapa ou atividade cadastrada.\n");
                return 1;
            }
            produto *novo_produto = xmalloc(sizeof(produto));
            novo_produto->id = produtos_criados_total++;
            novo_produto->criacao = i;
            novo_produto->entrada_linha = -1;
            novo_produto->falhas_totais = 0;
            novo_produto->tempo_espera = 0;
            novo_produto->historico = NULL;
            novo_produto->etapa_atual = fila_etapas;
            novo_produto->ativ_atual = fila_etapas->atividades.start;
            novo_produto->restante = novo_produto->ativ_atual->duracao;
            novo_produto->tick_entrou_na_fila_da_atividade = i;
            novo_produto->tick_entrada_etapa = -1;
            novo_produto->tentativa_atual = 1;

            enqueue_entrada(&f_entrada, novo_produto);
        }

        // 2. Transferência para a Linha
        while (f_entrada.start && fila_etapas->ocupacao_atual < fila_etapas->cap_eta) {
            produto *p = dequeue_entrada(&f_entrada);
            p->entrada_linha = i;
            p->tick_entrou_na_fila_da_atividade = i;
            p->tick_entrada_etapa = i; 
            fila_etapas->ocupacao_atual++;
            arvore_produtos = inserir_arvore(arvore_produtos, p); 
            inserir_ativos(&fila_ativos, p);
        }

        // 3. Processamento na esteira
        ativos *aux = fila_ativos;
        while (aux) {
            ativos *failsafe = aux->next;
            produto *p = aux->prod;

            if (p->restante == 0) {
                if (!p->ativ_atual->next && p->etapa_atual->next && p->etapa_atual->next->ocupacao_atual >= p->etapa_atual->next->cap_eta) {
                    p->tempo_espera++;
                    aux = failsafe;
                    continue;
                }
            }
            if (p->restante > 0) p->restante--;

            if (p->restante == 0) {
                float r = (float)rand() / RAND_MAX;

                if (r < (p->ativ_atual->falha * 0.15f)) {
                    total_falhas_sistema++;
                    p->etapa_atual->total_falhas++;
                    p->etapa_atual->ocupacao_atual--;
                    arvore_produtos = remover_arvore(arvore_produtos, p->id);
                    remover_ativos(&fila_ativos, p->id);
                    push_descarte(p); 
                } else if (r < p->ativ_atual->falha) {
                    total_falhas_sistema++;
                    p->falhas_totais++;
                    p->etapa_atual->total_falhas++;
                    
                    int inicio_real = (i - p->ativ_atual->duracao) + 1;
                    push_historico(p, p->ativ_atual, p->etapa_atual, p->tick_entrou_na_fila_da_atividade, inicio_real, i, 0);
                    
                    p->ativ_atual = p->etapa_atual->atividades.start;
                    p->restante = p->ativ_atual->duracao;
                    p->tick_entrou_na_fila_da_atividade = i;
                    p->tentativa_atual++; 

                    while (p->historico && p->historico->status_resultado == 1) pop_historico(p);
                } else {
                    int inicio_real = (i - p->ativ_atual->duracao) + 1;
                    int tempo_fila = inicio_real - p->tick_entrou_na_fila_da_atividade;
                    if (tempo_fila < 0) tempo_fila = 0; 

                    p->ativ_atual->tempo_total_fila += tempo_fila;
                    p->etapa_atual->tempo_total_fila += tempo_fila;
                    p->ativ_atual->tempo_total_execucao += p->ativ_atual->duracao;
                    p->ativ_atual->itens_processados++;

                    push_historico(p, p->ativ_atual, p->etapa_atual, p->tick_entrou_na_fila_da_atividade, inicio_real, i, 1);

                    if (p->ativ_atual->next) {
                        p->ativ_atual = p->ativ_atual->next;
                        p->restante = p->ativ_atual->duracao;
                        p->tick_entrou_na_fila_da_atividade = i;
                    } else {
                        int tempo_gasto_na_etapa = i - p->tick_entrada_etapa;
                        p->etapa_atual->tempo_total_gasto += tempo_gasto_na_etapa;
                        p->etapa_atual->produtos_processados++;
                        if(tempo_gasto_na_etapa < p->etapa_atual->tempo_minimo) p->etapa_atual->tempo_minimo = tempo_gasto_na_etapa;
                        if(tempo_gasto_na_etapa > p->etapa_atual->tempo_maximo) p->etapa_atual->tempo_maximo = tempo_gasto_na_etapa;

                        if (p->etapa_atual->next) {
                            etapa *proxima = p->etapa_atual->next;
                            if (proxima->ocupacao_atual < proxima->cap_eta) {
                                float r_qualidade = (float)rand() / RAND_MAX;
                                if (r_qualidade < proxima->taxa_falha_ini) {
                                    total_falhas_sistema++;
                                    p->falhas_totais++;
                                    p->etapa_atual->total_falhas++;
                                    p->ativ_atual = p->etapa_atual->atividades.start;
                                    p->restante = p->ativ_atual->duracao;
                                    p->tick_entrou_na_fila_da_atividade = i;
                                    p->tentativa_atual++;
                                } else {
                                    p->etapa_atual->ocupacao_atual--;
                                    p->etapa_atual = proxima;
                                    p->etapa_atual->ocupacao_atual++;
                                    p->ativ_atual = p->etapa_atual->atividades.start;
                                    p->restante = p->ativ_atual->duracao;
                                    p->tick_entrou_na_fila_da_atividade = i;
                                    p->tick_entrada_etapa = i; 
                                    p->tentativa_atual = 1;    
                                }
                            }
                        } else {
                            p->tick_saida = i; 
                            total_concluidos_sistema++;
                            p->etapa_atual->ocupacao_atual--;
                            arvore_produtos = remover_arvore(arvore_produtos, p->id);
                            remover_ativos(&fila_ativos, p->id);
                            inserir_ativos(&fila_concluidos, p); 
                        }
                    }
                }
            }
            aux = failsafe;
        }

        // 4. Controles Visuais e de Teclado
        if (painel_ativo) {
            imprimir_painel(i, fila_etapas, &f_entrada, fila_ativos);

            if (kbhit()) {
                char tecla = getch();
                
                // Botão de PAUSAR
                if (tecla == 'p' || tecla == 'P') {
                    printf("\n [ SIMULACAO PAUSADA ] Pressione qualquer tecla para continuar...\n");
                    while(!kbhit()) { 
                        #ifdef _WIN32
                            Sleep(100);
                        #else
                            usleep(100000);
                        #endif
                    }
                    getch(); // limpa buffer
                } 
                // Botão de FAST-FORWARD (S ou ESC)
                else if (tecla == 's' || tecla == 'S' || tecla == 27) { 
                    painel_ativo = 0; // Desliga a tela!
                    printf("\n [ MODO FAST-FORWARD ] Tela desligada! Calculando o restante em velocidade maxima...\n");
                    #ifdef _WIN32
                        Sleep(1500); // Pausa de 1.5s só pro usuario ler a mensagem
                    #else
                        usleep(1500000);
                    #endif
                }
            }

            // O tempo real (agora de 1 segundo para visualização confortável)
            if (painel_ativo) {
                #ifdef _WIN32
                    Sleep(1000); 
                #else
                    usleep(1000000);
                #endif
            }
        }
    }

    // ==========================================
    // 6. GERAÇÃO DO RELATÓRIO FINAL 
    // ==========================================
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
    printf("\nGerando relatorio final de metricas...\n");

    char nome[256];
    int i = 1;

    FILE *arquivo_saida;

    do {
        sprintf(nome, "relatorios/relatorio_%03d.txt", i);
        arquivo_saida = fopen(nome, "r");

        if (arquivo_saida) {
            fclose(arquivo_saida);
            i++;
        }

    } while (arquivo_saida);

    arquivo_saida = fopen(nome, "w");

    if (!arquivo_saida) {
        printf("Erro ao gerar arquivo de saída!\n");
        liberar_etapas(fila_etapas);
        liberar_produtos(fila_ativos);
        liberar_produtos(fila_concluidos);
        liberar_arvore(arvore_produtos);
        liberar_descarte(pilha_descarte_global);
        liberar_fila_entrada(&f_entrada);
        return -1;
     }

    double tempo_medio_linha = 0.0;
    double tempo_medio_espera = 0.0;
    
    ativos *aux_calc = fila_concluidos;
    while(aux_calc) {
        tempo_medio_linha += (aux_calc->prod->tick_saida - aux_calc->prod->entrada_linha);
        tempo_medio_espera += aux_calc->prod->tempo_espera;
        aux_calc = aux_calc->next;
    }
    if (total_concluidos_sistema > 0) {
        tempo_medio_linha /= total_concluidos_sistema;
        tempo_medio_espera /= total_concluidos_sistema;
    }

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char timestamp[50];
    sprintf(timestamp, "%d%02d%02d_%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min);

    fprintf(arquivo_saida, "=== METADADOS ===\n");
    fprintf(arquivo_saida, "id_simulacao: simulacao_%s\n", timestamp);
    fprintf(arquivo_saida, "semente_utilizada: %d\n", semente);
    fprintf(arquivo_saida, "arquivo_entrada: %s\n", arquivo);
    fprintf(arquivo_saida, "tick_fim: %d\n", tick_final);
    fprintf(arquivo_saida, "produtos_concluidos: %d\n", total_concluidos_sistema);
    fprintf(arquivo_saida, "tempo_medio_linha: %.2f\n", tempo_medio_linha);
    fprintf(arquivo_saida, "tempo_medio_espera: %.2f\n", tempo_medio_espera);
    fprintf(arquivo_saida, "falhas_totais: %d\n", total_falhas_sistema);
    fprintf(arquivo_saida, "Meta alcancada: %s\n", total_concluidos_sistema >= qtd_produtos ? "sim" : "nao");
    fprintf(arquivo_saida, "Produtos faltantes: %d\n\n", qtd_produtos - total_concluidos_sistema);
    
    fprintf(arquivo_saida, "=== RELATORIO DA SIMULACAO %s ===\n", nome_produto);
    
    fprintf(arquivo_saida, "------- RELATORIO DAS ETAPAS -----------------\n");
    etapa *aux_etapa = fila_etapas;
    while(aux_etapa) {
        double falhas_por_prod = aux_etapa->produtos_processados > 0 ? (double)aux_etapa->total_falhas / aux_etapa->produtos_processados : 0.0;
        double tempo_med_etapa = aux_etapa->produtos_processados > 0 ? (double)aux_etapa->tempo_total_gasto / aux_etapa->produtos_processados : 0.0;
        double tempo_med_fila = aux_etapa->produtos_processados > 0 ? (double)aux_etapa->tempo_total_fila / aux_etapa->produtos_processados : 0.0;
        if(aux_etapa->tempo_minimo == 999999) aux_etapa->tempo_minimo = 0; 

        fprintf(arquivo_saida, "ETAPA %d %s:\n", aux_etapa->id, aux_etapa->nome);
        fprintf(arquivo_saida, "Atividades: %d\n", aux_etapa->quant_atv);
        fprintf(arquivo_saida, "Quantidade de falhas: %d\n", aux_etapa->total_falhas);
        fprintf(arquivo_saida, "Falhas por produto: %.2f\n", falhas_por_prod);
        fprintf(arquivo_saida, "Tempo minimo: %d\n", aux_etapa->tempo_minimo);
        fprintf(arquivo_saida, "Tempo medio: %.2f\n", tempo_med_etapa);
        fprintf(arquivo_saida, "Tempo maximo: %d\n", aux_etapa->tempo_maximo);
        fprintf(arquivo_saida, "Tempo medio em fila: %.2f\n\n", tempo_med_fila);
        aux_etapa = aux_etapa->next;
    }

    fprintf(arquivo_saida, "------- RELATORIO DAS ATIVIDADES--------------\n");
    aux_etapa = fila_etapas;
    while(aux_etapa) {
        fprintf(arquivo_saida, "ETAPA %d %s:\n", aux_etapa->id, aux_etapa->nome);
        atividade *aux_ativ = aux_etapa->atividades.start;
        while(aux_ativ) {
            double tempo_med_fila_ativ = aux_ativ->itens_processados > 0 ? (double)aux_ativ->tempo_total_fila / aux_ativ->itens_processados : 0.0;
            double tempo_med_total_ativ = aux_ativ->itens_processados > 0 ? (double)(aux_ativ->tempo_total_execucao + aux_ativ->tempo_total_fila) / aux_ativ->itens_processados : 0.0;

            fprintf(arquivo_saida, "ATIVIDADE %d %s:\n", aux_ativ->id, aux_ativ->nome);
            fprintf(arquivo_saida, "Capacidade: %d\n", aux_etapa->cap_eta);
            fprintf(arquivo_saida, "Vazao: %d\n", aux_ativ->itens_processados); 
            fprintf(arquivo_saida, "Tempo de execucao: %d\n", aux_ativ->duracao);
            fprintf(arquivo_saida, "Tempo medio em fila: %.2f\n", tempo_med_fila_ativ);
            fprintf(arquivo_saida, "Tempo medio total: %.2f\n\n", tempo_med_total_ativ);
            aux_ativ = aux_ativ->next;
        }
        aux_etapa = aux_etapa->next;
    }

    fprintf(arquivo_saida, "------- RELATORIO DOS PRODUTOS ----------------\n");
    ativos *aux_prod = fila_concluidos;
    while (aux_prod) {
        produto *p = aux_prod->prod;
        fprintf(arquivo_saida, "--- PRODUTO %d ---\n", p->id);
        fprintf(arquivo_saida, "Modelo: %s\n", nome_produto);
        fprintf(arquivo_saida, "Criacao: tick %d\n", p->criacao);
        fprintf(arquivo_saida, "Entrada na linha: tick %d\n", p->entrada_linha);
        
        fprintf(arquivo_saida, "Saida da linha: tick %d\n", p->tick_saida);
        fprintf(arquivo_saida, "Tempo total: %d ticks\n", (p->tick_saida - p->entrada_linha));
        fprintf(arquivo_saida, "Tempo em espera: %d ticks\n", p->tempo_espera);
        fprintf(arquivo_saida, "Falhas: %d\n", p->falhas_totais);
        fprintf(arquivo_saida, "Trajetoria:\n");

        p->historico = inverter_historico(p->historico);
        no_pilha *hist = p->historico;
        int etapa_memoria = -1;
        int tentativa_memoria = -1;
        int tick_inicio_etapa = -1;

        while (hist) {
            if (hist->etapa_id != etapa_memoria || hist->tentativa_num != tentativa_memoria) {
                if (etapa_memoria != -1) {
                    fprintf(arquivo_saida, "Ticks na etapa: %d ticks\n", (hist->tick_fila - tick_inicio_etapa)); 
                }
                fprintf(arquivo_saida, "Etapa %d tentativa %d:\n", hist->etapa_id, hist->tentativa_num);
                etapa_memoria = hist->etapa_id;
                tentativa_memoria = hist->tentativa_num;
                tick_inicio_etapa = hist->tick_fila;
            }

            fprintf(arquivo_saida, "Atividade %d (%s) fila:%d inicio:%d fim:%d %s\n",
                    hist->atividade_id, hist->nome_atividade, 
                    hist->tick_fila, hist->tick_inicio, hist->tick_fim, 
                    hist->status_resultado ? "OK" : "FALHOU");
            hist = hist->prox;
        }
        
        if (p->historico) {
            no_pilha *ultimo = p->historico;
            while(ultimo->prox) ultimo = ultimo->prox;
            fprintf(arquivo_saida, "Ticks na etapa: %d ticks\n", (ultimo->tick_fim - tick_inicio_etapa));
        }

        fprintf(arquivo_saida, "--- PRODUTO\n\n");
        aux_prod = aux_prod->next;
    }
    
    if (pilha_descarte_global) {
        fprintf(arquivo_saida, "------- PRODUTOS DESCARTADOS (FALHA CRITICA) --\n");
        no_descarte *desc = pilha_descarte_global;
        while(desc) {
            fprintf(arquivo_saida, "Produto ID: %d | Falhou na Etapa: %d\n", desc->prod->id, desc->prod->etapa_atual->id);
            desc = desc->prox;
        }
    }

    fclose(arquivo_saida);
    printf("\n****** SIMULACAO FINALIZADA NO TICK %d ******\n", tick_final);
    printf("Painel encerrado. O Relatorio foi salvo em '%s'!\n",nome);

    liberar_etapas(fila_etapas);
    liberar_produtos(fila_ativos);
    liberar_produtos(fila_concluidos);
    liberar_arvore(arvore_produtos);
    liberar_descarte(pilha_descarte_global);
    liberar_fila_entrada(&f_entrada);

    return 0;
}