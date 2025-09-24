/**
 * @file simulador_so.cpp
 * @brief Implementação de um simulador de sistema operacional em C++.
 *
 * Este simulador modela um escalonador de processos com o algoritmo de Sorteio (Lottery)
 * e um subsistema de gerenciamento de dispositivos de Entrada e Saída (E/S).
 * Ele lê uma configuração de um arquivo, simula a execução de processos
 * e, ao final, apresenta métricas de desempenho detalhadas.
 *
 * FUNCIONALIDADES PRINCIPAIS:
 * - Escalonamento de processos usando algoritmo de Sorteio (Lottery Scheduling)
 * - Gerenciamento de dispositivos de E/S com limites de uso simultâneo
 * - Filas de espera para dispositivos ocupados
 * - Simulação probabilística de solicitações de E/S durante execução
 * - Relatórios detalhados de desempenho (tempo de retorno, CPU, espera)
 *
 * FLUXO GERAL DO SISTEMA:
 * 1. Leitura do arquivo de configuração (processos, dispositivos, parâmetros)
 * 2. Inicialização das estruturas de dados (filas, dispositivos)
 * 3. Loop principal da simulação:
 * a. Verificação de chegada de novos processos
 * b. Verificação de conclusão de operações de E/S
 * c. Gerenciamento de filas de espera dos dispositivos
 * d. Seleção e execução de processos na CPU
 * e. Tratamento de solicitações de E/S
 * f. Avanço do tempo da simulação
 * 4. Geração de relatório final com métricas de desempenho
 *
 * @author Seu Nome/Grupo Aqui
 * @date 24/09/2025
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <list>
#include <cmath>

// =====================================================================================
// DEFINIÇÕES E ESTRUTURAS BÁSICAS
// =====================================================================================

// Enum para os estados do processo
enum ProcessState {
    PRONTO,      // Processo pronto para executar
    EXECUTANDO,  // Processo atualmente na CPU
    BLOQUEADO,   // Processo aguardando E/S
    TERMINADO    // Processo finalizado
};

// Forward declaration para a classe Process
class Process;

// Estrutura para representar um dispositivo de E/S
struct DeviceInfo {
    int id;                       // Identificador único do dispositivo
    int numUsosSimultaneos;       // Número máximo de processos simultâneos
    int tempoOperacao;            // Tempo que o dispositivo leva para processar uma operação
    std::list<Process*> usuarios_ativos;  // Processos atualmente usando o dispositivo
    std::list<Process*> fila_de_espera;   // Processos aguardando para usar o dispositivo
};

// Estrutura de configuração global da simulação
struct Config {
    std::string scheduling_algorithm; // Algoritmo de escalonamento (ex: "sorteio")
    int cpu_fraction;                 // Quantum de tempo da CPU
    int numDispositivosES;            // Número total de dispositivos de E/S
};


// =====================================================================================
// CLASSE PROCESS (BLOCO DE CONTROLE DE PROCESSO)
// =====================================================================================

/**
 * @class Process
 * @brief Representa um processo no sistema operacional.
 *
 * Esta classe modela um processo com todas as suas características:
 * - Identificação (PID)
 * - Tempos de execução e criação
 * - Estado atual (Pronto, Executando, Bloqueado, Terminado)
 * - Recursos necessários (bilhetes para sorteio, chance de E/S)
 * - Métricas de desempenho coletadas durante a execução
 */
class Process {
public:
    // Atributos lidos do arquivo
    int pid;
    int creation_time;
    int burst_time;
    int tickets; // Usado como prioridade no Sorteio
    double chanceRequisitarES;

    // Atributos dinâmicos da simulação
    int remaining_time;
    ProcessState state;
    int device_id_blocked = -1;
    int io_completion_time = -1;

    // Métricas de desempenho
    int start_time = -1;
    int end_time = -1;
    int time_in_ready = 0;
    int time_in_blocked = 0;

    Process(int p, int ct, int bt, int t, double chance_es)
        : pid(p), creation_time(ct), burst_time(bt), tickets(t), chanceRequisitarES(chance_es) {
        remaining_time = burst_time;
        state = PRONTO; // Estado inicial conceitual
    }

    // Função para obter a string correspondente ao estado
    std::string getStateString() const {
        switch (state) {
            case PRONTO: return "Pronto";
            case EXECUTANDO: return "Executando";
            case BLOQUEADO: return "Bloqueado";
            case TERMINADO: return "Terminado";
            default: return "Desconhecido";
        }
    }
};

// =====================================================================================
// CLASSE LOTTERYSCHEDULER (O CORAÇÃO DO SIMULADOR)
// =====================================================================================

/**
 * @class LotteryScheduler
 * @brief Implementa o escalonador por Sorteio com gerenciamento de E/S.
 *
 * Esta classe é o núcleo do simulador. Ela gerencia:
 * - As filas de processos (prontos, futuros, finalizados)
 * - A seleção de processos via algoritmo de sorteio
 * - O controle de quantum de tempo para cada processo
 * - O gerenciamento de dispositivos de E/S e suas filas de espera
 * - A simulação temporal da execução e a coleta de métricas
 */
class LotteryScheduler {
public:
    // --- INTERFACE PÚBLICA ---
    LotteryScheduler();
    void set_quantum(int q);
    void add_process(Process* process);
    void add_device(const DeviceInfo& device);
    void run();

private:
    // --- LÓGICA INTERNA DO ESCALONADOR ---
    void update_process_states();   // Atualiza métricas de tempo dos processos
    void check_for_arrivals();      // Verifica chegada de novos processos
    void check_for_io_completions();// Verifica conclusão de operações E/S
    void service_device_queues();   // Move processos das filas de espera para os dispositivos
    void handle_running_process();  // Gerencia execução do processo na CPU
    void dispatch_new_process();    // Seleciona novo processo para a CPU

    // --- FUNÇÕES DE RELATÓRIO ---
    void print_system_state();      // Imprime estado atual do sistema
    void print_final_report();      // Imprime relatório final de desempenho
    
    // --- LÓGICA DO ALGORITMO ---
    Process* select_winner();       // Seleciona processo via sorteio

    // --- ESTRUTURAS DE DADOS INTERNAS ---
    std::vector<Process*> all_processes;      // Vetor com todos os processos do sistema
    std::list<Process*> future_queue;         // Lista de processos que ainda não chegaram
    std::list<Process*> ready_queue;          // Fila de processos prontos para executar
    std::vector<Process*> finished_processes; // Vetor de processos já finalizados
    Process* running_process = nullptr;       // Ponteiro para o processo na CPU
    
    std::vector<DeviceInfo> devices;          // Vetor com todos os dispositivos de E/S
    
    int quantum;            // Quantum de tempo da CPU
    int quantum_timer = 0;  // Contador do quantum atual
    int current_time = 0;   // Tempo atual da simulação
};

// --- IMPLEMENTAÇÃO DOS MÉTODOS DE LOTTERYSCHEDULER ---

LotteryScheduler::LotteryScheduler() = default;

void LotteryScheduler::set_quantum(int q) { quantum = q; }

void LotteryScheduler::add_process(Process* process) {
    all_processes.push_back(process);
    future_queue.push_back(process);
    // Ordena a lista de futuros processos por tempo de chegada para otimizar a verificação
    future_queue.sort([](const Process* a, const Process* b) {
        return a->creation_time < b->creation_time;
    });
}

void LotteryScheduler::add_device(const DeviceInfo& device) { devices.push_back(device); }

/**
 * @brief Seleciona o processo vencedor baseado no número de bilhetes.
 * @return Ponteiro para o processo vencedor ou nullptr se a fila de prontos estiver vazia.
 */
Process* LotteryScheduler::select_winner() {
    if (ready_queue.empty()) {
        return nullptr;
    }

    int total_tickets = 0;
    for (const auto& process : ready_queue) {
        total_tickets += process->tickets;
    }

    if (total_tickets == 0) return ready_queue.front(); // Caso de fallback

    int winning_ticket = std::rand() % total_tickets;
    int current_ticket_sum = 0;
    for (auto& process : ready_queue) {
        current_ticket_sum += process->tickets;
        if (winning_ticket < current_ticket_sum) {
            return process;
        }
    }
    return nullptr; // Não deve ser alcançado se houver bilhetes
}

/**
 * @brief Imprime o estado atual de todas as filas e dispositivos.
 * Chamada a cada troca de contexto.
 */
void LotteryScheduler::print_system_state() {
    std::cout << "\n--- Tempo: " << current_time << " ---\n";

    if (running_process) {
        std::cout << "Executando: PID " << running_process->pid 
                  << " (CPU restante: " << running_process->remaining_time << ")\n";
    } else {
        std::cout << "Executando: Nenhum\n";
    }

    std::cout << "Prontos    : ";
    if (ready_queue.empty()) {
        std::cout << "Nenhum";
    } else {
        for (const auto& p : ready_queue) {
            std::cout << "P" << p->pid << "(" << p->remaining_time << ") ";
        }
    }
    std::cout << "\n";

    std::cout << "Bloqueados : ";
    bool any_blocked = false;
    for (const auto& dev : devices) {
        for (const auto& p : dev.usuarios_ativos) {
            std::cout << "P" << p->pid << "(D" << dev.id << ") ";
            any_blocked = true;
        }
        for (const auto& p : dev.fila_de_espera) {
            std::cout << "P" << p->pid << "(D" << dev.id << ") ";
            any_blocked = true;
        }
    }
    if (!any_blocked) {
        std::cout << "Nenhum";
    }
    std::cout << "\n\n";

    for (const auto& dev : devices) {
        std::cout << "Dispositivo " << dev.id << " (Uso: " << dev.usuarios_ativos.size() << "/" << dev.numUsosSimultaneos
                  << ", Fila: " << dev.fila_de_espera.size() << ")\n";
        std::cout << "  - Utilizando: ";
        if(dev.usuarios_ativos.empty()) std::cout << "Nenhum";
        else for(const auto& p : dev.usuarios_ativos) std::cout << "P" << p->pid << " ";
        std::cout << "\n";
        
        std::cout << "  - Esperando : ";
        if(dev.fila_de_espera.empty()) std::cout << "Nenhum";
        else for(const auto& p : dev.fila_de_espera) std::cout << "P" << p->pid << " ";
        std::cout << "\n";
    }
    std::cout << "----------------------------------------\n";
}

void LotteryScheduler::check_for_arrivals() {
    auto it = future_queue.begin();
    while (it != future_queue.end()) {
        if ((*it)->creation_time <= current_time) {
            (*it)->state = PRONTO;
            ready_queue.push_back(*it);
            it = future_queue.erase(it);
        } else {
            break; // Otimização: a lista está ordenada
        }
    }
}

void LotteryScheduler::check_for_io_completions() {
    for (auto& dev : devices) {
        auto it = dev.usuarios_ativos.begin();
        while (it != dev.usuarios_ativos.end()) {
            Process* p = *it;
            if (p->io_completion_time <= current_time) {
                p->state = PRONTO;
                p->device_id_blocked = -1;
                ready_queue.push_back(p);
                it = dev.usuarios_ativos.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void LotteryScheduler::service_device_queues() {
    for (auto& dev : devices) {
        while (dev.usuarios_ativos.size() < (size_t)dev.numUsosSimultaneos && !dev.fila_de_espera.empty()) {
            Process* p = dev.fila_de_espera.front();
            dev.fila_de_espera.pop_front();
            p->io_completion_time = current_time + dev.tempoOperacao;
            dev.usuarios_ativos.push_back(p);
        }
    }
}

void LotteryScheduler::handle_running_process() {
    if (!running_process) return;

    running_process->remaining_time--;
    quantum_timer--;

    if (running_process->remaining_time <= 0) {
        running_process->state = TERMINADO;
        running_process->end_time = current_time + 1;
        finished_processes.push_back(running_process);
        std::cout << ">>> Processo " << running_process->pid << " finalizado no tempo " << running_process->end_time << " <<<\n";
        running_process = nullptr;
    } else if (quantum_timer <= 0) {
        running_process->state = PRONTO;
        ready_queue.push_back(running_process);
        running_process = nullptr;
    }
}

void LotteryScheduler::dispatch_new_process() {
    if (running_process || ready_queue.empty()) return;

    Process* winner = select_winner();
    ready_queue.remove(winner);
    
    running_process = winner;
    running_process->state = EXECUTANDO;
    if (running_process->start_time == -1) {
        running_process->start_time = current_time;
    }
    
    quantum_timer = quantum;
    print_system_state();

    double chance = static_cast<double>(rand()) / RAND_MAX;
    if (chance < running_process->chanceRequisitarES) {
        int device_idx = rand() % devices.size();
        
        // Simula o consumo de 1 unidade de CPU antes de bloquear
        running_process->remaining_time--;
        if(running_process->remaining_time <= 0) {
            running_process->state = TERMINADO;
            running_process->end_time = current_time + 1;
            finished_processes.push_back(running_process);
            std::cout << ">>> Processo " << running_process->pid << " finalizado no tempo " << running_process->end_time << " <<<\n";
            running_process = nullptr;
            return;
        }

        std::cout << "!!! Processo " << running_process->pid << " solicitou E/S para o dispositivo " << devices[device_idx].id << " !!!\n";

        running_process->state = BLOQUEADO;
        running_process->device_id_blocked = device_idx;
        
        DeviceInfo& dev = devices[device_idx];
        if (dev.usuarios_ativos.size() < (size_t)dev.numUsosSimultaneos) {
            running_process->io_completion_time = current_time + 1 + dev.tempoOperacao;
            dev.usuarios_ativos.push_back(running_process);
        } else {
            dev.fila_de_espera.push_back(running_process);
        }
        
        running_process = nullptr; // Libera a CPU
    }
}

/**
 * @brief Atualiza as métricas de tempo de espera dos processos (CORRIGIDO).
 */
void LotteryScheduler::update_process_states() {
    for (auto* p : all_processes) {
        // Apenas incrementa os contadores se o processo AINDA NÃO TERMINOU.
        if (p->state != TERMINADO) {
            if (p->state == PRONTO) {
                p->time_in_ready++;
            } else if (p->state == BLOQUEADO) {
                p->time_in_blocked++;
            }
        }
    }
}

/**
 * @brief O laço principal da simulação.
 */
void LotteryScheduler::run() {
    std::srand(time(0));
    std::cout << "--- Simulacao do Escalonador Sorteio com E/S ---\n\n";

    while (finished_processes.size() < all_processes.size()) {
        check_for_arrivals();
        check_for_io_completions();
        service_device_queues();

        if (!running_process) {
            dispatch_new_process();
        }

        handle_running_process();
        update_process_states();

        current_time++;

        if (current_time > 10000) { // Trava de segurança contra loops infinitos
            std::cerr << "Simulacao excedeu o tempo limite!" << std::endl;
            break;
        }
    }

    std::cout << "\n--- Simulacao do Escalonador finalizada no tempo " << current_time << " ---\n\n";
    print_final_report();
}

void LotteryScheduler::print_final_report() {
    std::cout << "\n\n========================= Relatorio Final ========================\n";
    std::cout << std::left << std::setw(6) << "PID"
              << std::setw(18) << "| Tempo de Retorno"
              << std::setw(15) << "| Tempo de CPU"
              << std::setw(18) << "| Tempo em Pronto"
              << std::setw(18) << "| Tempo Bloqueado" << "\n";
    std::cout << "------------------------------------------------------------------\n";

    std::sort(finished_processes.begin(), finished_processes.end(), [](const Process* a, const Process* b) {
        return a->pid < b->pid;
    });

    for (const auto& p : finished_processes) {
        int turnaround_time = p->end_time - p->creation_time;
        std::cout << std::left << std::setw(6) << p->pid
                  << "| " << std::setw(16) << turnaround_time
                  << "| " << std::setw(13) << p->burst_time
                  << "| " << std::setw(16) << p->time_in_ready
                  << "| " << std::setw(16) << p->time_in_blocked
                  << "\n";
    }
    std::cout << "==================================================================\n";
}


// =====================================================================================
// FUNÇÃO DE LEITURA E FUNÇÃO PRINCIPAL (MAIN)
// =====================================================================================

/**
 * @brief Lê o arquivo de entrada e popula as estruturas de dados do simulador.
 * @return True se a leitura for bem-sucedida, false caso contrário.
 */
bool read_file(const std::string& filename, Config& config, LotteryScheduler& scheduler) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo: " << filename << std::endl;
        return false;
    }

    std::string line;

    // 1. Ler a linha de configuração
    if (!std::getline(file, line)) return false;
    std::stringstream ss_config(line);
    std::string token;
    
    std::getline(ss_config, config.scheduling_algorithm, '|');
    std::getline(ss_config, token, '|'); config.cpu_fraction = std::stoi(token);
    std::getline(ss_config, token, '|'); // políticaMemória (ignorado)
    std::getline(ss_config, token, '|'); // tamanhoMemória (ignorado)
    std::getline(ss_config, token, '|'); // tamanhoPáginasMolduras (ignorado)
    std::getline(ss_config, token, '|'); // percentualAlocação (ignorado)
    std::getline(ss_config, token, '|'); config.numDispositivosES = std::stoi(token);

    // 2. Ler as linhas dos dispositivos de E/S
    for (int i = 0; i < config.numDispositivosES; ++i) {
        if (!std::getline(file, line)) return false;
        std::stringstream ss_device(line);
        DeviceInfo dev{};
        std::getline(ss_device, token, '|'); 
        size_t dash_pos = token.find('-');
        if (dash_pos != std::string::npos) {
            token = token.substr(dash_pos + 1);
        }
        dev.id = std::stoi(token);
        std::getline(ss_device, token, '|'); dev.numUsosSimultaneos = std::stoi(token);
        std::getline(ss_device, token, '|'); dev.tempoOperacao = std::stoi(token);
        scheduler.add_device(dev);
    }

    // 3. Ler as linhas dos processos
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss_process(line);
        
        int creation_time, pid_arquivo, execution_time, priority, memory;
        double chance_es;
        
        std::getline(ss_process, token, '|'); creation_time = std::stoi(token);
        std::getline(ss_process, token, '|'); pid_arquivo = std::stoi(token);
        std::getline(ss_process, token, '|'); execution_time = std::stoi(token);
        std::getline(ss_process, token, '|'); priority = std::stoi(token); // bilhetes
        std::getline(ss_process, token, '|'); memory = std::stoi(token); // qtdeMemoria (ignorado)
        std::getline(ss_process, token, '|'); // sequenciaAcessoPaginas (ignorado)
        
        // CORREÇÃO: Converte o valor lido (ex: 88) para uma probabilidade (0.88)
        std::getline(ss_process, token, '|'); chance_es = std::stod(token) / 100.0;
        
        scheduler.add_process(new Process(pid_arquivo, creation_time, execution_time, priority, chance_es));
    }

    file.close();
    return true;
}


int main(int argc, char* argv[]) {
    std::string filename;
    
    if (argc >= 2) {
        filename = argv[1];
        // Tenta múltiplas possibilidades se o arquivo não for encontrado diretamente
        std::ifstream test_file(filename);
        if (!test_file.is_open()) {
            // Tenta no diretório pai
            std::string alt_filename = "../" + filename;
            test_file.open(alt_filename);
            if (test_file.is_open()) {
                filename = alt_filename;
                test_file.close();
            } else {
                std::cerr << "Erro ao abrir o arquivo: " << argv[1] << std::endl;
                return 1;
            }
        } else {
            test_file.close();
        }
    } else {
        // Modo padrão: tenta encontrar entrada_ES.txt
        std::string possible_paths[] = {
            "entrada_ES.txt",                    // Mesmo diretório
            "../entrada_ES.txt",                 // Diretório pai
            "./entrada_ES.txt",                  // Mesmo diretório com ./
        };
        
        filename = "";
        for (const auto& path : possible_paths) {
            std::ifstream test_file(path);
            if (test_file.is_open()) {
                filename = path;
                test_file.close();
                break;
            }
        }
        
        if (filename.empty()) {
            std::cerr << "Erro: Não foi possível encontrar o arquivo entrada_ES.txt" << std::endl;
            std::cerr << "Uso: " << argv[0] << " <arquivo_de_entrada>" << std::endl;
            return 1;
        }
    }

    Config config;
    LotteryScheduler scheduler;

    if (!read_file(filename, config, scheduler)) {
        return 1;
    }

    scheduler.set_quantum(config.cpu_fraction);
    scheduler.run();
    
    // NOTA: Em C++ moderno, seria ideal usar std::unique_ptr para gerenciar a memória
    // dos processos e evitar a necessidade de limpeza manual. Para este projeto,
    // a alocação com 'new' é suficiente, mas a memória não é liberada.
    
    return 0;
}

