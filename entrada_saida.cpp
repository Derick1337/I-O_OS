#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <queue>
#include <list>
#include <iomanip>
#include <cmath>
#include <unordered_map>
#include <ctime>

struct Management_Infos // infos gerais da simulação
{
    std::string scheduling_algorithm;
    int cpu_fraction = 0;

    // logica da memoria
    std::string memory_policy;
    int memory_size = 0;
    int page_size = 0;
    double allocation_percentage = 0.0;

    int num_devices = 0;
    int num_processes = 0;
    int global_time = 0;
};

struct Device // infos de cada dispositivo
{
    std::string name_id;   // nome ou ID do dispositivo
    int simultaneous_uses; // usos simultâneos
    int operation_time;    // tempo de operação

    bool is_busy = false; // se o dispositivo está ocupado

    std::vector<int> processes_using_devices; // processos usando o dispositivo 
    std::queue<int> waiting_processes;        // fila de espera do dispositivo 
};

struct Process // infos de cada processo
{
    int pid = 0;
    int creation_time = 0;
    int execution_time = 0;
    int remaining_time = 0;

    bool is_finished = false;
    bool is_blocked = false;
    bool is_running = false;
    bool is_ready = false;
    bool is_io_pending = false;
    bool is_using_io = false;

    int priority = 0;
    int memory_needed = 0;
    std::vector<int> page_sequence;
    int io_chance = 0;

    int ready_time = 0;
    int blocked_time = 0;

    int start_time = -1;
    int finish_time = -1;
    int turnaround_time = 0;
    int waiting_time = 0;

    int io_start_time = -1;
    int io_end_time = -1;
    int total_io_time = 0;
};

// dados da simulação
struct Simulation_data 
{
    Management_Infos management_infos;
    std::vector<Device> devices;
    std::vector<Process> processes;
};

Simulation_data read_file(const std::string &filename)
{
    Simulation_data simData;

    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Erro ao abrir o arquivo: " + filename);
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
            std::stringstream ss(line);
            std::string token;

            std::getline(ss, simData.management_infos.scheduling_algorithm, '|');
            std::getline(ss, token, '|');
            simData.management_infos.cpu_fraction = std::stoi(token);
            std::getline(ss, simData.management_infos.memory_policy, '|');
            std::getline(ss, token, '|');
            simData.management_infos.memory_size = std::stoi(token);
            std::getline(ss, token, '|');
            simData.management_infos.page_size = std::stoi(token);
            std::getline(ss, token, '|');
            simData.management_infos.allocation_percentage = std::stod(token);
            std::getline(ss, token);
            simData.management_infos.num_devices = std::stoi(token);
        }

        else if (devices_read < simData.management_infos.num_devices)
        {
            Device device;
            std::stringstream ss(line);
            std::string token;

            std::getline(ss, device.name_id, '|');
            std::getline(ss, token, '|');
            device.simultaneous_uses = std::stoi(token);
            std::getline(ss, token);
            device.operation_time = std::stoi(token);

            simData.devices.push_back(device);
            devices_read++;
        }

        else
        {
            Process proc;
            std::stringstream ss(line);
            std::string token;

            std::getline(ss, token, '|');
            proc.creation_time = std::stoi(token);
            std::getline(ss, token, '|');
            proc.pid = std::stoi(token);
            std::getline(ss, token, '|');
            proc.execution_time = std::stoi(token);
            proc.remaining_time = proc.execution_time;
            std::getline(ss, token, '|');
            proc.priority = std::stoi(token);
            std::getline(ss, token, '|');
            proc.memory_needed = std::stoi(token);

            std::getline(ss, token, '|');
            std::stringstream pages_ss(token);
            int page;
            while (pages_ss >> page)
                proc.page_sequence.push_back(page);

            if (std::getline(ss, token))
                proc.io_chance = std::stoi(token);
            else
                proc.io_chance = 0;

            simData.processes.push_back(proc);
        }
    }

    simData.management_infos.num_processes = (int)simData.processes.size();
    file.close();
    return simData;
}

// classe que gerencia as entradas e saídas
class IOManager
{
private:
    std::vector<Device> *devices_list; // lista de dispositivos
    std::vector<Process *> *blocked_list; // lista de processos bloqueados

public:
    IOManager(std::vector<Device> *devices_list, std::vector<Process *> *blocked_list)
    {
        this->devices_list = devices_list;
        this->blocked_list = blocked_list;
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
    }

    // sorteio que decide se o processo vai fazer entrada ou saída
    bool request_io(const Process &process)
    {
        if (process.remaining_time <= 0 || process.is_finished) // impede entrada e saída se o processo tiver acabado
            return false;
        int chance = std::rand() % 100;
        return chance < process.io_chance;
    }

    // decide em que fatia vai ter entrada/saída
    int when_request_io(int slice_used)
    {
        if (slice_used <= 1)
            return 1;
        return 1 + (std::rand() % slice_used);
    }

    // escolhe um dispositivo aleatoriamente 
    int choose_device()
    {
        if (devices_list->empty())
            return -1;
        return std::rand() % devices_list->size();
    }

    // gerencia a entrada/saída de um processo
    int handle_io(Process &process, int cpu_fraction, int global_time)
    {
        // se terminou não faz entrada/saída
        if (process.remaining_time <= 0 || process.is_finished)
            return 0;

        if (!request_io(process))
            return 0;

        // limete do sorteio
        int slice_used = std::min(cpu_fraction, process.remaining_time);
        if (slice_used <= 0)
            return 0;

        int moment_to_request = when_request_io(slice_used);
        
        // cancela se o processo terminar antes do momento sorteado
        if (process.remaining_time - moment_to_request <= 0)
            return 0;

        // consumo de CPU até o momento sorteado
        process.remaining_time -= moment_to_request;
        process.io_start_time = global_time + moment_to_request;

        process.is_running = false;
        process.is_blocked = true;
        process.is_ready = false;
        process.is_io_pending = true;

        int device_index = choose_device(); // chama aleatoriamente um dispositivo
        if (device_index == -1)
            return 0; 

        Device &device = (*devices_list)[device_index];

        // tenta usar o dispositivo ou entra na fila de espera
        if ((int)device.processes_using_devices.size() < device.simultaneous_uses)
        {
            device.processes_using_devices.push_back(process.pid);
            device.is_busy = true;
            process.is_using_io = true;
        }
        else // sem dispositivo entra na fila de espera
        {
            device.waiting_processes.push(process.pid);
            process.is_using_io = false;
        }

        // evitar duplicidade na lista de bloqueados
        auto already = std::find(blocked_list->begin(), blocked_list->end(), &process);
        if (already == blocked_list->end())
            blocked_list->push_back(&process);

        std::cout << "[E/S] PID " << process.pid
                  << " requisitou E/S no dispositivo '" << device.name_id
                  << "' (ficou bloqueado em t=" << process.io_start_time << ")\n";

        return moment_to_request;
    }

    // atualiza o estado dos dispositivos e processos bloqueados
    void update_devices(int global_time, std::vector<Process> &processes_list)
    {
        for (auto &device : *devices_list)
        {
            // verifica processos que estão usando o dispositivo e libera os que concluíram
            for (auto it = device.processes_using_devices.begin();
                 it != device.processes_using_devices.end();)
            {
                int pid = *it;
                auto it_proc = std::find_if(processes_list.begin(), processes_list.end(),
                                            [pid](const Process &p)
                                            { return p.pid == pid; });

                if (it_proc != processes_list.end())
                {
                    int elapsed = global_time - it_proc->io_start_time;
                    if (elapsed >= device.operation_time)
                    {
                        // atualiza a struct Process
                        it_proc->is_blocked = false;
                        it_proc->is_io_pending = false;
                        it_proc->is_using_io = false;
                        it_proc->io_end_time = global_time;
                        it_proc->total_io_time += device.operation_time;

                        std::cout << "[E/S] PID " << it_proc->pid
                                  << " terminou uso de " << device.name_id
                                  << " em t=" << global_time << "\n";

                        // remove pid da lista de processos usando o dispositivo
                        it = device.processes_using_devices.erase(it);
                        continue; 
                    }
                }
                ++it;
            }

            // mover fila de espera para uso se houver slot
            while ((int)device.processes_using_devices.size() < device.simultaneous_uses &&
                   !device.waiting_processes.empty())
            {
                int next_pid = device.waiting_processes.front();
                device.waiting_processes.pop();
                device.processes_using_devices.push_back(next_pid);

                auto it_proc = std::find_if(processes_list.begin(), processes_list.end(),
                                            [next_pid](const Process &p)
                                            { return p.pid == next_pid; });

                if (it_proc != processes_list.end())
                {
                    it_proc->is_using_io = true;
                    it_proc->is_blocked = true;
                    it_proc->io_start_time = global_time; 
                    std::cout << "[E/S] PID " << it_proc->pid
                              << " começou uso de " << device.name_id
                              << " em t=" << global_time << "\n";
                }
            }

            device.is_busy = !device.processes_using_devices.empty();
        }
    }
};


class RoundRobinScheduler
{
private:
    Management_Infos management_infos;
    std::vector<Device> devices_list;
    std::vector<Process> processes_list;
    std::queue<Process *> ready_queue; // processos em estado de pronto
    std::vector<Process> finished_list; // processos finalizados
    std::vector<Process *> blocked_list; // processos em estado de bloqueado
    IOManager *io_manager;

    int global_time;
    int cpu_fraction;

public:
    RoundRobinScheduler(const Simulation_data &data)
    {
        management_infos = data.management_infos;
        devices_list = data.devices;
        processes_list = data.processes;
        cpu_fraction = management_infos.cpu_fraction;
        global_time = 0;

        io_manager = new IOManager(&devices_list, &blocked_list);
    }

    ~RoundRobinScheduler()
    {
        delete io_manager;
    }

    // checa se todos os processos terminaram
    bool all_processes_finished()
    {
        return (int)finished_list.size() == management_infos.num_processes;
    }

    // retorna o nome do dispositivo que o pid está usando
    std::string device_using_by_pid(int pid)
    {
        for (auto &dev : devices_list)
        {
            for (int u : dev.processes_using_devices)
                if (u == pid)
                    return dev.name_id;
        }
        return "";
    }

    // retorna dispositivos onde o pid está na fila de espera 
    std::string device_waiting_by_pid(int pid)
    {
        for (auto &dev : devices_list)
        {
            std::queue<int> temp = dev.waiting_processes;
            while (!temp.empty())
            {
                if (temp.front() == pid)
                    return dev.name_id;
                temp.pop();
            }
        }
        return "";
    }

    // imprime o estado do sistema no momento de troca de processo
    void print_system_state(Process *running_process)
    {
        std::cout << "==================== Estado do sistema (t=" << global_time << ") ====================\n";

        
        if (running_process)
        {
            std::cout << "CPU: PID " << running_process->pid
                      << " (remaining=" << running_process->remaining_time << ")\n";
        }
        else
        {
            std::cout << "CPU: idle\n";
        }

        // prontos
        std::cout << "Prontos: ";
        bool any_ready = false;
        for (auto &proc : processes_list)
        {
            if (proc.is_ready && !proc.is_running && !proc.is_finished)
            {
                any_ready = true;
                std::cout << "PID " << proc.pid << "(rem=" << proc.remaining_time << ") ";
            }
        }
        if (!any_ready)
            std::cout << "nenhum";
        std::cout << "\n";

        // bloqueados 
        std::cout << "Bloqueados:\n";
        bool any_blocked = false;
        for (auto &proc : processes_list)
        {
            if (proc.is_blocked)
            {
                any_blocked = true;
                std::string using_dev = device_using_by_pid(proc.pid);
                std::string waiting_dev = device_waiting_by_pid(proc.pid);
                std::cout << "  PID " << proc.pid << " (rem=" << proc.remaining_time << ")";
                if (!using_dev.empty())
                    std::cout << " usando " << using_dev;
                else if (!waiting_dev.empty())
                    std::cout << " aguardando " << waiting_dev;
                std::cout << "\n";
            }
        }
        if (!any_blocked)
            std::cout << "  nenhum\n";

        // imprime estado dos dispositivos
        std::cout << "Dispositivos:\n";
        for (auto &dev : devices_list)
        {
            std::cout << "  " << dev.name_id << " (op_time=" << dev.operation_time
                      << ", slots=" << dev.simultaneous_uses << ") ";
            if (dev.is_busy)
                std::cout << "[BUSY]\n";
            else
                std::cout << "[FREE]\n";

            std::cout << "    Usando: ";
            if (dev.processes_using_devices.empty())
                std::cout << "nenhum";
            else
            {
                for (int pid : dev.processes_using_devices)
                    std::cout << pid << " ";
            }
            std::cout << "\n";

            std::cout << "    Fila: ";
            if (dev.waiting_processes.empty())
                std::cout << "vazia";
            else
            {
                std::queue<int> temp = dev.waiting_processes;
                while (!temp.empty())
                {
                    std::cout << temp.front() << " ";
                    temp.pop();
                }
            }
            std::cout << "\n";
        }

        std::cout << "====================================================================\n";
    }

    void print_final_report()
    {
        std::cout << "\n==================== Relatorio final ====================\n";
        std::cout << std::left << std::setw(6) << "PID"
                  << std::setw(12) << "Turnaround"
                  << std::setw(12) << "TempoPronto"
                  << std::setw(12) << "TempoBloq"
                  << std::setw(12) << "TotalIO"
                  << "\n";

        for (auto &proc : processes_list)
        {
            int turnaround = proc.finish_time - proc.creation_time;
            std::cout << std::left << std::setw(6) << proc.pid
                      << std::setw(12) << turnaround
                      << std::setw(12) << proc.ready_time
                      << std::setw(12) << proc.blocked_time
                      << std::setw(12) << proc.total_io_time
                      << "\n";
        }
        std::cout << "=========================================================\n";
    }

    // atualiza fila de prontos 
    void update_ready_queue()
    {
        for (auto &process : processes_list)
        {
            if (process.creation_time <= global_time &&
                !process.is_finished &&
                !process.is_ready &&
                !process.is_blocked &&
                !process.is_running) 
            {
                process.is_ready = true;
                ready_queue.push(&process);
            }
        }
    }

    void run()
    {
        // chama a atualização
        update_ready_queue();

        while (!all_processes_finished())
        {
            // escolher próximo processo 
            if (!ready_queue.empty())
            {
                Process *process = ready_queue.front();
                ready_queue.pop();

                process->is_running = true;
                process->is_ready = false;

                print_system_state(process);

                // chama gerenciador de entrada/saída
                int time_until_io = io_manager->handle_io(*process, cpu_fraction, global_time);

                int time_advance = 0;

                if (time_until_io > 0)
                {
                    // o processo requisitou entrada/saída ficando bloqueado
                    time_advance = time_until_io;
                }
                else
                {
                    // não houve entrada/saída continua rodando normal
                    int slice_used = std::min(cpu_fraction, process->remaining_time);
                    process->remaining_time -= slice_used;
                    time_advance = slice_used;

                    if (process->remaining_time <= 0)
                    {
                        process->is_finished = true;
                        process->finish_time = global_time + slice_used;
                        process->turnaround_time = process->finish_time - process->creation_time;
                        process->waiting_time = process->turnaround_time - process->execution_time;
                        finished_list.push_back(*process);

                        std::cout << "[CPU] PID " << process->pid << " finalizou em t=" << process->finish_time << "\n";
                    }
                    else
                    {
                        // volta para fila de prontos
                        process->is_running = false;
                        process->is_ready = true;
                        ready_queue.push(process);
                    }
                }

                // avança o tempo global pelo período consumido 
                if (time_advance > 0)
                {
                    // atualizar contadores por todos os processos pelo tempo avançado
                    for (auto &proc : processes_list)
                    {
                        if (proc.is_blocked)
                            proc.blocked_time += time_advance;
                        else if (proc.is_ready && !proc.is_running && !proc.is_finished)
                            proc.ready_time += time_advance;
                    }

                    global_time += time_advance;
                }

                // após avanço de tempo atualiza estado dos dispositivos
                io_manager->update_devices(global_time, processes_list);

                // tira de bloqueado e coloca na fila de prontos
                for (auto it = blocked_list.begin(); it != blocked_list.end();)
                {
                    Process *proc_ptr = *it;
                    if (!proc_ptr->is_blocked && !proc_ptr->is_running)
                    {
                        proc_ptr->is_ready = true;
                        ready_queue.push(proc_ptr);
                        it = blocked_list.erase(it);
                    }
                    else
                        ++it;
                }

                // atualiza fila de prontos
                update_ready_queue();
            }
            else
            {
                // CPU ociosa avança tempo até o próximo evento
                for (auto &proc : processes_list)
                {
                    if (proc.is_blocked)
                        proc.blocked_time++;
                    else if (proc.is_ready && !proc.is_running && !proc.is_finished)
                        proc.ready_time++;
                }

                global_time++;
                io_manager->update_devices(global_time, processes_list);

                for (auto it = blocked_list.begin(); it != blocked_list.end();)
                {
                    Process *proc_ptr = *it;
                    if (!proc_ptr->is_blocked)
                    {
                        proc_ptr->is_ready = true;
                        ready_queue.push(proc_ptr);
                        it = blocked_list.erase(it);
                    }
                    else
                        ++it;
                }

                update_ready_queue();
            }
        }

        print_final_report();
    }
};

// O FIFO esta sendo usado na substituição de páginas
class FIFO
{
protected:
    int num_frames;
    std::vector<int> frames;
    std::vector<int> arrival_queue;
    int page_replacements;

public:
    FIFO(int n_frames) : num_frames(n_frames), page_replacements(0) {}
    int get_page_replacements() const { return page_replacements; }

protected:
    bool is_page_in_memory(int page) const
    {
        for (size_t i = 0; i < frames.size(); ++i)
        {
            if (frames[i] == page)
                return true;
        }
        return false;
    }

    void replace_page(int page)
    {
        int victim_page = arrival_queue[0];

        for (size_t i = 0; i < arrival_queue.size() - 1; ++i)
        {
            arrival_queue[i] = arrival_queue[i + 1];
        }
        arrival_queue.pop_back();

        for (size_t i = 0; i < frames.size(); ++i)
        {
            if (frames[i] == victim_page)
            {
                frames.erase(frames.begin() + i);
                break;
            }
        }

        frames.push_back(page);
        arrival_queue.push_back(page);
        page_replacements++;
    }

public:
    void execute(const std::vector<int> &access_sequence)
    {
        for (int page : access_sequence)
        {
            if (!is_page_in_memory(page))
            {
                if (frames.size() >= (size_t)num_frames)
                {
                    replace_page(page);
                }
                else
                {
                    frames.push_back(page);
                    arrival_queue.push_back(page);
                }
            }
        }
    }
};

class MemorySimulator
{
private:
    Management_Infos config;
    std::vector<Process> processes;
    int total_fifo_replacements;

public:
    MemorySimulator(const Simulation_data &data)
        : config(data.management_infos), processes(data.processes), total_fifo_replacements(0) {}

    void run()
    {
        std::string mem_policy = config.memory_policy;
        for (char &c : mem_policy)
            c = (char)std::tolower(static_cast<unsigned char>(c));
        bool is_local = (mem_policy == "local");

        total_fifo_replacements = 0;

        std::cout << "--- Simulacao de Gerenciamento de Memoria ---\n";

        if (is_local)
        {
            run_local_policy();
        }
        else
        {
            run_global_policy();
        }

        std::cout << "\nTotal FIFO replacements: " << total_fifo_replacements << "\n";
    }

    int get_total_replacements() const { return total_fifo_replacements; }

private:
    void run_local_policy()
    {
        // percorre os processos
        for (const auto &proc : processes)
        {
            // ignora o processo se não tiver sequência de páginas
            if (proc.page_sequence.empty() || config.page_size <= 0)
                continue;

            int process_virtual_pages = (int)std::ceil((double)proc.memory_needed / (double)config.page_size);

            // número de quadros para este processo 
            int num_frames = (int)std::floor(process_virtual_pages * (config.allocation_percentage / 100.0));

            // garante ao menos 1 quadro
            if (num_frames <= 0)
                num_frames = 1;

            std::cout << "\n--- Processo PID: " << proc.pid << " (com " << num_frames << " quadros) ---\n";

            
            FIFO fifo(num_frames);

            // executa a sequência de acessos 
            fifo.execute(proc.page_sequence);

            // número de substituições
            int fifo_reps = fifo.get_page_replacements();
            total_fifo_replacements += fifo_reps;

            std::cout << "-> FIFO: " << fifo_reps << " trocas de pagina.\n";
        }
    }

    void run_global_policy()
    {
        // constrói sequência combinada de acessos de todos os processos
        std::vector<int> combined_sequence;

       
        for (const auto &proc : processes)
        {
            for (int page : proc.page_sequence)
                combined_sequence.push_back(proc.pid * 10000 + page);
        }

        // calcula o número total de quadros disponíveis
        int total_frames = (config.page_size > 0) ? (config.memory_size / config.page_size) : 1;
        if (total_frames <= 0)
            total_frames = 1;

       
        std::cout << "\n--- Politica GLOBAL com " << total_frames << " molduras totais ---\n";

        FIFO fifo(total_frames);

        fifo.execute(combined_sequence);

        // atualiza o total de substituições de página
        total_fifo_replacements = fifo.get_page_replacements();
        std::cout << "-> FIFO: " << total_fifo_replacements << " trocas de pagina.\n";
    }
};

// estou passando os valores pelo terminal cansei de editar no vs code (Lucas te vira e aprende a usar terminal)
int main(int argc, char *argv[])
{
    std::string file_name;
    
    
    if (argc > 1)
    {
        file_name = argv[1];
    }
    else
    {
        std::cout << "erro: nome do arquivo de entrada nao fornecido.\n";
        return 1;
    }
    
    std::ifstream test_file(file_name);

    // coloquei como debug para ver se abri o arquivo certo no terminal
    if (test_file.is_open())
    {
        std::cout << "Arquivo '" << file_name << "' carregado com sucesso.\n";
        test_file.close();
    }
    else
    {
        std::cout << "Erro ao carregar o arquivo '" << file_name << "'.\n";
        return 1;
    }

    try
    {
        Simulation_data data = read_file(file_name);

        RoundRobinScheduler scheduler(data);
        scheduler.run();

        MemorySimulator memory_simulator(data);
        memory_simulator.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Erro: " << e.what() << "\n";
    }

    return 0;
}
