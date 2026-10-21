#include "ConsumerWorker.h"

ConsumerWorker::ConsumerWorker() : should_close(false), is_running(false){}
ConsumerWorker::~ConsumerWorker()
{
    if(this->is_running)
    {
        this->should_close = true;
        this->run_thread->join();
    }
}