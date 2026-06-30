#include "SamplesRingQueue.h"

SamplesRingQueue::SamplesRingQueue(unsigned short max_len, MbValueType type): max_len(max_len), type(type)
{

}

void SamplesRingQueue::append(Sample sample)
{
    queue.push_back(sample);
    if(queue.size() == max_len+1)
        queue.pop_front();
}

std::size_t SamplesRingQueue::size() const
{
    return queue.size();
}

Sample SamplesRingQueue::operator[](unsigned short index) const
{
    //occhio
    return Sample(queue[index]);
}
