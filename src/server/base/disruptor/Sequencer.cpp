#include "Sequencer.h"
#include "ProcessingSequenceBarrier.h"

#include <algorithm>
#include <iostream>

using namespace base;

void Sequencer::AddGatingSequence(Sequence *sequence)
{
  std::shared_ptr<std::vector<Sequence*>> current;
  std::shared_ptr<std::vector<Sequence*>> updated;
  do
  {
    current = std::atomic_load_explicit(&gating_sequences_, std::memory_order_relaxed);
    if (current)
      updated.reset(new std::vector<Sequence*>(*current));
    else
      updated.reset(new std::vector<Sequence*>());
    updated->push_back(sequence);
  }
  while (!std::atomic_compare_exchange_weak_explicit(
        &gating_sequences_, &current, updated, std::memory_order_release, std::memory_order_relaxed));
}

void Sequencer::RemoveGatingSequence(Sequence *sequence)
{
  std::shared_ptr<std::vector<Sequence*>> current;
  std::shared_ptr<std::vector<Sequence*>> updated;
  do
  {
    current = std::atomic_load_explicit(&gating_sequences_, std::memory_order_relaxed);
    updated.reset(new std::vector<Sequence*>(*current));
    updated->erase(std::remove(updated->begin(), updated->end(), sequence), updated->end());
  }
  while (!std::atomic_compare_exchange_weak_explicit(
        &gating_sequences_, &current, updated, std::memory_order_release, std::memory_order_relaxed));
}

int64_t Sequencer::GetMinimumSequence()
{
  return Sequence::GetMinimumSequence(
      std::atomic_load_explicit(&gating_sequences_, std::memory_order_acquire), cursor_.Get());
}

SequenceBarrier* Sequencer::NewBarrier(const std::vector<Sequence*> &sequences)
{
  return new ProcessingSequenceBarrier(this, wait_strategy_, &cursor_, sequences);
}
