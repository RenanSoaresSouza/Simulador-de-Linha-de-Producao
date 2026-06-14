#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

// ==========================================
// 1. ESTRUTURAS DE DADOS OBRIGATÓRIAS
// ==========================================

typedef struct atividade // LSE para as Atividades
{
    char nome[50];
    float falha;
    int duracao;
    int id;
    struct atividade *next;
} atividade;

typedef struct fila_atividades 
{
    atividade *start;
    atividade *end;
} fila_atividades;

typedef struct etapa // LDE para as Etapas
{
    int id;
    char nome[100];
    int cap_eta;          // Capacidade Máxima da Etapa
    int ocupacao_atual;   // Quantidade atual de produtos na etapa
    float taxa_falha_ini; // Controle de qualidade inicial da etapa
    fila_atividades atividades;
    struct etapa *next;
    struct etapa *prev;
} etapa;

typedef struct no_pilha // Pilha (LIFO) de Histórico Individual do Produto
{
    int atividade_id;
    char nome_atividade[50];
    int tick_fila;
    int tick_inicio;
    int tick_fim;
    int status_resultado; // 1 = OK, 0 = FALHOU
    struct no_pilha *prox;
} no_pilha;

typedef struct produto // Estrutura Central do Item
{
    int id;
    etapa *etapa_atual;
    atividade *ativ_atual;
    int criacao;          // Tick de nascimento
    int entrada_linha;    // Tick de entrada real na primeira etapa
    int restante;         // Tempo restante na atividade corrente
    int falhas_totais;    // Contador de erros do produto
    int tempo_espera;     // Acumulador de tempo parado em filas/gargalos
    int tick_entrou_na_fila_da_atividade;
    no_pilha *historico;  // Topo da Pilha LIFO Individual
} produto;

typedef struct no_fila // Nós para a Fila FIFO de Entrada Global
{
    produto *prod;
    struct no_fila *next;
} no_fila;

typedef struct fila_entrada 
{
    no_fila *start;
    no_fila *end;
} fila_entrada;

typedef struct ativos // Registro dos itens ativos na esteira (Processamento)
{
    produto *prod;
    struct ativos *next;
} ativos;

typedef struct produtos_log_tree // Árvore BST para Rastreamento em Tempo Real
{
    produto *prod;
    struct produtos_log_tree *left;
    struct produtos_log_tree *right;
} produtos_log_tree;

typedef struct no_descarte // Pilha LIFO Global de Descarte (Scrap)
{
    produto *prod;
    struct no_descarte *prox;
} no_descarte;

// Global para a Pilha de Descarte
no_descarte *pilha_descarte_global = NULL;
int total_falhas_sistema = 0;
int total_concluidos_sistema = 0;

// ==========================================
// 2. FUNÇÕES DE MANIPULAÇÃO DAS ESTRUTURAS
// ==========================================

void push_historico(produto *p, atividade *ativ, int t_fila, int t_ini, int t_fim, int status)
{
    no_pilha *novo = (no_pilha *)malloc(sizeof(no_pilha));
    novo->atividade_id = ativ->id;
    strcpy(novo->nome_atividade, ativ->nome);
    novo->tick_fila = t_fila;
    novo->tick_inicio = t_ini;
    novo->tick_fim = t_fim;
    novo->status_resultado = status;
    novo->prox = p->historico;
    p->historico = novo;
}

void pop_historico(produto *p)
{
    if (p->historico)
    {
        no_pilha *aux = p->historico;
        p->historico = p->historico->prox;
        free(aux);
    }
}

void push_descarte(produto *p)
{
    no_descarte *novo = (no_descarte *)malloc(sizeof(no_descarte));
    novo->prod = p;
    novo->prox = pilha_descarte_global;
    pilha_descarte_global = novo;
}

void enqueue_entrada(fila_entrada *fila, produto *p)
{
    no_fila *novo = (no_fila *)malloc(sizeof(no_fila));
    novo->prod = p;
    novo->next = NULL;
    if (!fila->start)
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

produto *dequeue_entrada(fila_entrada *fila)
{
    if (!fila->start) return NULL;
    no_fila *aux = fila->start;
    produto *p = aux->prod;
    fila->start = fila->start->next;
    if (!fila->start) fila->end = NULL;
    free(aux);
    return p;
}

void inserir_atividade(fila_atividades *fila, const char *nome, float falha, int duracao, int id)
{
    atividade *novo = (atividade *)malloc(sizeof(atividade));
    strcpy(novo->nome, nome);
    novo->falha = falha;
    novo->duracao = duracao;
    novo->id = id;
    novo->next = NULL;

    if (!(fila->start))
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

void inserir_etapa(etapa **head, int id, int cap, float falha_ini, char *nome)
{
    etapa *novo = (etapa *)malloc(sizeof(etapa));
    strcpy(novo->nome, nome);
    novo->id = id;
    novo->cap_eta = cap;
    novo->ocupacao_atual = 0;
    novo->taxa_falha_ini = falha_ini;
    novo->atividades.start = NULL;
    novo->atividades.end = NULL;
    novo->next = NULL;
    novo->prev = NULL;

    if (!(*head))
    {
        *head = novo;
    }
    else
    {
        etapa *aux = *head;
        while (aux->next) aux = aux->next;
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
    while (aux && aux->prod->id != id)
    {
        ant = aux;
        aux = aux->next;
    }
    if (aux)
    {
        if (!ant) *head = aux->next;
        else ant->next = aux->next;
        free(aux);
    }
}

produtos_log_tree *inserir_arvore(produtos_log_tree *raiz, produto *p)
{
    if (!(raiz))
    {
        produtos_log_tree *novo = (produtos_log_tree *)malloc(sizeof(produtos_log_tree));
        novo->prod = p;
        novo->left = NULL;
        novo->right = NULL;
        return novo;
    }
    if (p->id < raiz->prod->id) raiz->left = inserir_arvore(raiz->left, p);
    else raiz->right = inserir_arvore(raiz->right, p);
    return raiz;
}

produtos_log_tree *remover_arvore(produtos_log_tree *raiz, int id)
{
    if (!raiz) return NULL;
    if (id < raiz->prod->id) raiz->left = remover_arvore(raiz->left, id);
    else if (id > raiz->prod->id) raiz->right = remover_arvore(raiz->right, id);
    else
    {
        if (!raiz->left)
        {
            produtos_log_tree *temp = raiz->right;
            free(raiz);
            return temp;
        }
        else if (!raiz->right)
        {
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
// 3. ENTRADA DE CONFIGURAÇÃO (PARSER)
// ==========================================

void carregar_arquivo(char *nome_file, etapa **fila_etapas, int *qtd_total, int *vazao, char *nome, int *semente, int *duracao)
{
    FILE *file = fopen(nome_file, "r");
    if (!file) { printf("Erro ao abrir o arquivo\n"); return; }

    char string[50];
    etapa *etapa_atual = NULL;

    while (fscanf(file, "%s", string) != EOF)
    {
        if (!(strcmp(string, "SIMULACAO")))
        {
            char nome_simulacao[50];
            fscanf(file, "%s %d %d", nome_simulacao, semente, duracao);
            srand(*semente);
        }
        else if (!(strcmp(string, "PRODUTOS")))
        {
            int tipo;
            fscanf(file, "%d %d %s", qtd_total, vazao, nome);
        }
        else if (!(strcmp(string, "ETAPA")))
        {
            int id, atividades, cap;
            float falha_ini;
            char nome_etapa[50];
            fscanf(file, "%d %d %d %f %s", &id, &atividades, &cap, &falha_ini, nome_etapa);

            inserir_etapa(fila_etapas, id, cap, falha_ini, nome_etapa);
            etapa *aux_busca = *fila_etapas;
            while (aux_busca->next) aux_busca = aux_busca->next;
            etapa_atual = aux_busca;
        }
        else if (!(strcmp(string, "ATIVIDADE")))
        {
            int id, duracao_at;
            float chance;
            char nome_atividade[50];
            fscanf(file, "%d %d %f %s", &id, &duracao_at, &chance, nome_atividade);

            if (etapa_atual) inserir_atividade(&(etapa_atual->atividades), nome_atividade, chance, duracao_at, id);
        }
    }
    fclose(file);
    printf("Configuracao de fabrica carregada com sucesso!\n");
}

// ==========================================
// 4. DA_MAIN_E_EXECUÇÃO_DO_TEMPO
// ==========================================

int main()
{
    etapa *fila_etapas = NULL;
    ativos *fila_ativos = NULL;
    produtos_log_tree *arvore_produtos = NULL;
    fila_entrada f_entrada = {NULL, NULL};

    int qtd_produtos = 0, taxa_vazao = 0, semente = 0, duracao_simulacao = 0;
    char nome_produto[50] = "";
    char arquivo[50];
    int produtos_criados_total = 0;

    printf("digite um arquivo de entrada: ");
    scanf("%s",arquivo);
    carregar_arquivo(arquivo, &fila_etapas, &qtd_produtos, &taxa_vazao, nome_produto, &semente, &duracao_simulacao);
    printf("A iniciar simulacao de %d segundos para o produto %s...\n\n", duracao_simulacao, nome_produto);

    FILE *arquivo_saida = fopen("relatorio_producao.txt", "w");
    if (!arquivo_saida) { printf("Erro ao gerar arquivo de saída!\n"); return -1; }

    // Loop do Pulso Temporal (Clock Ativo de 1s)
    for (int i = 0; i < duracao_simulacao; i++)
    {
        // Ingestão controlada pela taxa de vazão na Fila FIFO de Entrada
        for (int k = 0; k < taxa_vazao && produtos_criados_total < qtd_produtos; k++)
        {
            produto *novo_produto = (produto *)malloc(sizeof(produto));
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

            enqueue_entrada(&f_entrada, novo_produto);
        }

        // Tenta transferir itens da Fila FIFO de Entrada para a Linha de Produção Ativa
        while (f_entrada.start && fila_etapas->ocupacao_atual < fila_etapas->cap_eta)
        {
            produto *p = dequeue_entrada(&f_entrada);
            p->entrada_linha = i;
            p->tick_entrou_na_fila_da_atividade = i;
            fila_etapas->ocupacao_atual++;
            arvore_produtos = inserir_arvore(arvore_produtos, p); // Adiciona na BST Ativa
            inserir_ativos(&fila_ativos, p);
        }

        // Processamento dos itens na esteira
        ativos *aux = fila_ativos;
        while (aux)
        {
            ativos *failsafe = aux->next;
            produto *p = aux->prod;

            // Se o produto está trancado por falta de espaço na etapa subsequente (Gargalo)
            if (p->restante == 0)
            {
                if (!p->ativ_atual->next && p->etapa_atual->next && p->etapa_atual->next->ocupacao_atual >= p->etapa_atual->next->cap_eta)
                {
                    p->tempo_espera++;
                    aux = failsafe;
                    continue;
                }
            }

            p->restante--;

            if (p->restante == 0)
            {
                // Roteio Estatístico Estocástico (Três Cenários Mutuamente Exclusivos)
                float r = (float)rand() / RAND_MAX;

                if (r < (p->ativ_atual->falha * 0.15f)) // Cenário 3: Erro Crítico (Morte)
                {
                    total_falhas_sistema++;
                    p->etapa_atual->ocupacao_atual--;
                    arvore_produtos = remover_arvore(arvore_produtos, p->id);
                    remover_ativos(&fila_ativos, p->id);
                    push_descarte(p); // Salva permanentemente no Scrap Global
                }
                else if (r < p->ativ_atual->falha) // Cenário 2: Erro Leve (Retrabalho da Etapa)
                {
                    total_falhas_sistema++;
                    p->falhas_totais++;
                    push_historico(p, p->ativ_atual, p->tick_entrou_na_fila_da_atividade, i - p->ativ_atual->duracao, i, 0);
                    
                    // Rollback para o primeiro nó da LSE da Etapa Atual
                    p->ativ_atual = p->etapa_atual->atividades.start;
                    p->restante = p->ativ_atual->duracao;
                    p->tick_entrou_na_fila_da_atividade = i;

                    // Limpa histórico da pilha individual referente a esta etapa
                    while (p->historico && p->historico->status_resultado == 1) pop_historico(p);
                }
                else // Cenário 1: Sucesso total na atividade
                {
                    push_historico(p, p->ativ_atual, p->tick_entrou_na_fila_da_atividade, i - p->ativ_atual->duracao, i, 1);

                    if (p->ativ_atual->next) // Próxima Atividade na LSE
                    {
                        p->ativ_atual = p->ativ_atual->next;
                        p->restante = p->ativ_atual->duracao;
                        p->tick_entrou_na_fila_da_atividade = i;
                    }
                    else // Fim das Atividades da Etapa Atual
                    {
                        if (p->etapa_atual->next) // Avança para Próxima Etapa da LDE
                        {
                            etapa *proxima = p->etapa_atual->next;
                            
                            // Se houver espaço físico na próxima etapa
                            if (proxima->ocupacao_atual < proxima->cap_eta)
                            {
                                // Política Inter-etapas: Controle de Qualidade Inicial Obrigatório
                                float r_qualidade = (float)rand() / RAND_MAX;
                                if (r_qualidade < proxima->taxa_falha_ini) // Desvio: Volta para Etapa Anterior
                                {
                                    total_falhas_sistema++;
                                    p->falhas_totais++;
                                    // Continua na mesma etapa, mas reinicia as atividades
                                    p->ativ_atual = p->etapa_atual->atividades.start;
                                    p->restante = p->ativ_atual->duracao;
                                    p->tick_entrou_na_fila_da_atividade = i;
                                }
                                else // Avanço definitivo de etapa
                                {
                                    p->etapa_atual->ocupacao_atual--;
                                    p->etapa_atual = proxima;
                                    p->etapa_atual->ocupacao_atual++;
                                    p->ativ_atual = p->etapa_atual->atividades.start;
                                    p->restante = p->ativ_atual->duracao;
                                    p->tick_entrou_na_fila_da_atividade = i;
                                }
                            }
                        }
                        else // Produto Concluído com Sucesso Total!
                        {
                            total_concluidos_sistema++;
                            p->etapa_atual->ocupacao_atual--;
                            arvore_produtos = remover_arvore(arvore_produtos, p->id);
                            remover_ativos(&fila_ativos, p->id);

                            // Gravação imediata e estruturada no arquivo físico (Persistência)
                            fprintf(arquivo_saida, "--- PRODUTO %d\n", p->id);
                            fprintf(arquivo_saida, "Modelo: %s\n", nome_produto);
                            fprintf(arquivo_saida, "Criacao: tick %d\n", p->criacao);
                            fprintf(arquivo_saida, "Entrada na linha: tick %d\n", p->entrada_linha);
                            fprintf(arquivo_saida, "Saida da linha: tick %d\n", i);
                            fprintf(arquivo_saida, "Tempo total: %d ticks\n", (i - p->entrada_linha));
                            fprintf(arquivo_saida, "Tempo em espera: %d ticks\n", p->tempo_espera);
                            fprintf(arquivo_saida, "Falhas: %d\n", p->falhas_totais);
                            fprintf(arquivo_saida, "Trajetoria:\n");

                            // Descarrega os dados desempilhando a Pilha de Histórico Individual
                            no_pilha *hist = p->historico;
                            while (hist)
                            {
                                fprintf(arquivo_saida, "  Atividade %d (%s) fila:%d inicio:%d fim:%d %s\n",
                                        hist->atividade_id, hist->nome_atividade, hist->tick_fila,
                                        hist->tick_inicio, hist->tick_fim, hist->status_resultado ? "OK" : "FALHOU");
                                hist = hist->prox;
                            }
                            fprintf(arquivo_saida, "\n");

                            // Liberação adequada de memória do objeto finalizado
                            while (p->historico) pop_historico(p);
                            free(p);
                        }
                    }
                }
            }
            aux = failsafe;
        }

        #ifdef _WIN32
            Sleep(1); // Modificado para 1ms para os testes rodarem rápido
        #else
            usleep(1000);
        #endif
    }

    // Gravação dos Metadados Consolidados no topo ou base do arquivo
    rewind(arquivo_saida);
    fprintf(arquivo_saida, "=== METADADOS ===\n");
    fprintf(arquivo_saida, "id_simulacao: %s_2026\n", nome_produto);
    fprintf(arquivo_saida, "semente_utilizada: %d\n", semente);
    fprintf(arquivo_saida, "produtos_concluidos: %d\n", total_concluidos_sistema);
    fprintf(arquivo_saida, "falhas_totais: %d\n", total_falhas_sistema);
    fprintf(arquivo_saida, "Meta alcancada: %s\n\n", total_concluidos_sistema >= qtd_produtos ? "sim" : "nao");

    fclose(arquivo_saida);
    printf("\n****** SIMULACAO CONCLUIDA ******\n");
    printf("Todos os logs fisicos estruturados foram persistidos com sucesso em 'relatorio_producao.txt'!\n");

    return 0;
}