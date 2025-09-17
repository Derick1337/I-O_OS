#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <limits>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iomanip>

struct DeviceInfo
{
    std::string name;
    int priority;
    int access_time;
};

struct ProcessInfo
{
    int creation_time;
    int pid;
    int execution_time;
    int priority;
    int memory_needed;
    std::vector<int> page_sequence;
    int io_operations; // Novo campo para operações de I/O
};

struct Config
{
    std::string scheduling_algorithm;
    int cpu_fraction;
    std::string memory_policy;
    int memory_size;
    int page_size;
    double allocation_percentage;
    int num_devices; // Novo campo para número de dispositivos
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
            // Primeira linha: configuração
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
            device.priority = std::stoi(token);
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
            std::getline(ss, token, '|');
            // Sequência de páginas
            std::stringstream pages_ss(token);
            std::string page_str;
            while (pages_ss >> page_str)
            {
                process.page_sequence.push_back(std::stoi(page_str));
            }
            // Último campo: operações de I/O
            if (std::getline(ss, token))
            {
                process.io_operations = std::stoi(token);
            }
            processes.push_back(process);
        }
    }
    file.close();
    return true;
}

// --- CLASSE FIFO (First-In, First-Out) ---
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

// Escalonador de Loteria

class LotteryScheduler;

class Process
{
public:
    Process(int pid, int creation_time, int burst_time, int tickets = 0);
    int get_pid() const;

private:
    int pid;
    int creation_time;
    int burst_time;
    int remaining_time;
    int start_time;
    int end_time;
    bool is_finished;
    int tickets;
    friend class LotteryScheduler;
};

Process::Process(int pid, int creation_time, int burst_time, int tickets)
{
    this->pid = pid;
    this->creation_time = creation_time;
    this->burst_time = burst_time;
    this->remaining_time = burst_time;
    this->start_time = -1;
    this->end_time = -1;
    this->tickets = tickets;
    this->is_finished = false;
}
int Process::get_pid() const { return pid; }

class LotteryScheduler
{
public:
    LotteryScheduler();
    void set_quantum(int q);
    void add_process(const Process &process);
    void run();

private:
    void update_ready_queue();
    Process *select_winner();
    std::vector<Process> processes;
    std::vector<Process *> ready_queue;
    int quantum;
    int current_time;
};

LotteryScheduler::LotteryScheduler()
{
    quantum = 0;
    current_time = 0;
}

void LotteryScheduler::set_quantum(int q) { quantum = q; }
void LotteryScheduler::add_process(const Process &process) { processes.push_back(process); }

void LotteryScheduler::update_ready_queue()
{
    for (auto &process : processes)
    {
        bool in_ready_queue = false;
        for (const auto &ready_process : ready_queue)
        {
            if (ready_process->get_pid() == process.get_pid())
            {
                in_ready_queue = true;
                break;
            }
        }
        if (!process.is_finished && !in_ready_queue && process.creation_time <= current_time)
        {
            ready_queue.push_back(&process);
        }
    }
}

Process *LotteryScheduler::select_winner()
{
    int total_tickets = 0;
    for (const auto &process : ready_queue)
    {
        total_tickets += process->tickets;
    }
    if (total_tickets == 0)
        return nullptr;

    int winning_ticket = std::rand() % total_tickets;
    int current_ticket_sum = 0;
    for (auto &process : ready_queue)
    {
        current_ticket_sum += process->tickets;
        if (winning_ticket < current_ticket_sum)
        {
            return process;
        }
    }
    return nullptr;
}

void LotteryScheduler::run()
{
    std::srand(time(0));
    std::cout << "--- Simulacao do Escalonador Loteria ---\n\n";

    while (true)
    {
        update_ready_queue();
        bool all_finished = true;
        for (const auto &process : processes)
        {
            if (!process.is_finished)
            {
                all_finished = false;
                break;
            }
        }
        if (all_finished)
        {
            std::cout << "\n--- Simulacao do Escalonador finalizada no tempo " << current_time << " ---\n\n";
            break;
        }
        if (ready_queue.empty())
        {
            current_time++;
            continue;
        }
        Process *winner = select_winner();
        if (winner == nullptr)
        {
            current_time++;
            continue;
        }
        if (winner->start_time == -1)
        {
            winner->start_time = current_time;
        }
        int time_to_run = std::min(winner->remaining_time, quantum);
        std::cout << "Tempo[" << std::setw(3) << current_time << " -> " << std::setw(3) << current_time + time_to_run << "]: "
                  << "Processo " << winner->get_pid() << " esta na CPU. (Restante: "
                  << winner->remaining_time - time_to_run << ")" << std::endl;
        current_time += time_to_run;
        winner->remaining_time -= time_to_run;
        if (winner->remaining_time <= 0)
        {
            winner->is_finished = true;
            winner->end_time = current_time;
            std::cout << ">>> Processo " << winner->get_pid() << " finalizado no tempo " << current_time << " <<<" << std::endl;
            for (size_t i = 0; i < ready_queue.size(); ++i)
            {
                if (ready_queue[i]->get_pid() == winner->get_pid())
                {
                    ready_queue.erase(ready_queue.begin() + i);
                    break;
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
}