# Simulador de Algoritmos de Escalonamento

Trabalho prático da disciplina **Sistemas Operacionais** — Mestrado em Ciência da
Computação, UFMS, 2026.

## 1. Objetivo

Implementar um simulador de algoritmos de escalonamento de processos com
interface gráfica que permita ao usuário selecionar o algoritmo, fornecer um
conjunto de processos e observar passo a passo (tick a tick) a execução do
escalonador, incluindo transições entre os estados *pronto*, *executando*,
*bloqueado em E/S* e *finalizado*.

## 2. Algoritmos implementados

| Algoritmo                              | Preempção | Parâmetros adicionais       |
|----------------------------------------|-----------|-----------------------------|
| FCFS (First-Come, First-Served)        | não       | —                           |
| SJF (Shortest Job First)               | sim/não   | escolhido pelo usuário      |
| Prioridade                             | sim/não   | escolhido pelo usuário      |
| Round Robin                            | por quantum | quantum (inteiro positivo)|

Convenções adotadas:

- **Prioridade:** menor número indica maior prioridade (convenção
  Unix/Tanenbaum).
- **Bursts:** cada processo é descrito por uma sequência alternada de bursts
  de CPU e de E/S. O primeiro burst deve ser de CPU.
- **Tempo:** unidades discretas (*ticks*). Cada tick representa uma unidade
  de tempo configurável conceitualmente (por exemplo, 1 ms).
- **Tempo máximo de simulação (opcional):** o usuário pode informar um
  limite de ticks. Caso seja atingido antes da conclusão de todos os
  processos, a simulação é encerrada e os processos não-finalizados são
  reportados normalmente, com seu estado final (executando, pronto,
  bloqueado ou ainda não chegado).
- **Empate em SJF preemptivo (SRTF):** o processo em execução tem preferência
  sobre processos prontos com o mesmo tempo restante, evitando trocas de
  contexto desnecessárias.
- **Empate em Round Robin:** ordem FIFO da fila de prontos. A coluna
  *Prioridade* é ignorada nesse algoritmo.

## 3. Arquitetura

O projeto é dividido em três módulos compilados como alvos CMake
independentes:

```
so/
├── backend/         biblioteca estática com a lógica de simulação
│   ├── include/
│   │   ├── process.hpp     tipos de domínio (Process, Burst, State, ...)
│   │   └── scheduler.hpp   API pública: run(processes, config)
│   └── src/simulator.cpp   loop tick-a-tick unificado
├── frontend/        executável Qt6 que consome o backend
│   ├── main.cpp
│   ├── mainwindow.{h,cpp}
│   ├── process_table_model.{h,cpp}   modelo da tabela editável de processos
│   └── timeline_view.{h,cpp}         visualização gráfica tick-a-tick
├── tests/           verificação de sanidade dos algoritmos
│   └── sanity_test.cpp
├── examples/
│   └── proc1.txt    conjunto de exemplo de processos
└── CMakeLists.txt
```

A separação entre backend e frontend é estrita: o backend não depende de Qt
e pode ser linkado por qualquer outro frontend (CLI, web, etc.). A
comunicação se dá por chamada direta à função
`sched::run(processes, config)`, que recebe os processos parseados e a
configuração do algoritmo, e retorna um `SimulationResult` contendo a
linha do tempo completa, estatísticas por processo e o gráfico de Gantt
em forma de vetor de PIDs por tick.

### 3.1. Modelo de simulação

O simulador opera em ticks discretos. Em cada tick `t`, na ordem:

1. Processos com tempo de chegada `<= t` transitam de NEW para READY.
2. É selecionado o próximo processo a executar segundo o critério do
   algoritmo escolhido.
3. Caso o processo selecionado seja diferente do anterior, registra-se o
   evento de troca de contexto (preempção ou início de execução).
4. Captura-se um *snapshot* do estado de todos os processos.
5. Decrementa-se em uma unidade o burst corrente do processo em execução
   (CPU) e o burst corrente de cada processo em estado BLOCKED (E/S).
6. Após o decremento, são processadas as transições:
   - Burst de CPU concluído → próximo burst (BLOCKED se for E/S; FINISHED
     se não houver mais bursts).
   - Quantum esgotado em Round Robin → READY (final da fila).
   - Burst de E/S concluído → próximo burst de CPU (READY).

Esse modelo único é parametrizado por uma função de comparação
`better(a, b, cfg)` que define o critério de seleção para cada algoritmo.
Dessa forma, todos os algoritmos compartilham o mesmo motor de
execução, garantindo coerência entre os estados reportados.

## 4. Pré-requisitos
O projeto foi desenvolvido em C++17 com CMake e Qt6 Widgets. Ele pode ser
compilado em Linux. No Windows, recomenda-se o uso do WSL2 com Ubuntu e WSLg,
pois a interface gráfica utiliza Qt.

- Sistema Linux (testado em Debian/Ubuntu sob WSL2 com WSLg)
- `g++` 12 ou superior (compatível com C++17)
- `CMake` 3.16 ou superior
- `Qt6` (módulo Widgets)
- bibliotecas de desenvolvimento para OpenGL e XKB, necessárias para o Qt6 no WSL.

Instalação em distribuições baseadas em Debian:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake qt6-base-dev
```
Instalação no Ubuntu/WSL2:

No terminal do Ubuntu, execute:

```bash
sudo apt-update
sudo apt-install -y \
  build-essential \
  cmake \
  qt6-base-dev \
  qt6-base-dev-tools \
  libgl1-mesa-dev \
  mesa-common-dev \
  libglu1-mesa-dev \
  libxkbcommon-dev \
  libxkbcommon-x11-dev
```

## 5. Compilação

A partir da raiz do projeto:

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

Serão produzidos:

- `backend/libscheduler_backend.a` — biblioteca estática do backend
- `frontend/simulador` — executável da interface gráfica
- `tests/sanity_test` — programa de verificação dos algoritmos

## 6. Execução

### 6.1. Interface gráfica

```bash
./frontend/simulador
```

Procedimento típico:

1. Selecionar o algoritmo no menu suspenso. Os campos *Quantum* e
   *Preemptivo* são habilitados conforme a relevância.
2. (Opcional) Definir o *Tempo máx.* — limite em ticks para a
   simulação. Deixe em **ilimitado** para rodar até todos os
   processos terminarem.
3. Editar a tabela de processos diretamente, ou clicar em *Carregar
   arquivo…* e selecionar um arquivo de entrada.
4. Clicar em *Executar simulação*.
5. Inspecionar o resultado nas três abas:
   - **Timeline (tick-a-tick):** representação gráfica do gráfico de
     Gantt e do estado de cada processo em cada instante.
   - **Log textual:** sequência cronológica de eventos, incluindo
     preempções, bloqueios e conclusões.
   - **Estatísticas:** turnaround, tempo de espera e tempo de resposta
     por processo, além de médias gerais e utilização de CPU.

### 6.2. Verificação de sanidade

Para validar que os algoritmos produzem resultados consistentes com
exemplos clássicos de livro-texto:

```bash
./tests/sanity_test
```

A saída exibe o gráfico de Gantt e estatísticas para sete cenários
distintos, cobrindo os quatro algoritmos (com variantes preemptivas e
um caso com E/S). A mensagem final
`[OK] Todos os testes passaram.` indica sucesso.

### 6.3. Comparação entre algoritmos

Para reproduzir as tabelas comparativas da Seção 9, basta executar:

```bash
./tests/compare
```

O programa executa o mesmo conjunto de processos sob todos os
algoritmos e imprime, lado a lado, gráfico de Gantt e métricas.

## 7. Formato do arquivo de entrada

Arquivo texto com uma linha por processo, no formato:

```
pid chegada prioridade burst1 burst2 ...
```

- `pid`: inteiro identificador do processo
- `chegada`: instante de chegada (inteiro, em ticks)
- `prioridade`: inteiro (menor = mais prioritário)
- `burst_i`: descritor de burst, no formato `Cn` para CPU ou `In` para
  E/S, onde `n` é a duração em ticks. O primeiro burst deve ser de CPU.

Linhas iniciadas por `#` e linhas em branco são ignoradas. Exemplo
(arquivo `examples/proc1.txt`):

```
# pid chegada prioridade bursts
1 0 2 C5 I3 C2
2 1 1 C4
3 3 3 C7 I2 C3
4 6 1 C2
```

## 8. Métricas reportadas

Para cada processo finalizado, ao final da simulação, são calculados:

- **Turnaround**: `completion - arrival`
- **Espera**: tempo total em que o processo permaneceu no estado READY
- **Resposta**: `first_run - arrival`
- **CPU usada**: tempo efetivo em estado RUNNING

Para processos **não-finalizados** (caso o tempo máximo de simulação
seja atingido), são reportados:

- **Estado final**: executando, pronto, bloqueado ou não chegado.
- **CPU usada**: tempo já consumido em RUNNING.
- **Espera**: tempo já acumulado em READY.
- **Resposta**: `first_run - arrival` se o processo chegou a executar
  ao menos uma vez; "—" caso contrário.
- **Conclusão** e **turnaround** são exibidos como "—" pois não se
  aplicam.

Médias agregadas (turnaround, espera, resposta) são calculadas **apenas
sobre processos finalizados**, para evitar distorção. A interface
indica quantos processos finalizaram do total.

A **utilização de CPU** é a fração de ticks da janela de simulação em
que houve processo executando, computada sobre o tempo total
observado.

## 9. Comparação entre algoritmos

A título de análise comparativa, dois conjuntos de entrada foram
submetidos a todos os algoritmos implementados. Os números a seguir
foram gerados pelo programa `tests/compare` (Seção 6.3) e,
portanto, podem ser reproduzidos textualmente.

### 9.1. Caso 1 — quatro processos, somente CPU

Conjunto de entrada:

| PID | Chegada | Prioridade | CPU |
|-----|---------|------------|-----|
| P1  | 0       | 3          | 10  |
| P2  | 1       | 1          | 4   |
| P3  | 2       | 2          | 6   |
| P4  | 3       | 4          | 2   |

Resultados:

| Algoritmo               | Gráfico de Gantt                              | Turnaround médio | Espera média | Resposta média |
|-------------------------|-----------------------------------------------|------------------|--------------|----------------|
| FCFS                    | P1(10) P2(4) P3(6) P4(2)                      | 15.00            | 9.50         | 9.50           |
| SJF não-preemptivo      | P1(10) P4(2) P2(4) P3(6)                      | 13.50            | 8.00         | 8.00           |
| SJF preemptivo (SRTF)   | P1(1) P2(4) P4(2) P3(6) P1(9)                 | **10.25**        | **4.75**     | **1.75**       |
| Prioridade não-preempt. | P1(10) P2(4) P3(6) P4(2)                      | 15.00            | 9.50         | 9.50           |
| Prioridade preemptiva   | P1(1) P2(4) P3(6) P1(9) P4(2)                 | 13.00            | 7.50         | 5.00           |
| Round Robin (q=2)       | P1·P2·P1·P3·P4·P2·P1·P3·P1·P3·P1 (em fatias)  | 14.50            | 9.00         | 2.50           |
| Round Robin (q=4)       | P1(4) P2(4) P3(4) P4(2) P1(4) P3(2) P1(2)     | 14.50            | 9.00         | 4.50           |

Em negrito, os melhores valores de cada coluna. Todos os algoritmos
processam o mesmo total de CPU (22 ticks), portanto a métrica de
tempo total e utilização não diferencia esse caso (todos atingem 100%).

Observações:

- **SJF preemptivo (SRTF) domina em todas as métricas médias** porque,
  ao priorizar continuamente o menor tempo restante, executa P2, P4 e
  P3 antes de retomar P1, reduzindo a espera dos jobs curtos.
- **FCFS e Prioridade não-preemptiva produzem o mesmo Gantt** neste
  cenário: P1 já estava em execução quando P2 (prioridade 1) chegou, e
  como a Prioridade não-preemptiva não interrompe o processo corrente,
  P1 continuou até o fim. A coluna *Prioridade* só passa a influenciar
  na escolha entre os processos que chegam durante a execução de P1.
- **Round Robin oferece o segundo melhor tempo de resposta** (atrás
  apenas do SRTF), ao custo de um turnaround maior para o processo
  mais longo (P1 acaba só no instante 22, igual ao FCFS).
- **Prioridade preemptiva** consegue equilíbrio razoável, mas penaliza
  P4 (prioridade 4, a mais baixa), que só executa ao final.
- **Quantum maior em RR (q=4)** reduz a frequência de troca de contexto
  e prejudica processos curtos: P4 espera de 3 a 12 (espera = 9), contra
  espera = 5 com q=2. Em compensação, o tempo de resposta de processos
  longos melhora.

### 9.2. Caso 2 — três processos com E/S

Conjunto de entrada:

| PID | Chegada | Prioridade | Bursts            |
|-----|---------|------------|-------------------|
| P1  | 0       | 2          | C4, I3, C2        |
| P2  | 1       | 1          | C5                |
| P3  | 3       | 3          | C3, I2, C2        |

Resultados:

| Algoritmo               | Tempo total | CPU%   | Turnaround médio | Espera média | Resposta média |
|-------------------------|-------------|--------|------------------|--------------|----------------|
| FCFS                    | 16          | 100.00 | 11.67            | 4.67         | 3.00           |
| SJF não-preemptivo      | 16          | 100.00 | 10.67            | 3.67         | 3.67           |
| SJF preemptivo (SRTF)   | 16          | 100.00 | 10.67            | 3.67         | 3.67           |
| Prioridade não-preempt. | **18**      | **88.89** | 11.33         | 4.33         | 3.67           |
| Prioridade preemptiva   | 16          | 100.00 | **10.67**        | **3.67**     | **2.00**       |
| Round Robin (q=2)       | 16          | 100.00 | 13.00            | 6.00         | 1.33           |

Observações:

- **A Prioridade não-preemptiva apresenta CPU ociosa** (88.89% de
  utilização). Ao escolher P3 só após o término total de P1, fica sem
  ninguém para escalonar enquanto P3 está bloqueado em E/S, gerando
  dois ticks de ociosidade. Os demais algoritmos sobrepõem o
  bloqueio de E/S de um processo com a CPU de outro.
- **A preempção por prioridade aproveita melhor a E/S**: P2 (maior
  prioridade) preempta P1 logo no início, P1 retoma quando P2
  termina e libera a CPU para P3 enquanto está em E/S, mantendo
  100% de utilização.
- **SJF preemptivo e SJF não-preemptivo coincidem** neste cenário
  porque, no momento de cada decisão, o processo escolhido é também
  aquele com menor burst restante; nenhuma preempção dispara.
- **Round Robin sacrifica o turnaround pelo tempo de resposta**:
  obtém o menor R (1.33) mas o maior turnaround médio, pois a
  rotação frequente atrasa cada processo.

### 9.3. Síntese qualitativa

| Critério             | Melhor candidato típico             | Pior candidato típico |
|----------------------|-------------------------------------|-----------------------|
| Turnaround médio     | SJF preemptivo (SRTF)               | FCFS                  |
| Espera média         | SJF preemptivo (SRTF)               | FCFS                  |
| Tempo de resposta    | Round Robin (quantum pequeno)       | FCFS / SJF NP         |
| Utilização de CPU    | Algoritmos preemptivos com E/S      | Prioridade NP com E/S |
| Justiça (fairness)   | Round Robin                         | Prioridade (starvation) |
| Simplicidade         | FCFS                                | SJF preemptivo        |

A escolha do algoritmo, portanto, depende do critério a ser otimizado
e da natureza da carga (intensiva em CPU, mista, interativa). Não
existe um algoritmo dominante em todas as dimensões.

## 10. Limitações e trabalhos futuros

- O simulador opera com tempo discreto unitário. Bursts não inteiros
  exigiriam reformulação do laço.
- Não há simulação de múltiplas filas (multilevel queue) nem
  envelhecimento de prioridade — em cargas adversas o algoritmo de
  Prioridade pode causar *starvation*, o que fica evidente quando se
  utiliza o limite de tempo de simulação descrito na Seção 3.
- O modelo de E/S assume um único dispositivo: processos em BLOCKED
  têm seus bursts decrementados em paralelo, sem disputa por
  recurso de E/S.
