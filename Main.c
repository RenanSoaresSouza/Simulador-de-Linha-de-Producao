#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

// ==========================================
// 1. ESTRUTURAS DE DADOS OBRIGATÓRIAS
// ==========================================

typedef struct atividade {
    char nome[50];
    float falha;
    int duracao;
    int id;
    int itens_processados;
    int tempo_total_fila;
    int tempo_total_execucao;
    struct atividade *next;
} atividade;

typedef struct fila_atividades {
    atividade *start;
    atividade *end;
} fila_atividades;

typedef struct etapa {
    int id;
    char nome[100];
    int quant_atv;
    int cap_eta;          
    int ocupacao_atual;   
    float taxa_falha_ini; 
    int total_falhas;
    int produtos_processados;
    int tempo_minimo;
    int tempo_maximo;
    int tempo_total_gasto;
    int tempo_total_fila;
    fila_atividades atividades;
    struct etapa *next;
    struct etapa *prev;
} etapa;

typedef struct no_pilha {
    int atividade_id;
    int etapa_id;
    char nome_etapa[50];
    char nome_atividade[50];
    int tentativa_num; 
    int tick_fila;
    int tick_inicio;
    int tick_fim;
    int status_resultado; 
    struct no_pilha *prox;
} no_pilha;

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

typedef struct no_fila {
    produto *prod;
    struct no_fila *next;
} no_fila;

typedef struct fila_entrada {
    no_fila *start;
    no_fila *end;
} fila_entrada;

typedef struct ativos {
    produto *prod;
    struct ativos *next;
} ativos;

typedef struct produtos_log_tree {
    produto *prod;
    struct produtos_log_tree *left;
    struct produtos_log_tree *right;
} produtos_log_tree;

typedef struct no_descarte {
    produto *prod;
    struct no_descarte *prox;
} no_descarte;

no_descarte *pilha_descarte_global = NULL;
int total_falhas_sistema = 0;
int total_concluidos_sistema = 0;

void *xmalloc(size_t size) {
    void *p = malloc(size);

    if (p == NULL) {
        printf("Erro de memoria\n");
        exit(EXIT_FAILURE);
    }

    return p;
}

// ==========================================
// 2. FUNÇÕES DE MANIPULAÇÃO DAS ESTRUTURAS
// ==========================================

void push_historico(produto *p, atividade *ativ, etapa *eta, int t_fila, int t_ini, int t_fim, int status) {
    no_pilha *novo = xmalloc(sizeof(no_pilha));
    novo->atividade_id = ativ->id;
    strncpy(novo->nome_atividade, ativ->nome, sizeof(novo->nome_atividade)-1);
    novo->nome_atividade[sizeof(novo->nome_atividade)-1] = '\0';
    novo->tick_fila = t_fila;
    novo->tick_inicio = t_ini;
    novo->tick_fim = t_fim;
    novo->status_resultado = status;
    novo->prox = p->historico;
    novo->etapa_id = eta->id;
    strncpy(novo->nome_etapa, eta->nome, sizeof(novo->nome_etapa)-1);
    novo->nome_etapa[sizeof(novo->nome_etapa)-1] = '\0';
    novo->tentativa_num = p->tentativa_atual; 
    p->historico = novo;
}

void pop_historico(produto *p) {
    if (p->historico) {
        no_pilha *aux = p->historico;
        p->historico = p->historico->prox;
        free(aux);
    }
}

no_pilha* inverter_historico(no_pilha *topo) {
    no_pilha *anterior = NULL;
    no_pilha *atual = topo;
    no_pilha *proximo = NULL;
    while (atual != NULL) {
        proximo = atual->prox;  
        atual->prox = anterior; 
        anterior = atual;       
        atual = proximo;        
    }
    return anterior; 
}

void push_descarte(produto *p) {
    no_descarte *novo = xmalloc(sizeof(no_descarte));
    novo->prod = p;
    novo->prox = pilha_descarte_global;
    pilha_descarte_global = novo;
}

void enqueue_entrada(fila_entrada *fila, produto *p) {
    no_fila *novo = xmalloc(sizeof(no_fila));
    novo->prod = p;
    novo->next = NULL;
    if (!fila->start) {
        fila->start = novo;
        fila->end = novo;
    } else {
        fila->end->next = novo;
        fila->end = novo;
    }
}

produto *dequeue_entrada(fila_entrada *fila) {
    if (!fila->start) return NULL;
    no_fila *aux = fila->start;
    produto *p = aux->prod;
    fila->start = fila->start->next;
    if (!fila->start) fila->end = NULL;
    free(aux);
    return p;
}

void inserir_atividade(fila_atividades *fila, const char *nome, float falha, int duracao, int id) {
    atividade *novo = xmalloc(sizeof(atividade));
    strncpy(novo->nome, nome, sizeof(novo->nome)-1);
    novo->nome[sizeof(novo->nome)-1] = '\0';
    novo->falha = falha;
    novo->duracao = duracao;
    novo->id = id;
    novo->itens_processados = 0;
    novo->tempo_total_fila = 0;
    novo->tempo_total_execucao = 0;
    novo->next = NULL;
    if (!(fila->start)) {
        fila->start = novo;
        fila->end = novo;
    } else {
        fila->end->next = novo;
        fila->end = novo;
    }
}

void inserir_etapa(etapa **head, int id, int cap, float falha_ini, char *nome, int q_atv) {
    etapa *novo = xmalloc(sizeof(etapa));
    strncpy(novo->nome, nome, sizeof(novo->nome)-1);
    novo->nome[sizeof(novo->nome)-1] = '\0';
    novo->id = id;
    novo->quant_atv = q_atv;
    novo->cap_eta = cap;
    novo->ocupacao_atual = 0;
    novo->taxa_falha_ini = falha_ini;
    novo->total_falhas = 0;
    novo->produtos_processados = 0;
    novo->tempo_minimo = 999999; 
    novo->tempo_maximo = 0;
    novo->tempo_total_gasto = 0;
    novo->tempo_total_fila = 0;
    novo->atividades.start = NULL;
    novo->atividades.end = NULL;
    novo->next = NULL;
    novo->prev = NULL;
    if (!(*head)) *head = novo;
    else {
        etapa *aux = *head;
        while (aux->next) aux = aux->next;
        aux->next = novo;
        novo->prev = aux;
    }
}

void inserir_ativos(ativos **head, produto *p) {
    ativos *novo = xmalloc(sizeof(ativos));
    novo->prod = p;
    novo->next = *head;
    (*head) = novo;
}

void remover_ativos(ativos **head, int id) {
    ativos *aux = *head, *ant = NULL;
    while (aux && aux->prod->id != id) {
        ant = aux;
        aux = aux->next;
    }
    if (aux) {
        if (!ant) *head = aux->next;
        else ant->next = aux->next;
        free(aux);
    }
}

produtos_log_tree *inserir_arvore(produtos_log_tree *raiz, produto *p) {
    if (!(raiz)) {
        produtos_log_tree *novo = xmalloc(sizeof(produtos_log_tree));
        novo->prod = p;
        novo->left = NULL;
        novo->right = NULL;
        return novo;
    }
    if (p->id < raiz->prod->id) raiz->left = inserir_arvore(raiz->left, p);
    else raiz->right = inserir_arvore(raiz->right, p);
    return raiz;
}

produtos_log_tree *remover_arvore(produtos_log_tree *raiz, int id) {
    if (!raiz) return NULL;
    if (id < raiz->prod->id) raiz->left = remover_arvore(raiz->left, id);
    else if (id > raiz->prod->id) raiz->right = remover_arvore(raiz->right, id);
    else {
        if (!raiz->left) {
            produtos_log_tree *temp = raiz->right;
            free(raiz);
            return temp;
        } else if (!raiz->right) {
            produtos_log_tree *temp = raiz->left;
            free(raiz);
            return temp;
        }
        produtos_log_tree *temp = raiz->right;
        while (temp && temp->left) temp = temp->left;
        raiz->prod = temp->prod;
        raiz->right = remover_arvore(raiz->right, temp->prod->id);
    }
    return raiz;
}

// ==========================================
// 3. IMPRESSÃO DO PAINEL EM TEMPO REAL
// ==========================================
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

void liberar_atividades(fila_atividades *fila) {
    atividade *aux;
    while (fila->start){
        aux = fila->start;
        fila->start = fila->start->next;
        free(aux);
    }
    fila->end = NULL;
}

void liberar_historico(no_pilha *head){
    no_pilha *aux;
    while(head){
        aux = head;
        head = head->prox;
        free(aux);
    }
}

void liberar_produtos(ativos *lista){
    ativos *aux;
    while (lista){
        aux = lista;
        lista = lista->next;
        liberar_historico(aux->prod->historico);
        free(aux->prod);
        free(aux);
    }
}

void liberar_etapas(etapa *head) {
    etapa *aux;
    while(head){
        aux = head;
        head = head->next;
        liberar_atividades(&aux->atividades);
        free(aux);
    }
}

void liberar_arvore(produtos_log_tree *raiz) {
    if (raiz == NULL)
        return;

    liberar_arvore(raiz->left);
    liberar_arvore(raiz->right);

    free(raiz);
}
/*
typedef struct no_descarte {
    produto *prod;
    struct no_descarte *prox;
} no_descarte;
 */
void liberar_descarte(no_descarte *pilha){
    no_descarte *aux;
    while (pilha){
        aux = pilha;
        pilha = pilha->prox;
        liberar_historico(aux->prod->historico);
        free(aux->prod);
        free(aux);
    }
}
void liberar_fila_entrada(fila_entrada *fila) {
    no_fila *aux;

    while (fila->start) {
        aux = fila->start;
        fila->start = fila->start->next;

        liberar_historico(aux->prod->historico);
        free(aux->prod);
        free(aux);
    }

    fila->end = NULL;
}


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

    FILE *arquivo_saida = fopen("relatorio_producao.txt", "w");
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
    printf("Painel encerrado. O Relatorio foi salvo em 'relatorio_producao.txt'!\n");

    liberar_etapas(fila_etapas);
    liberar_produtos(fila_ativos);
    liberar_produtos(fila_concluidos);
    liberar_arvore(arvore_produtos);
    liberar_descarte(pilha_descarte_global);
    liberar_fila_entrada(&f_entrada);

    return 0;
}