class final_results {
    public:
    int turnaround_time;
    int time_in_ready;
    int time_in_blocked;
    int burst_time;

    void Round_Robin::print_final_report() {
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
};