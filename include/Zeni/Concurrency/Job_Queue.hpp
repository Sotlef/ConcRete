#ifndef ZENI_CONCURRENCY_IJOB_QUEUE_HPP
#define ZENI_CONCURRENCY_IJOB_QUEUE_HPP

#include "Internal/Linkage.hpp"

#include <memory>
#include <vector>

namespace Zeni::Concurrency {

  class IJob;
  class Thread_Pool;

  class Job_Queue {
    Job_Queue(const Job_Queue &) = delete;
    Job_Queue & operator=(const Job_Queue &) = delete;

  public:
    ZENI_CONCURRENCY_LINKAGE Job_Queue() = default;

    ZENI_CONCURRENCY_LINKAGE static std::shared_ptr<Job_Queue> Create(Thread_Pool * const thread_pool) noexcept;

    /// Take a Job off the queue. Will be null if and only if SHUT_DOWN.
    ZENI_CONCURRENCY_LINKAGE virtual std::shared_ptr<IJob> try_take_one(const bool is_already_awake) noexcept = 0;

    /// Give the queue a new Job. Can throw Job_Queue_Must_Not_Be_Shut_Down.
    ZENI_CONCURRENCY_LINKAGE virtual void give_one(const std::shared_ptr<IJob> job) noexcept(false) = 0;

    /// Give the queue many new Jobs. Can throw Job_Queue_Must_Not_Be_Shut_Down.
    ZENI_CONCURRENCY_LINKAGE virtual void give_many(std::vector<std::shared_ptr<IJob>> &&jobs) noexcept(false) = 0;

    /// Give the queue many new Jobs. Can throw Job_Queue_Must_Not_Be_Shut_Down.
    ZENI_CONCURRENCY_LINKAGE virtual void give_many(const std::vector<std::shared_ptr<IJob>> &jobs) noexcept(false) = 0;
  };

}

#endif
