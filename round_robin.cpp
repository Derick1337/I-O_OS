#include <iostream>
#include <fstream> // leitura de arquivos
#include <sstream> // manipulação de strings
#include <string>
#include <iomanip> // formatação de saída
#include <cstdlib> // gerar números aleatórios
#include <vector>  // vetor de processos
#include <map>     // Arvore rubro negra
#include <queue>   // Fila de chegada


class RoundRobinScheduler
{
public:
    RoundRobinScheduler();
    void set_algorithm_name(const std::string &name);
    void set_quantum(int q);
    void add_process(const Process &process);
    void run();
    void print_statistics();

private:
    void update_ready_queue();
    std::vector<Process> all_processes;
    std::queue<Process *> ready_queue;
    std::string algorithm_name;
    int quantum;
    int current_time;
    size_t finished_process_count;
};

RoundRobinScheduler::RoundRobinScheduler()
{
    quantum = 0;
    current_time = 0;
    finished_process_count = 0;
}

void RoundRobinScheduler::set_algorithm_name(const std::string &name) { algorithm_name = name; }
void RoundRobinScheduler::set_quantum(int q) { quantum = q; }
void RoundRobinScheduler::add_process(const Process &process) { all_processes.push_back(process); }

void RoundRobinScheduler::update_ready_queue()
{
    for (auto &process : all_processes)
    {
        if (!process.is_finished && process.creation_time <= current_time)
        {
            bool in_queue = false;

            std::queue<Process *> temp_q = ready_queue;
            while (!temp_q.empty())
            {
                if (temp_q.front()->get_pid() == process.get_pid())
                {
                    in_queue = true;
                    break;
                }
                temp_q.pop();
            }
            if (process.start_time != -1)
                in_queue = true;

            if (!in_queue)
            {
                ready_queue.push(&process);
            }
        }
    }
}

void RoundRobinScheduler::run()
{
    std::cout << "--- Iniciando Simulacao do Escalonador ---\n";
    std::cout << "Algoritmo: " << algorithm_name << " | Fatia de CPU: " << quantum << std::endl
              << std::endl;

    while (finished_process_count < all_processes.size())
    {
        update_ready_queue();

        if (ready_queue.empty())
        {
            current_time++;
            continue;
        }

        Process *current_proc = ready_queue.front();
        ready_queue.pop();

        if (current_proc->start_time == -1)
        {
            current_proc->start_time = current_time;
        }

        int time_to_run = std::min(current_proc->remaining_time, quantum);

        std::cout << "Tempo[" << std::setw(3) << current_time << " -> " << std::setw(3) << current_time + time_to_run << "]: "
                  << "Processo " << current_proc->get_pid() << " esta na CPU. (Restante: "
                  << current_proc->remaining_time - time_to_run << ")" << std::endl;

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
            std::cout << ">>> Processo " << current_proc->get_pid() << " finalizado no tempo " << current_time << " <<<" << std::endl;
        }
    }

    std::cout << "\n--- Simulacao finalizada no tempo " << current_time << " ---\n";
}

void RoundRobinScheduler::print_statistics()
{
    std::cout << "\n--- Estatisticas Finais ---\n";
    std::cout << std::left << std::setw(10) << "PID"
              << std::setw(25) << "Tempo Total"
              << std::setw(25) << "Tempo Pronto" << std::endl;
    std::cout << "------------------------------------------------------------\n";

    for (const auto &process : all_processes)
    {
        int turnaround_time = process.get_end_time() - process.get_creation_time();
        int waiting_time = turnaround_time - process.burst_time;

        std::cout << std::left << std::setw(10) << process.get_pid()
                  << std::setw(25) << turnaround_time
                  << std::setw(25) << waiting_time << std::endl;
    }
}