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

class memory
{
    protected:
    
    void local_politic(bool local, bool global)
    {
        if (local)
        {
            for (size_t i = 0; < proceses.size(); i++)
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
            }
        }

        if (global)
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

        }
    }

}