#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <forward_list>
#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <thread>

/**
 * Thread safe queue for writing and reading objects T from multiple threads
 * Queue should be destroyed after all readers and writers are finished
 * @tparam T
 */
template<typename T>
class BoundedBlockingQueue
{
public:
  BoundedBlockingQueue(size_t max_capacity = 255)
      : m_max_capacity(max_capacity)
  {

  }

  ~BoundedBlockingQueue()
  {
  }

  /**
 *
 * @param element - new elemenent for inserting to queue
 * can throw std::bad_alloc
 */
  void add(const T& element)
  {
    //    std::shared_lock lock{ consumers_mtx };
    std::unique_lock lock{ consumers_mtx };
    m_cv_queue_overflow.wait(lock, [&]() { return m_data.size() < m_max_capacity; });

    m_data.push(element);
    lock.unlock();
    m_cv_queue_empty.notify_all();
  }

  /**
   *
   * @param element - new elemenent for inserting to queue
   * can throw std::bad_alloc
   */
  void add(T&& element)
  {
    std::unique_lock lock{ consumers_mtx };
    m_cv_queue_overflow.wait(lock, [&]() { return m_data.size() < m_max_capacity; });

    m_data.push(std::move(element));
    lock.unlock();
    m_cv_queue_empty.notify_all();
  }
/**
 * thread safe method
 * @return object T from queue, if queue is empty, wait a new object
 */
  T take()
  {
    std::unique_lock lock{ consumers_mtx };
    m_cv_queue_empty.wait(lock, [&]() { return !m_data.empty(); });

    const T& el = m_data.front();
    m_data.pop();
    lock.unlock();

    m_cv_queue_overflow.notify_all();
    return el;
  }

  bool empty() const{
    std::unique_lock lock{ consumers_mtx };
    return m_data.empty();
  }

private:
  size_t m_max_capacity;
  std::shared_mutex consumers_mtx;

  std::queue<T> m_data;
  std::condition_variable_any m_cv_queue_empty;
  std::condition_variable_any m_cv_queue_overflow;
};
