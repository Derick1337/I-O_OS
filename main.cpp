#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <queue>
#include <iomanip>

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

bool read_file(const std::string &filename, Config &config, std::vector<DeviceInfo> &devices, std::vector<ProcessInfo> &processes)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Erro ao abrir o arquivo: " << filename << std::endl;
        return false;
    }

    std::string line;
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
        }
    }

    file.close();
    return true;
}

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
        return 1;

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
