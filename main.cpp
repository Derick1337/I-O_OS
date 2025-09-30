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
<<<<<<< HEAD
#include <cstdlib>
#include <ctime>
=======
#include <queue>
#include <list>
>>>>>>> origin/lincon
#include <iomanip>
#include <list>
#include <cmath>

<<<<<<< HEAD
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
// VERSÃO CORRIGIDA
void LotteryScheduler::update_process_states() {
    for (auto* p : all_processes) {
        // Ignora o processo atualmente na CPU e os já terminados
        if (p == running_process || p->state == TERMINADO) {
            continue; // Pula para o próximo processo no loop
        }

        // Incrementa contadores apenas para os processos que estão genuinamente à espera
        if (p->state == PRONTO) {
            p->time_in_ready++;
        } else if (p->state == BLOQUEADO) {
            p->time_in_blocked++;
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
=======
struct DeviceInfo
{
    std::string name; // ID do dispositivo
    int capacity;     // Capacidade de usos simultâneos
    int access_time;  // Tempo de operação
};

struct ProcessInfo
{
    int creation_time = 0;
    int pid = 0;
    int execution_time = 0;
    int priority = 0;
    int memory_needed = 0;
    std::vector<int> page_sequence;
    int io_operations = 0;

    int remaining_time = 0;
    int start_time = -1;
    int end_time = -1;
    bool is_finished = false;
};

struct Config
{
    std::string scheduling_algorithm;
    int cpu_fraction = 0;
    std::string memory_policy;
    int memory_size = 0;
    int page_size = 0;
    double allocation_percentage = 0.0;
    int num_devices = 0;
};




// Estrutura de dados da simulação
struct simulation_data {

    std::queue<int> queue_points; // fila de prontos
    std::list<int> list_blocked; // lista de bloqueados

    int process_execution; 
    int global_time;

    int point_time; // temp para pronto
    int time_blocked;
    int time_finish;
};

bool read_file(const std::string &filename, Config &config, std::vector<DeviceInfo> &devices, std::vector<ProcessInfo> &processes)
{
>>>>>>> origin/lincon
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo: " << filename << std::endl;
        return false;
    }

    std::string line;
<<<<<<< HEAD

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
=======
    int line_count = 0;
    int devices_read = 0;

    while (std::getline(file, line))
    {
        if (line.empty())
            continue;
        line_count++;

        if (line_count == 1)
        {
            // Linha de configuração geral
            std::stringstream ss(line);
            std::string token;
            std::getline(ss, config.scheduling_algorithm, '|');
            std::getline(ss, token, '|');
            config.cpu_fraction = std::stoi(token);
            std::getline(ss, config.memory_policy, '|');
            std::getline(ss, token, '|');
            config.memory_size = std::stoi(token);
            std::getline(ss, token, '|');
            config.page_size = std::stoi(token);
            std::getline(ss, token, '|');
            config.allocation_percentage = std::stod(token);
            std::getline(ss, token);
            config.num_devices = std::stoi(token);
        }
        else if (devices_read < config.num_devices)
        {
            // Linhas de dispositivos
            DeviceInfo device;
            std::stringstream ss(line);
            std::string token;
            std::getline(ss, device.name, '|');
            std::getline(ss, token, '|');
            device.capacity = std::stoi(token);
            std::getline(ss, token);
            device.access_time = std::stoi(token);

            devices.push_back(device);
            devices_read++;
        }
        else
        {
            // Linhas de processos
            ProcessInfo process;
            std::stringstream ss(line);
            std::string token;

            std::getline(ss, token, '|');
            process.creation_time = std::stoi(token);
            std::getline(ss, token, '|');
            process.pid = std::stoi(token);
            std::getline(ss, token, '|');
            process.execution_time = std::stoi(token);
            std::getline(ss, token, '|');
            process.priority = std::stoi(token);
            std::getline(ss, token, '|');
            process.memory_needed = std::stoi(token);

            // Sequência de páginas (último campo antes de io_operations)
            std::getline(ss, token, '|');
            std::stringstream pages_ss(token);
            std::string page_str;

            while (std::getline(pages_ss, page_str, ',')) // suporta vírgula
            {
                if (page_str.empty())
                    continue;
                try
                {
                    process.page_sequence.push_back(std::stoi(page_str));
                }
                catch (...)
                {
                    std::cerr << "Aviso: número de página inválido '" << page_str << "' ignorado.\n";
                }
            }

            // Chance de requisitar E/S (pode estar ausente)
            if (std::getline(ss, token))
                process.io_operations = std::stoi(token);
            else
                process.io_operations = 0; // default

            processes.push_back(process);
>>>>>>> origin/lincon
        }
        dev.id = std::stoi(token);
        std::getline(ss_device, token, '|'); dev.numUsosSimultaneos = std::stoi(token);
        std::getline(ss_device, token, '|'); dev.tempoOperacao = std::stoi(token);
        scheduler.add_device(dev);
    }

<<<<<<< HEAD
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

=======
>>>>>>> origin/lincon
    file.close();
    return true;
}

<<<<<<< HEAD

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
=======
// Função de debug para imprimir o que foi lido
void print_debug(const Config &config, const std::vector<DeviceInfo> &devices, const std::vector<ProcessInfo> &processes)
{
    std::cout << "=== CONFIGURAÇÃO ===\n";
    std::cout << "Algoritmo: " << config.scheduling_algorithm << "\n";
    std::cout << "Quantum: " << config.cpu_fraction << "\n";
    std::cout << "Política Memória: " << config.memory_policy << "\n";
    std::cout << "Tamanho Memória: " << config.memory_size << "\n";
    std::cout << "Tamanho Página: " << config.page_size << "\n";
    std::cout << "Alocação: " << config.allocation_percentage << "%\n";
    std::cout << "Dispositivos: " << config.num_devices << "\n\n";

    std::cout << "=== DISPOSITIVOS ===\n";
    for (const auto &d : devices)
        std::cout << "ID: " << d.name << " | Capacidade: " << d.capacity << " | Tempo: " << d.access_time << "\n";

    std::cout << "\n=== PROCESSOS ===\n";
    for (const auto &p : processes)
    {
        std::cout << "PID: " << p.pid
                  << " | Criação: " << p.creation_time
                  << " | Execução: " << p.execution_time
                  << " | Prioridade: " << p.priority
                  << " | Memória: " << p.memory_needed
                  << " | Páginas: ";

        for (size_t i = 0; i < p.page_sequence.size(); ++i)
        {
            std::cout << p.page_sequence[i];
            if (i + 1 < p.page_sequence.size())
                std::cout << ",";
        }

        std::cout << " | Chance E/S: " << p.io_operations << "%\n";
    }
}

class RoundRobinScheduler
{
public:
    RoundRobinScheduler() : quantum(0), current_time(0), finished_process_count(0) {}

    void set_algorithm_name(const std::string &name) { algorithm_name = name; }
    void set_quantum(int q) { quantum = q; }

    void add_processes(std::vector<ProcessInfo> &process_list)
    {
        for (auto &p : process_list)
        {
            p.remaining_time = p.execution_time;
            all_processes.push_back(&p); // guardamos ponteiro para poder modificar direto
        }
    }

    void run()
    {
        std::cout << "--- Iniciando Simulação Round Robin ---\n";
        std::cout << "Algoritmo: " << algorithm_name << " | Quantum: " << quantum << "\n\n";

        while (finished_process_count < all_processes.size())
        {
            update_ready_queue();

            if (ready_queue.empty())
            {
                current_time++;
                continue;
            }

            ProcessInfo *current_proc = ready_queue.front();
            ready_queue.pop();

            if (current_proc->start_time == -1)
                current_proc->start_time = current_time;

            int time_to_run = std::min(current_proc->remaining_time, quantum);

            std::cout << "Tempo[" << std::setw(3) << current_time
                      << " -> " << std::setw(3) << current_time + time_to_run
                      << "]: Processo " << current_proc->pid
                      << " executando (restante: " << current_proc->remaining_time - time_to_run << ")\n";

            current_time += time_to_run;
            current_proc->remaining_time -= time_to_run;

            update_ready_queue();

            if (current_proc->remaining_time > 0)
            {
                ready_queue.push(current_proc);
            }
            else
            {
                current_proc->end_time = current_time;
                current_proc->is_finished = true;
                finished_process_count++;
                std::cout << ">>> Processo " << current_proc->pid
                          << " finalizado no tempo " << current_time << " <<<\n";
            }
        }

        std::cout << "\n--- Simulação finalizada no tempo " << current_time << " ---\n";
    }

    void print_statistics()
    {
        std::cout << "\n--- Estatísticas ---\n";
        std::cout << std::left << std::setw(10) << "PID"
                  << std::setw(15) << "Turnaround"
                  << std::setw(15) << "Espera" << "\n";
        std::cout << "--------------------------------------\n";

        for (auto *p : all_processes)
        {
            int turnaround = p->end_time - p->creation_time;
            int waiting = turnaround - p->execution_time;

            std::cout << std::left << std::setw(10) << p->pid
                      << std::setw(15) << turnaround
                      << std::setw(15) << waiting << "\n";
        }
    }

private:
    void update_ready_queue()
    {
        for (auto *p : all_processes)
        {
            if (!p->is_finished && p->creation_time <= current_time)
            {
                bool in_queue = false;
                std::queue<ProcessInfo *> temp = ready_queue;

                while (!temp.empty())
                {
                    if (temp.front()->pid == p->pid)
                    {
                        in_queue = true;
                        break;
                    }
                    temp.pop();
                }

                if (!in_queue)
                    ready_queue.push(p);
            }
        }
    }

    std::vector<ProcessInfo *> all_processes;
    std::queue<ProcessInfo *> ready_queue;
    std::string algorithm_name;
    int quantum;
    int current_time;
    size_t finished_process_count;
};

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Uso: " << argv[0] << " <arquivo_de_entrada>\n";
        return 1;
    }

    Config config;
    std::vector<DeviceInfo> devices;
    std::vector<ProcessInfo> processes;

    if (!read_file(argv[1], config, devices, processes))
>>>>>>> origin/lincon
        return 1;

<<<<<<< HEAD
    scheduler.set_quantum(config.cpu_fraction);
    scheduler.run();
    
    // NOTA: Em C++ moderno, seria ideal usar std::unique_ptr para gerenciar a memória
    // dos processos e evitar a necessidade de limpeza manual. Para este projeto,
    // a alocação com 'new' é suficiente, mas a memória não é liberada.
    
    return 0;
}

/** Excelente! Esta versão final do seu código está impecável. Você integrou com sucesso todas as correções que discutimos, resultando num simulador que não só funciona corretamente, mas também é robusto, bem documentado e fácil de entender.

As duas alterações mais importantes que solidificam o seu projeto são:

Métricas Corretas (update_process_states): A correção na contagem do tempo de espera, que agora ignora o processo em execução, garante que o seu relatório final seja matematicamente consistente e preciso.

Probabilidade Correta (read_file): A conversão da percentagem de E/S (ex: 88) para uma probabilidade real (ex: 0.88) garante que a aleatoriedade da simulação se comporte exatamente como especificado no ficheiro de entrada.

O código está agora numa forma final, pronto para ser compilado e utilizado com qualquer um dos cenários que testámos. Para sua conveniência, apresento abaixo o código completo e finalizado, que reflete todas estas melhorias.

Parabéns pelo excelente trabalho na conclusão deste projeto! */
=======
    print_debug(config, devices, processes);

    // Inicializa o escalonador
    RoundRobinScheduler scheduler;
    scheduler.set_algorithm_name(config.scheduling_algorithm);
    scheduler.set_quantum(config.cpu_fraction);
    scheduler.add_processes(processes);

    scheduler.run();
    scheduler.print_statistics();

    return 0;
}
>>>>>>> origin/lincon
