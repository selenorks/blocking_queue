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
  BoundedBlockingQueue(size_t max_capacity = 255)
      : m_max_capacity(max_capacity)
  {
    //    m_data.resize(max_capacity);
  }

  ~BoundedBlockingQueue()
  {
    m_is_stopped = true;
    m_cv_queue_empty.notify_all();
    m_cv_queue_overflow.notify_all();
  }

  // can throw std::bad_alloc
  void add(const T& element)
  {
    //    std::shared_lock lock{ consumers_mtx };
    std::unique_lock lock{ consumers_mtx };
    m_cv_queue_overflow.wait(lock, [&]() { return m_data.size() < m_max_capacity; });

    m_data.push(element);
    lock.unlock();
    m_cv_queue_empty.notify_all();
  }

  // can throw std::bad_alloc
  void add(T&& element)
  {
    std::unique_lock lock{ consumers_mtx };
    m_cv_queue_overflow.wait(lock, [&]() { return m_data.size() < m_max_capacity; });

    m_data.push(std::move(element));
    lock.unlock();
    m_cv_queue_empty.notify_all();
  }

  T take()
  {
    std::unique_lock lock{ consumers_mtx };
    m_cv_queue_empty.wait(lock, [&]() { return !m_data.empty(); });

    T n = m_data.front();
    m_data.pop();
    m_cv_queue_overflow.notify_all();
    return n;
  }

  bool empty() const{
    std::unique_lock lock{ consumers_mtx };
    return m_data.empty();
  }

private:
  size_t m_max_capacity;
  std::shared_mutex consumers_mtx;

  std::queue<T> m_data;
//  std::vector
  std::condition_variable_any m_cv_queue_empty;
  std::condition_variable_any m_cv_queue_overflow;
  bool m_is_stopped = false;
};