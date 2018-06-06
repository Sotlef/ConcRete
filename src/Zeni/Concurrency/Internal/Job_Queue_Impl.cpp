#include "Zeni/Concurrency/Internal/Job_Queue_Impl.hpp"

#include "Zeni/Concurrency/IJob.hpp"
#include "Zeni/Concurrency/Thread_Pool.hpp"

#ifndef DISABLE_MULTITHREADING
#include <thread>
#endif

namespace Zeni::Concurrency {

  Job_Queue_Impl::Job_Queue_Impl(Thread_Pool * const thread_pool) noexcept
    : m_thread_pool(thread_pool)
  {
  }

  std::shared_ptr<Job_Queue_Impl> Job_Queue_Impl::Create(Thread_Pool * const thread_pool) noexcept {
    class Friendly_Job_Queue_Impl : public Job_Queue_Impl {
    public:
      Friendly_Job_Queue_Impl(Thread_Pool * const thread_pool) : Job_Queue_Impl(thread_pool) {}
    };

    return std::make_shared<Friendly_Job_Queue_Impl>(thread_pool);
  }

  std::shared_ptr<IJob> Job_Queue_Impl::try_take_one(const bool is_already_awake) noexcept {
#ifndef DISABLE_MULTITHREADING
    if (!m_has_jobs.load(std::memory_order_acquire))
      return nullptr;

    std::unique_lock<std::mutex> mutex_lock(m_mutex);
#endif

    if (m_jobs.empty())
      return nullptr;

    const std::shared_ptr<IJob> job = m_jobs.front().back();
    m_jobs.front().pop_back();

    if (m_jobs.front().empty())
      m_jobs.pop();

    if (!is_already_awake)
      m_thread_pool->worker_awakened();

    if (m_jobs.empty())
      m_thread_pool->job_queue_emptied();

    job->set_Job_Queue(shared_from_this());

    return job;
  }

  void Job_Queue_Impl::give_one(const std::shared_ptr<IJob> job) noexcept(false) {
#ifndef DISABLE_MULTITHREADING
    std::lock_guard<std::mutex> mutex_lock(m_mutex);
#endif

    [[maybe_unused]] const bool nonemptied = m_jobs.empty();

    m_jobs.emplace(1, job);

#ifndef DISABLE_MULTITHREADING
    if (nonemptied) {
      m_has_jobs.store(true, std::memory_order_release);
      m_thread_pool->job_queue_nonemptied();
    }
#endif
  }

  void Job_Queue_Impl::give_many(std::vector<std::shared_ptr<IJob>> &&jobs) noexcept(false) {
    if (jobs.empty())
      return;

#ifndef DISABLE_MULTITHREADING
    std::lock_guard<std::mutex> mutex_lock(m_mutex);
#endif

    [[maybe_unused]] const bool nonemptied = m_jobs.empty();

    m_jobs.emplace(std::move(jobs));

#ifndef DISABLE_MULTITHREADING
    if (nonemptied) {
      m_has_jobs.store(true, std::memory_order_release);
      m_thread_pool->job_queue_nonemptied();
    }
#endif
  }

  void Job_Queue_Impl::give_many(const std::vector<std::shared_ptr<IJob>> &jobs) noexcept(false) {
    if (jobs.empty())
      return;

#ifndef DISABLE_MULTITHREADING
    std::lock_guard<std::mutex> mutex_lock(m_mutex);
#endif

    [[maybe_unused]] const bool nonemptied = m_jobs.empty();

    m_jobs.emplace(jobs);

#ifndef DISABLE_MULTITHREADING
    if (nonemptied) {
      m_has_jobs.store(true, std::memory_order_release);
      m_thread_pool->job_queue_nonemptied();
    }
#endif
  }

}
