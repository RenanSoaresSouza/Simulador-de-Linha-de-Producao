# Simulador de Linha de Produção Industrial

O **Simulador de Linha de Produção Industrial** é um projeto desenvolvido em **C** com o objetivo de simular o funcionamento de uma linha de produção composta por diversas etapas e atividades. Durante a execução, produtos percorrem a linha produtiva, podendo enfrentar filas, tempos de espera, falhas operacionais e descarte por falhas críticas.

Ao final da simulação, o sistema gera um relatório completo contendo métricas de desempenho da fábrica, das etapas, das atividades e de cada produto processado.

---

# Problema

Em processos industriais, alterações na capacidade produtiva, tempos de processamento e taxas de falha podem gerar gargalos, reduzir a produtividade e aumentar os custos.

Realizar testes diretamente em uma linha de produção real pode ser caro, demorado e arriscado.

Este projeto busca reproduzir esses cenários em um ambiente virtual, permitindo analisar o comportamento da linha de produção antes da implementação de mudanças reais.

---

# Objetivos

* Simular o fluxo de produtos em uma linha de produção.
* Modelar filas entre etapas da produção.
* Simular falhas operacionais e falhas críticas.
* Gerar indicadores de desempenho.
* Demonstrar a utilização de estruturas de dados dinâmicas em C.

---

# Funcionalidades

* Leitura automática da configuração da fábrica através de arquivo de entrada.
* Simulação baseada em ticks (tempo discreto).
* Controle da capacidade máxima de cada etapa.
* Sistema de filas FIFO.
* Simulação de falhas probabilísticas.
* Reprocessamento de produtos após falhas.
* Descarte automático em caso de falha crítica.
* Registro completo do histórico de processamento de cada produto.
* Geração automática de relatório em arquivo `.txt`.
* Modo Fast Forward para acelerar a simulação.

---

# Estruturas de Dados Utilizadas

O projeto utiliza diversas estruturas de dados clássicas.

### Lista Duplamente Encadeada

Utilizada para representar as etapas da linha de produção.

Cada etapa conhece sua anterior e sua próxima etapa.

### Lista Simplesmente Encadeada

Utilizada para representar:

* atividades de cada etapa;
* produtos ativos;
* produtos descartados.

### Fila (FIFO)

Responsável pela fila de entrada dos produtos na linha de produção.

### Pilha

Armazena todo o histórico de execução de cada produto, incluindo:

* atividade executada;
* etapa;
* tentativa;
* tempo de fila;
* tempo de início;
* tempo de término;
* sucesso ou falha.

### Árvore Binária de Busca

Utilizada para manter os produtos ativos indexados pelo identificador.

---

# Arquitetura

```
Entrada (arquivo)

        │

        ▼

Fila de Entrada (FIFO)

        │

        ▼

Etapa 1

        │

        ▼

Etapa 2

        │

        ▼

...

        │

        ▼

Última Etapa

        │

 ┌──────┴──────┐

 ▼             ▼

Concluído   Descartado
```

---

# Organização do Projeto

```
include/

    atividades.h
    etapas.h
    produtos.h
    fila.h
    pilha.h
    arvore.h
    descarte.h
    ativos.h
    utils.h

src/

    Main.c
    atividades.c
    etapas.c
    produtos.c
    fila.c
    pilha.c
    arvore.c
    descarte.c
    ativos.c
    utils.c
```

---

# Arquivo de Entrada

A simulação é configurada através de um arquivo texto:



---

# Arquivo de Saída

Ao término da simulação é gerado automaticamente um relatório contendo:

### Informações Gerais

* identificador da simulação;
* duração;
* produtos concluídos;
* falhas totais;
* tempo médio na linha;
* tempo médio em filas.

### Métricas das Etapas

* quantidade de produtos processados;
* tempo mínimo;
* tempo médio;
* tempo máximo;
* falhas por etapa.

### Métricas das Atividades

* vazão;
* tempo médio em fila;
* tempo médio de execução.

### Histórico dos Produtos

Para cada produto são registrados:

* criação;
* entrada na linha;
* saída;
* falhas;
* todas as atividades executadas;
* tempo gasto em cada etapa.

---

# Tecnologias Utilizadas

* Linguagem C
* GCC
* Visual Studio Code
* Estruturas de Dados Dinâmicas
* Manipulação de Arquivos
* Geração de Relatórios

---

# Como Compilar

```
gcc src/*.c -Iinclude -o simulador
```

---

# Como Executar

```
./simulador
```

ou, no Windows:

```
simulador.exe
```

Após iniciar o programa, informe o arquivo de configuração da simulação.

---

# Resultados

Ao final da execução, é criado automaticamente um arquivo de relatório contendo todas as métricas obtidas durante a simulação.

Os arquivos são armazenados na pasta `relatorios/` e recebem nomes sequenciais para evitar sobrescrever execuções anteriores.

---

---

