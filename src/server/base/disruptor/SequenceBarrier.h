#ifndef BASE_DISRUPTOR_SEQUENCE_BARRIER_H
#define BASE_DISRUPTOR_SEQUENCE_BARRIER_H

namespace base
{
class SequenceBarrier
{
  public:
    virtual int64_t WaitFor(int64_t sequence) = 0;
    virtual int64_t GetCursor() const = 0;
    virtual bool IsAlerted() const = 0;
    virtual void Alert() = 0;
    virtual void ClearAlert() = 0;
};
}

#endif
