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

struct ProcessInfo
{
    int creation_time;
    int pid;
    int execution_time;
    int priority;
    int memory_needed;
    std::vector<int> page_sequence;
};

struct Config
{
    std::string scheduling_algorithm;
    int cpu_fraction;
    std::string memory_policy;
    int memory_size;
    int page_size;
    double allocation_percentage;
};

bool read_file(const std::string &filename, Config &config, std::vector<ProcessInfo> &processes)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Erro ao abrir o arquivo: " << filename << std::endl;
        return false;
    }

    std::string line;
    bool first_line = true;

    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        if (first_line)
        {
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
            std::getline(ss, token);
            config.allocation_percentage = std::stod(token);
            first_line = false;
        }
        else
        {
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
            if (std::getline(ss, token))
            {
                std::stringstream pages_ss(token);
                std::string page_str;
                while (pages_ss >> page_str)
                {
                    process.page_sequence.push_back(std::stoi(page_str));
                }
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
    if (argc < 2)
    {
        std::cerr << "Uso: " << argv[0] << " <arquivo_de_entrada>" << std::endl;
        return 1;
    }
    std::string filename = argv[1];

    Config config;
    std::vector<ProcessInfo> processes;

    if (!read_file(filename, config, processes))
    {
        return 1;
    }

    std::string algorithm_name_lower = config.scheduling_algorithm;
    for (size_t i = 0; i < algorithm_name_lower.length(); ++i)
    {
        algorithm_name_lower[i] = tolower(algorithm_name_lower[i]);
    }

    if (algorithm_name_lower == "loteria")
    {
        LotteryScheduler scheduler;
        scheduler.set_quantum(config.cpu_fraction);
        for (size_t i = 0; i < processes.size(); ++i)
        {
            Process p(processes[i].pid, processes[i].creation_time, processes[i].execution_time, processes[i].priority);
            scheduler.add_process(p);
        }
        scheduler.run();
    }

    std::string memory_policy_lower = config.memory_policy;
    for (char &c : memory_policy_lower)
        c = tolower(c);
    bool is_local = (memory_policy_lower == "local");

    int total_fifo_replacements = 0;
    int total_mru_replacements = 0;
    int total_nuf_replacements = 0;
    int total_optimal_replacements = 0;

    std::cout << "--- Simulacao de Gerenciamento de Memoria ---\n";

    if (is_local) // Política é LOCAL
    {
        for (size_t i = 0; i < processes.size(); ++i)
        {
            const ProcessInfo &process = processes[i];
            if (process.page_sequence.empty() || config.page_size == 0)
                continue;

            int process_virtual_pages = (int)ceil((double)process.memory_needed / config.page_size);
            int num_frames = floor(process_virtual_pages * (config.allocation_percentage / 100.0));
            if (num_frames == 0)
                num_frames = 1;

            std::cout << "\n--- Processo PID: " << process.pid << " (com " << num_frames << " quadros) ---\n";

            FIFO fifo(num_frames);
            fifo.execute(process.page_sequence);
            int fifo_reps = fifo.get_page_replacements();
            total_fifo_replacements += fifo_reps;
            std::cout << "-> FIFO: " << fifo_reps << " trocas de pagina.\n";

            MRU mru(num_frames);
            mru.execute(process.page_sequence);
            int mru_reps = mru.get_page_replacements();
            total_mru_replacements += mru_reps;
            std::cout << "-> MRU: " << mru_reps << " trocas de pagina.\n";

            NUF nuf(num_frames);
            nuf.execute(process.page_sequence);
            int nuf_reps = nuf.get_page_replacements();
            total_nuf_replacements += nuf_reps;
            std::cout << "-> NUF: " << nuf_reps << " trocas de pagina.\n";

            Otimo optimal(num_frames);
            optimal.execute(process.page_sequence);
            int opt_reps = optimal.get_page_replacements();
            total_optimal_replacements += opt_reps;
            std::cout << "-> Otimo: " << opt_reps << " trocas de pagina.\n";
        }
    }
    else // Política é GLOBAL
    {
        std::vector<int> combined_sequence;
        for (const auto &proc : processes)
        {
            for (int page : proc.page_sequence)
            {
                combined_sequence.push_back(proc.pid * 10000 + page);
            }
        }

        int total_frames = config.memory_size / config.page_size;
        if (total_frames == 0)
            total_frames = 1;

        std::cout << "\n--- Politica GLOBAL com " << total_frames << " molduras totais ---\n";

        FIFO fifo(total_frames);
        fifo.execute(combined_sequence);
        total_fifo_replacements = fifo.get_page_replacements();
        std::cout << "-> FIFO: " << total_fifo_replacements << " trocas de pagina.\n";

        MRU mru(total_frames);
        mru.execute(combined_sequence);
        total_mru_replacements = mru.get_page_replacements();
        std::cout << "-> MRU: " << total_mru_replacements << " trocas de pagina.\n";

        NUF nuf(total_frames);
        nuf.execute(combined_sequence);
        total_nuf_replacements = nuf.get_page_replacements();
        std::cout << "-> NUF: " << total_nuf_replacements << " trocas de pagina.\n";

        Otimo optimal(total_frames);
        optimal.execute(combined_sequence);
        total_optimal_replacements = optimal.get_page_replacements();
        std::cout << "-> Otimo: " << total_optimal_replacements << " trocas de pagina.\n";
    }

    std::string best_algorithm = "empate";
    int min_diff = std::numeric_limits<int>::max();
    bool tie = false;

    int diff_fifo = abs(total_fifo_replacements - total_optimal_replacements);
    if (diff_fifo < min_diff)
    {
        min_diff = diff_fifo;
        best_algorithm = "FIFO";
        tie = false;
    }

    int diff_mru = abs(total_mru_replacements - total_optimal_replacements);
    if (diff_mru < min_diff)
    {
        min_diff = diff_mru;
        best_algorithm = "MRU (Menos Recentemente Usada)";
        tie = false;
    }
    else if (diff_mru == min_diff)
    {
        tie = true;
    }

    int diff_nuf = abs(total_nuf_replacements - total_optimal_replacements);
    if (diff_nuf < min_diff)
    {
        min_diff = diff_nuf;
        best_algorithm = "NUF (Não Usada Frequentemente)";
        tie = false;
    }
    else if (diff_nuf == min_diff)
    {
        tie = true;
    }

    if (tie)
        best_algorithm = "empate";

    std::cout << "\n\n=======================================================";
    std::cout << "\n--- Resultado Final do Gerenciamento de Memoria ---\n";
    std::cout << "=======================================================\n";
    std::cout << total_fifo_replacements << "|"
              << total_mru_replacements << "|"
              << total_nuf_replacements << "|"
              << total_optimal_replacements << "|"
              << best_algorithm << std::endl;
    std::cout << "=======================================================\n";

    return 0;
}