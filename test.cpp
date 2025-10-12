#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <list>
#include <iomanip>
#include <cmath>
#include <random>
#include <algorithm>
#include <chrono>
#include <map>

// -------------------- Estruturas de Dados --------------------
struct DeviceInfo
{
    std::string name; // ID do dispositivo
    int capacity;     // Capacidade de usos simultâneos
    int access_time;  // Tempo de operação (tempo total por requisição)

    // runtime structures
    std::vector<std::pair<int, int>> active; // pairs: (process_index, remaining_time)
    std::queue<int> wait_queue;              // queue of process indices waiting for this device
};

struct ProcessInfo
{
    int creation_time = 0;
    int pid = 0;
    int execution_time = 0;
    int priority = 0;
    int memory_needed = 0;
    std::vector<int> page_sequence;
    int io_operations = 0; // percent chance (0-100)

    // runtime
    int remaining_time = 0;
    int start_time = -1;
    int end_time = -1;
    bool is_finished = false;

    // statistics
    int ready_time = 0;
    int blocked_time = 0;

    // for IO
    bool waiting_for_io = false;
    int io_device_index = -1;
    int io_remaining = 0;
};

struct Config
{
    std::string scheduling_algorithm;
    int cpu_fraction = 0; // quantum
    std::string memory_policy;
    int memory_size = 0;
    int page_size = 0;
    double allocation_percentage = 0.0;
    int num_devices = 0;
};

// -------------------- Leitura do arquivo --------------------
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

            while (pages_ss >> page_str)
            {
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

            // runtime init
            process.remaining_time = process.execution_time;

            processes.push_back(process);
        }
    }

    file.close();
    return true;
}

// -------------------- Função de debug --------------------
void print_debug(const Config &config, const std::vector<DeviceInfo> &devices, const std::vector<ProcessInfo> &processes)
{
    std::cout << "=== CONFIGURACAO ===\n";
    std::cout << "Algoritmo: " << config.scheduling_algorithm << "\n";
    std::cout << "Quantum: " << config.cpu_fraction << "\n";
    std::cout << "Politica Memoria: " << config.memory_policy << "\n";
    std::cout << "Tamanho Memoria: " << config.memory_size << "\n";
    std::cout << "Tamanho Pagina: " << config.page_size << "\n";
    std::cout << "Alocacao: " << config.allocation_percentage << "%\n";
    std::cout << "Dispositivos: " << config.num_devices << "\n\n";

    std::cout << "=== DISPOSITIVOS ===\n";
    for (const auto &d : devices)
        std::cout << "ID: " << d.name << " | Capacidade: " << d.capacity << " | Tempo: " << d.access_time << "\n";

    std::cout << "\n=== PROCESSOS ===\n";
    for (const auto &p : processes)
    {
        std::cout << "PID: " << p.pid
                  << " | Criacao: " << p.creation_time
                  << " | Execucao: " << p.execution_time
                  << " | Prioridade: " << p.priority
                  << " | Memoria: " << p.memory_needed
                  << " | Paginas: ";

        for (size_t i = 0; i < p.page_sequence.size(); ++i)
        {
            std::cout << p.page_sequence[i];
            if (i + 1 < p.page_sequence.size())
                std::cout << ",";
        }

        std::cout << " | Chance E/S: " << p.io_operations << "%\n";
    }
}

// -------------------- Manager / Simulador --------------------
class Manager
{
public:
    Manager(const Config &cfg, std::vector<DeviceInfo> devs, std::vector<ProcessInfo> procs, unsigned seed = 42)
        : config(cfg), devices(std::move(devs)), processes(std::move(procs)), rng(seed)
    {
        current_time = 0;
        // prepare index order based on creation_time for admission
        for (size_t i = 0; i < processes.size(); ++i)
        {
            process_indices.push_back(i);
        }
        // sort indices by creation_time ascending (stable)
        std::sort(process_indices.begin(), process_indices.end(), [this](int a, int b)
                  {
            if (processes[a].creation_time != processes[b].creation_time)
                return processes[a].creation_time < processes[b].creation_time;
            return processes[a].pid < processes[b].pid; });

        next_admit_idx = 0;
    }

    void run()
    {
        std::cout << "--- Iniciando Simulacao (Manager) ---\n";
        std::cout << "Algoritmo: " << config.scheduling_algorithm << " | Quantum: " << config.cpu_fraction << "\n\n";

        // main loop until all finished
        int total_process_count = (int)processes.size();
        int finished_count = 0;

        // ready queue will contain indices of processes
        while (finished_count < total_process_count)
        {
            // 1) Admit processes whose creation_time == current_time
            admit_new_processes();

            // If no ready process and there is still future work on devices, we still need to advance time.
            if (ready_queue.empty())
            {
                // But maybe some device busy -> we still need to tick devices and blocked times
                tick_devices(1); // advance 1 unit
                increment_blocked_and_ready_times();
                current_time++;
                continue;
            }

            // 2) select next process (Round-Robin: pop front)
            int proc_idx = ready_queue.front();
            ready_queue.pop();

            ProcessInfo &proc = processes[proc_idx];

            if (proc.start_time == -1)
                proc.start_time = current_time;

            // decide if will request IO during this quantum
            bool will_request_io = false;
            int chosen_device = -1;
            int io_at_offset = -1; // when inside quantum it will request IO (1..quantum)
            if (proc.io_operations > 0)
            {
                std::uniform_int_distribution<int> d100(1, 100);
                int roll = d100(rng);
                if (roll <= proc.io_operations)
                {
                    will_request_io = true;
                    // pick a device randomly
                    std::uniform_int_distribution<int> ddev(0, (int)devices.size() - 1);
                    chosen_device = ddev(rng);
                    // pick moment in quantum to request (1..quantum)
                    int q = std::max(1, config.cpu_fraction);
                    std::uniform_int_distribution<int> doff(1, q);
                    io_at_offset = doff(rng);
                }
            }

            // Compute time_to_run = min(remaining, quantum)
            int q = std::max(1, config.cpu_fraction);
            int to_run = std::min(proc.remaining_time, q);

            // if will_request_io and io_at_offset <= to_run, process will be interrupted at that moment
            if (will_request_io && io_at_offset <= to_run)
            {
                // execute until io_at_offset
                int run_time = io_at_offset;
                // print context switch info
                print_state_before_execution(proc_idx, run_time);

                // advance time by run_time, decrement remaining
                for (int t = 0; t < run_time; ++t)
                {
                    // tick devices one by one (they progress in parallel)
                    tick_devices(1);
                    increment_blocked_and_ready_times_except(proc_idx); // process executing shouldn't count as ready
                    current_time++;
                }

                proc.remaining_time -= run_time;

                // process requests IO now -> go to device queue
                send_process_to_device(proc_idx, chosen_device);

                // after moving to device, we must print the state (context switch)
                print_state("Apos requisicao de E/S");
            }
            else
            {
                // execute full to_run (either finished, quantum-end or no IO)
                print_state_before_execution(proc_idx, to_run);

                for (int t = 0; t < to_run; ++t)
                {
                    tick_devices(1);
                    increment_blocked_and_ready_times_except(proc_idx);
                    current_time++;
                }

                proc.remaining_time -= to_run;

                if (proc.remaining_time <= 0)
                {
                    // finished
                    proc.end_time = current_time;
                    proc.is_finished = true;
                    finished_count++;
                    std::cout << ">>> Processo " << proc.pid << " finalizado no tempo " << current_time << " <<<\n";
                    print_state("Apos finalizacao de processo");
                }
                else
                {
                    // not finished -> requeue at ready (RR)
                    ready_queue.push(proc_idx);
                    print_state("Apos retorno a fila de prontos (quantum terminou)");
                }
            }

            // After each scheduling decision we also admit newly created processes that arrived while we ran
            admit_new_processes();

            // update finished count check is at top
            // also continue loop (ready queue may be empty; then we will tick devices)
        }

        // finalize statistics
        std::cout << "\n--- Simulacao finalizada no tempo " << current_time << " ---\n";
        print_final_statistics();
    }

private:
    Config config;
    std::vector<DeviceInfo> devices;
    std::vector<ProcessInfo> processes;

    std::queue<int> ready_queue;
    std::vector<int> process_indices;
    size_t next_admit_idx = 0;

    int current_time = 0;

    // RNG
    std::mt19937 rng;

    // -------------------- Helpers --------------------
    void admit_new_processes()
    {
        // while next process creation_time <= current_time, admit
        while (next_admit_idx < process_indices.size())
        {
            int idx = process_indices[next_admit_idx];
            if (processes[idx].creation_time <= current_time)
            {
                ready_queue.push(idx);
                next_admit_idx++;
            }
            else
                break;
        }
    }

    void tick_devices(int dt)
    {
        // dt is usually 1
        for (size_t di = 0; di < devices.size(); ++di)
        {
            DeviceInfo &dev = devices[di];

            // decrement active operations
            for (auto &pr : dev.active)
            {
                pr.second -= dt; // reduce remaining_time
            }

            // collect finished ones
            std::vector<int> finished_processes;
            for (auto it = dev.active.begin(); it != dev.active.end();)
            {
                if (it->second <= 0)
                {
                    finished_processes.push_back(it->first);
                    it = dev.active.erase(it);
                }
                else
                    ++it;
            }

            // finished processes go back to ready queue
            for (int pidx : finished_processes)
            {
                processes[pidx].waiting_for_io = false;
                processes[pidx].io_device_index = -1;
                processes[pidx].io_remaining = 0;

                // Re-admit to ready queue (at tail)
                if (!processes[pidx].is_finished)
                {
                    ready_queue.push(pidx);
                }
            }

            // start as many waiting processes as capacity allows
            while ((int)dev.active.size() < dev.capacity && !dev.wait_queue.empty())
            {
                int pidx = dev.wait_queue.front();
                dev.wait_queue.pop();
                int op_time = dev.access_time;
                // mark process as active in this device
                dev.active.emplace_back(pidx, op_time);

                // set process runtime io flags
                processes[pidx].io_remaining = op_time;
                processes[pidx].waiting_for_io = true; // remains blocked during op
                processes[pidx].io_device_index = (int)di;
            }
        }
    }

    void send_process_to_device(int proc_idx, int device_index)
    {
        if (device_index < 0 || device_index >= (int)devices.size())
            return;

        ProcessInfo &proc = processes[proc_idx];
        DeviceInfo &dev = devices[device_index];

        // mark as blocked
        proc.waiting_for_io = true;
        proc.io_device_index = device_index;
        // don't set io_remaining yet if queued; if started, set to dev.access_time

        // If device has free capacity -> start immediately
        if ((int)dev.active.size() < dev.capacity)
        {
            dev.active.emplace_back(proc_idx, dev.access_time);
            proc.io_remaining = dev.access_time;
            // proc.blocked_time will be incremented at ticks
        }
        else
        {
            // push to wait queue
            dev.wait_queue.push(proc_idx);
            proc.io_remaining = -1; // indicates waiting in queue
        }

        // Note: process must not be in ready queue; here we assume we removed it before calling
    }

    void increment_blocked_and_ready_times()
    {
        // ready_queue processes
        std::queue<int> tmp = ready_queue;
        while (!tmp.empty())
        {
            int idx = tmp.front();
            tmp.pop();
            processes[idx].ready_time++;
        }

        // blocked processes: includes those active in devices and waiting in device queues
        for (size_t di = 0; di < devices.size(); ++di)
        {
            DeviceInfo &dev = devices[di];
            // active
            for (auto &pr : dev.active)
            {
                processes[pr.first].blocked_time++;
            }
            // waiting
            std::queue<int> tempq = dev.wait_queue;
            while (!tempq.empty())
            {
                processes[tempq.front()].blocked_time++;
                tempq.pop();
            }
        }
    }

    void increment_blocked_and_ready_times_except(int executing_idx)
    {
        // same as above but do not count the currently executing process in 'ready'
        std::queue<int> tmp = ready_queue;
        while (!tmp.empty())
        {
            int idx = tmp.front();
            tmp.pop();
            processes[idx].ready_time++;
        }

        for (size_t di = 0; di < devices.size(); ++di)
        {
            DeviceInfo &dev = devices[di];
            for (auto &pr : dev.active)
            {
                processes[pr.first].blocked_time++;
            }
            std::queue<int> tempq = dev.wait_queue;
            while (!tempq.empty())
            {
                processes[tempq.front()].blocked_time++;
                tempq.pop();
            }
        }

        // executing process is not counted
        (void)executing_idx;
    }

    void print_state_before_execution(int proc_idx, int run_time)
    {
        std::cout << "\n[Tempo " << current_time << "] Escalando PID=" << processes[proc_idx].pid
                  << " por " << run_time << " unidade(s) (restante antes: " << processes[proc_idx].remaining_time << ")\n";
        print_state("Estado antes da execucao");
    }

    void print_state(const std::string &title)
    {
        std::cout << "\n--- " << title << " (t=" << current_time << ") ---\n";

        // Processo em execução: not directly stored here; show next if any
        std::cout << "Fila de Prontos: ";
        if (ready_queue.empty())
            std::cout << "(vazia)\n";
        else
        {
            std::queue<int> tmp = ready_queue;
            while (!tmp.empty())
            {
                int idx = tmp.front();
                tmp.pop();
                std::cout << "PID=" << processes[idx].pid << "(rem=" << processes[idx].remaining_time << ") ";
            }
            std::cout << "\n";
        }

        // Bloqueados: collect from devices active + wait queues
        std::cout << "Bloqueados (em E/S ou esperando):\n";
        bool any_blocked = false;
        for (size_t di = 0; di < devices.size(); ++di)
        {
            DeviceInfo &dev = devices[di];
            // active
            for (auto &pr : dev.active)
            {
                any_blocked = true;
                int pidx = pr.first;
                int rem = pr.second;
                std::cout << "  PID=" << processes[pidx].pid << " - dispositivo=" << dev.name
                          << " (em uso, rem_op=" << rem << ")\n";
            }
            // waiting
            std::queue<int> tmpq = dev.wait_queue;
            while (!tmpq.empty())
            {
                any_blocked = true;
                int pidx = tmpq.front();
                tmpq.pop();
                std::cout << "  PID=" << processes[pidx].pid << " - dispositivo=" << dev.name
                          << " (fila de espera)\n";
            }
        }
        if (!any_blocked)
            std::cout << "  (nenhum)\n";

        // Dispositivos: estado
        std::cout << "Dispositivos:\n";
        for (size_t di = 0; di < devices.size(); ++di)
        {
            DeviceInfo &dev = devices[di];
            std::cout << "  " << dev.name << " | capacidade=" << dev.capacity
                      << " | em uso=" << dev.active.size() << " | esperando=" << dev.wait_queue.size() << "\n";
            if (!dev.active.empty())
            {
                std::cout << "    Em uso: ";
                for (auto &pr : dev.active)
                    std::cout << "PID=" << processes[pr.first].pid << "(op_rem=" << pr.second << ") ";
                std::cout << "\n";
            }
            if (!dev.wait_queue.empty())
            {
                std::cout << "    Fila: ";
                std::queue<int> tmpq = dev.wait_queue;
                while (!tmpq.empty())
                {
                    std::cout << "PID=" << processes[tmpq.front()].pid << " ";
                    tmpq.pop();
                }
                std::cout << "\n";
            }
        }
    }

    void print_final_statistics()
    {
        std::cout << "\n--- Estatisticas Finais ---\n";
        std::cout << std::left << std::setw(8) << "PID"
                  << std::setw(12) << "Turnaround"
                  << std::setw(12) << "Pronto"
                  << std::setw(12) << "Bloqueado"
                  << std::setw(12) << "Start" << "\n";
        std::cout << "-------------------------------------------------\n";

        for (const auto &p : processes)
        {
            int turnaround = p.end_time - p.creation_time;
            std::cout << std::left << std::setw(8) << p.pid
                      << std::setw(12) << turnaround
                      << std::setw(12) << p.ready_time
                      << std::setw(12) << p.blocked_time
                      << std::setw(12) << p.start_time << "\n";
        }
    }
};

// -------------------- main --------------------
int main(int argc, char *argv[])
{
    std::string filename;
    if (argc >= 2)
        filename = argv[1];
    else
    {
        std::cout << "Uso: " << argv[0] << " <arquivo_entrada>\n";
        return 1;
    }

    Config config;
    std::vector<DeviceInfo> devices;
    std::vector<ProcessInfo> processes;

    if (!read_file(filename, config, devices, processes))
        return 1;

    // debug
    print_debug(config, devices, processes);

    unsigned seed = 42; // seed fixa pra reprodutibilidade (pode trocar)
    Manager manager(config, devices, processes, seed);
    manager.run();

    return 0;
}
