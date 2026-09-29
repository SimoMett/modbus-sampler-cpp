struct Segment
{
    uint32_t start;
    uint32_t end;
};

class ProducerWorker
{
public:
    virtual ~ProducerWorker() = 0;
    virtual void start() = 0;
    virtual void join() = 0;
    virtual void stop() = 0;

protected:
    virtual void run() = 0;

    virtual void fetch_and_push_words(const Segment &s) = 0;
    virtual void fetch_and_push_floats(const Segment &s) = 0;
    virtual void fetch_and_push_dwords(const Segment &s) = 0;
    virtual void fetch_and_push_coils(const Segment &s) = 0;
    virtual void fetch_and_push_bits(const Segment &s) = 0;
};