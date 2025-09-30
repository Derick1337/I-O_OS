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