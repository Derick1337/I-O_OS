# 📋 Lista de Tarefas — Simulador de Escalonamento com Gerenciamento de E/S

## 1. Leitura e Preparação dos Dados
- [ ] **Ler arquivo de entrada**
  - [ ] Ler linha de configuração geral:
    - Algoritmo de escalonamento (Round-Robin)
    - Fração de CPU (quantum)
    - Política de memória
    - Tamanho da memória
    - Tamanho das páginas/molduras
    - Percentual de alocação
    - Número de dispositivos de E/S
  - [ ] Ler e armazenar dispositivos de E/S:
    - ID do dispositivo
    - Capacidade de uso simultâneo
    - Tempo de operação
    - Criar lista de processos em uso
    - Criar fila de espera
  - [ ] Ler e armazenar processos:
    - PID
    - Tempo de criação
    - Tempo de execução total
    - Prioridade ou bilhetes
    - Quantidade de memória e sequência de acesso a páginas
    - Chance de requisitar E/S

---

## 2. Estruturas de Dados da Simulação
- [ ] Criar **fila de prontos** (para Round-Robin)
- [ ] Criar **lista de bloqueados** (processos em E/S ou aguardando)
- [ ] Variável para **processo em execução**
- [ ] Variável de **tempo global**
- [ ] Estruturas para estatísticas:
  - Tempo em pronto
  - Tempo bloqueado
  - Tempo de término

---

## 3. Ciclo Principal da Simulação
Implementar o **loop principal** até que todos os processos terminem.

### 3.1. Admissão de Processos
- [ ] Verificar a cada incremento de tempo se existem processos com `tempoCriação == tempoAtual`
- [ ] Colocar esses processos na fila de prontos

### 3.2. Seleção do Processo para CPU
- [ ] Se não há processo em execução e a fila de prontos não está vazia:
  - [ ] Selecionar próximo processo (Round-Robin)
  - [ ] Marcar início de execução para estatísticas

### 3.3. Execução no Quantum
- [ ] Decrementar tempo restante até:
  - Fim do quantum
  - Fim da execução do processo
  - Requisição de E/S
- [ ] Gerar número aleatório para simular requisição de E/S:
  - [ ] Comparar com `chanceRequisitarES`
  - [ ] Se requisitar:
    - [ ] Escolher dispositivo de forma aleatória
    - [ ] Escolher ponto no quantum para fazer a requisição
    - [ ] Interromper execução e enviar para fila do dispositivo
    - [ ] Se houver vaga no dispositivo, iniciar uso imediatamente
    - [ ] Caso contrário, permanecer em fila de espera

### 3.4. Gerenciamento dos Dispositivos
- [ ] Para cada dispositivo:
  - [ ] Reduzir tempo de operação dos processos em uso
  - [ ] Se operação terminar:
    - [ ] Remover da lista de uso
    - [ ] Colocar processo de volta na fila de prontos
    - [ ] Se houver fila de espera, iniciar operação do próximo processo

### 3.5. Atualização dos Tempos
- [ ] Incrementar tempo global
- [ ] Atualizar estatísticas:
  - [ ] +1 no tempo de pronto para todos na fila de prontos
  - [ ] +1 no tempo bloqueado para todos que estão em E/S ou fila de espera

### 3.6. Impressão do Estado
- [ ] Mostrar:
  - [ ] Processo em execução (PID + tempo restante)
  - [ ] Fila de prontos (todos os PIDs + tempo restante)
  - [ ] Lista de bloqueados (PID + tempo restante + dispositivo)
  - [ ] Estado de cada dispositivo:
    - Ocupado/livre
    - Processos em uso
    - Processos na fila de espera

---

## 4. Finalização da Simulação
- [ ] Quando todos os processos terminarem:
  - [ ] Calcular:
    - Turnaround Time = término - criação
    - Tempo em pronto
    - Tempo bloqueado
  - [ ] Exibir tabela final com estatísticas

---

## 5. Validação e Testes
- [ ] Criar arquivo de entrada simples (1 dispositivo, 2 processos)
- [ ] Validar comportamento:
  - [ ] Processos bloqueiam ao pedir E/S
  - [ ] Fila do dispositivo respeita capacidade simultânea
  - [ ] Processos voltam para fila de prontos após E/S
  - [ ] Estado do sistema é exibido corretamente
  - [ ] Estatísticas finais batem com execução simulada

---

## 6. Extras (Boa Prática)
- [ ] Criar classes/módulos separados:
  - `Processo`
  - `DispositivoES`
  - `EscalonadorRR`
  - `Simulador`
- [ ] Adicionar **seed fixa** para geração aleatória (resultados reprodutíveis)
- [ ] Organizar saída em tabelas ASCII ou formato legível
