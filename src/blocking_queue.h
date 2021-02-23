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
class RingQueue
{
public:
  RingQueue(size_t max_capacity = 256)
  {
    if (max_capacity % 2 != 0) {
      throw std::invalid_argument("buffer size is not a power of 2");
    }
    m_data.resize(max_capacity);
  }

  void add(const T& element)
  {
    m_size++;
    const uint16_t pos = end_pos++ % m_data.size();
    get(pos) = element;
  }

  T take()
  {
    m_size--;
    const uint16_t pos = start_pos++ % m_data.size();
    return get(pos % m_data.size());
  }

  bool empty() const { return start_pos == end_pos; }
  size_t size() const { return std::abs((start_pos ^ 255) - (end_pos ^ 255)); }

private:
  int16_t size_mask() const { return m_data.size() - 1; }
  T& get(int pos) { return *((T*)&(m_data[pos].block)); }
  struct DataBlock
  {
    uint8_t block[sizeof(T)];
  };

  std::vector<DataBlock> m_data;
  uint16_t start_pos = 0;
  uint16_t end_pos = 0;
  uint16_t m_size = 0;
};

/**
 * Thread safe queue for writing and reading objects T from multiple threads
 * Queue should be destroyed after all readers and writers are finished
 * @tparam T
 */
template<typename T>
class BoundedBlockingQueue
{
public:
  BoundedBlockingQueue(size_t max_capacity = 256)
      : m_max_capacity(max_capacity), m_data(max_capacity)
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
    std::unique_lock lock{ consumers_mtx };
    m_cv_queue_overflow.wait(lock, [&]() { return m_data.size() < m_max_capacity; });

    m_data.add(std::move(element));
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

    m_data.add(std::move(element));
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

    const T& n = m_data.take();
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

  RingQueue<T> m_data;
  std::condition_variable_any m_cv_queue_empty;
  std::condition_variable_any m_cv_queue_overflow;
};