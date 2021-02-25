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

template<typename T>
class BoundedBlockingQueue
{
public:
  BoundedBlockingQueue(size_t capacity = 256)
      : m_capacity(capacity)
  {}

  BoundedBlockingQueue(BoundedBlockingQueue&& other)
      : BoundedBlockingQueue(other.capacity)
  {
    std::unique_lock lock{ m_consumers_mtx };
    m_data = std::move(other.m_data);
  }

  BoundedBlockingQueue(const BoundedBlockingQueue& other) = delete;
  ~BoundedBlockingQueue() {}

  /**
   * add will blocked until operation will be finished
   * @param element - new element for inserting to queue
   * can throw std::bad_alloc
   */
  void add(const T& element)
  {
    {
      std::unique_lock lock{ m_consumers_mtx };
      m_cv_queue_overflow.wait(lock, [&]() { return size() < m_capacity; });

      m_data.push(element);
    }
    m_cv_queue_empty.notify_all();
  }

  /**
   * add will blocked until operation will be finished
   * @param element - new elemenent for inserting to queue
   * can throw std::bad_alloc
   */
  void add(T&& element)
  {
    std::unique_lock lock{ m_consumers_mtx };
    m_cv_queue_overflow.wait(lock, [&]() { return !full(); });

    m_data.push(std::forward<T>(element));
    lock.unlock();
    m_cv_queue_empty.notify_all();
  }

  /**
   * thread safe method
   * @return object T from queue, if queue is empty, wait a new object
   */
  T take()
  {
    std::unique_lock lock{ m_consumers_mtx };

    m_cv_queue_empty.wait(lock, [&]() { return !m_data.empty(); });

    T el{ std::move(m_data.front()) };
    m_data.pop();
    m_cv_queue_overflow.notify_all();
    return el;
  }

  bool empty() const
  {
    std::unique_lock lock{ m_consumers_mtx };
    return m_data.empty();
  }

  bool full() const
  {
    std::unique_lock lock{ m_consumers_mtx };
    return m_data.size() == m_capacity;
  }

  size_t size() const
  {
    std::unique_lock lock{ m_consumers_mtx };
    return m_data.size();
  }

  size_t capacity() const { return m_capacity; }

private:
  const size_t m_capacity;
  mutable std::recursive_mutex m_consumers_mtx;

  std::condition_variable_any m_cv_queue_empty;
  std::condition_variable_any m_cv_queue_overflow;
  std::queue<T> m_data;
};
