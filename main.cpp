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


struct Management_Infos // infos gerais da simulação 
{
    std::string scheduling_algorithm; 
    int cpu_fraction = 0; // fatia de cpu (igual pra todos os processos)
    std::string memory_policy;
    int memory_size = 0;
    int page_size = 0;
    double allocation_percentage = 0.0;
    int num_devices = 0;
    int num_processes = 0;
    int global_time = 0;

    /*##########decidir se essas filas vão aqui ou em outra struct ou classe 
    #######################*/ 

    


};

std::vector<int> blocked_list; // lista de bloqueados

struct Device // infos de cada dispositivo
{
    std::string name_id; // nome ou ID do dispositivo
    int simultaneous_uses;     // usos simultâneos
    int operation_time;  // tempo de operação

    bool is_busy = false; // se o dispositivo está ocupado

    std::vector<int> processes_using_devices; // processos usando o dispositivo
    std::queue<int> waiting_processes; // fila de espera do dispositivo

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
    bool is_io_pending = false; // se está esperando operação de E/S
    bool is_using_io = false; // se está usando o dispositivo de E/S

    int priority = 0;
    int memory_needed = 0;
    std::vector<int> page_sequence;
    int io_chance = 0; // chance de operação de E/S (%)

    int ready_time = 0;
    int blocked_time = 0;

    int start_time = -1;
    int finish_time = -1;
    int turnaround_time = finish_time - creation_time;
    int waiting_time = turnaround_time - execution_time;

    int io_start_time = -1;
    int io_end_time = -1;
    int total_io_time = 0;
 
}




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
            std::getline(ss, device.name_id, '|');
            std::getline(ss, token, '|');
            device.simultaneous_uses = std::stoi(token);
            std::getline(ss, token);
            device.operation_time = std::stoi(token);

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

            
            if (std::getline(ss, token))
                process.io_chance = std::stoi(token);
            else
                process.io_chance = 0; 

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
        std::cout << "ID: " << d.name_id << " | Usos simultâneos: " << d.simultaneous_uses << " | Tempo: " << d.operation_time << "\n";

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

        std::cout << " | Chance E/S: " << p.io_chance << "%\n";
    }
}



std::vector<Process> blocked_list;

class RoundRobinScheduler{
    protected:
    Management_Infos Management_Infos;

    std::vector<Process> all_processes; // lista de todos os processos
    std::queue<Process> ready_queue; // fila de prontos

    std::vector<Process> finished_processes; // lista de finalizados

    void update_management_infos(){

    }

    void add_processes(std::vector<Process> &process_list){
        for (auto &p : process_list)
        {
            p.remaining_time = p.execution_time;
            all_processes.push_back(p); 
        }
    }

    void update_ready_queue(){
        for (auto &p : all_processes){
            
        }
        

    void run(){
        while (true)
        {
            if finished_processes.size == 
        }
        
    }

};


// --- CLASSE FIFO (First-In, First-Out) ---
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cmath>

class FIFO
{
public:
    enum Mode
    {
        LOCAL,
        GLOBAL
    };

private:
    int total_frames;
    int page_replacements;
    Mode mode;

    // Para política GLOBAL
    std::vector<int> global_frames;
    std::vector<int> global_arrival_queue;

    // Para política LOCAL
    struct FrameTable
    {
        int num_frames;
        std::vector<int> frames;
        std::vector<int> arrival_queue;
    };
    std::unordered_map<int, FrameTable> per_process_tables;

public:
    FIFO(int n_frames, Mode m) : total_frames(n_frames), page_replacements(0), mode(m) {}

    int get_page_replacements() const { return page_replacements; }

    // Inicializa uma tabela local para um processo específico
    void add_process(int pid, int num_frames)
    {
        if (mode == LOCAL)
        {
            FrameTable table;
            table.num_frames = num_frames;
            per_process_tables[pid] = table;
        }
    }

    // Executa sequência global ou local
    void execute_global(const std::vector<int> &access_sequence)
    {
        for (int code : access_sequence)
        {
            int page = code; // já está codificado com pid*10000 + page
            if (!is_page_in_global(page))
            {
                if (global_frames.size() >= (size_t)total_frames)
                    replace_global(page);
                else
                {
                    global_frames.push_back(page);
                    global_arrival_queue.push_back(page);
                }
            }
        }
    }

    void execute_local(int pid, const std::vector<int> &page_sequence)
    {
        FrameTable &table = per_process_tables[pid];
        for (int page : page_sequence)
        {
            if (!is_page_in_local(table, page))
            {
                if (table.frames.size() >= (size_t)table.num_frames)
                    replace_local(table, page);
                else
                {
                    table.frames.push_back(page);
                    table.arrival_queue.push_back(page);
                }
            }
        }
    }

private:
    // -------- GLOBAL helpers --------
    bool is_page_in_global(int page) const
    {
        for (int p : global_frames)
            if (p == page)
                return true;
        return false;
    }

    void replace_global(int page)
    {
        int victim = global_arrival_queue.front();
        global_arrival_queue.erase(global_arrival_queue.begin());

        for (auto it = global_frames.begin(); it != global_frames.end(); ++it)
        {
            if (*it == victim)
            {
                global_frames.erase(it);
                break;
            }
        }

        global_frames.push_back(page);
        global_arrival_queue.push_back(page);
        page_replacements++;
    }

    // -------- LOCAL helpers --------
    bool is_page_in_local(const FrameTable &table, int page) const
    {
        for (int p : table.frames)
            if (p == page)
                return true;
        return false;
    }

    void replace_local(FrameTable &table, int page)
    {
        int victim = table.arrival_queue.front();
        table.arrival_queue.erase(table.arrival_queue.begin());

        for (auto it = table.frames.begin(); it != table.frames.end(); ++it)
        {
            if (*it == victim)
            {
                table.frames.erase(it);
                break;
            }
        }

        table.frames.push_back(page);
        table.arrival_queue.push_back(page);
        page_replacements++;
    }
};




bool all_processes_finished(){
    // if finished list size == num_processes in config, return true
    return finished_processes.size() == config.num_processes;
}

class Manager
{
    protected:
    std::string memory_policy;
    void run()
    {
        while (true)
        {
          if (memory_policy == "LOCAL")
          {

        
          }
          else if (memory_policy == "GLOBAL")
          {}


          if (all_processes_finished())
              break;
        }
        
    }
};

int main(int argc, char *argv[])
{
   

    return 0;
}
