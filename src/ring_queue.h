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

template<typename T = uint8_t, size_t SIZE = 256>
class RingQueue
{
  typedef uint16_t index_t;

public:
  RingQueue() noexcept { static_assert(SIZE % 2 == 0, "buffer size is not a power of 2"); }

  RingQueue(RingQueue&& other)
  {
    std::unique_lock lock(other.m_data_mutex);
    m_data = std::move(other.m_data);
    m_start_pos = other.m_start_pos;
    m_end_pos = other.m_end_pos;
  }

  RingQueue(const RingQueue& other) = delete;
  ~RingQueue() { reset(); }
  /**
   * The function must be called once before using the queue
   * @return false if queue is failed to allocate buffer
   */
  [[nodiscard]] bool init() noexcept
  {
    m_data.reset(new (std::nothrow) ElementBlock[SIZE]);

    if (m_data) {
      reset();
    }
    return m_data.get() != nullptr;
  }

  /**
   *
   * @param element
   * @return true if the element is added to the queue
   */
  [[nodiscard]] bool add(T&& element) noexcept
  {
    std::unique_lock lock(m_data_mutex);

    if (full())
      return false;

    index_t pos = m_end_pos++ & m_mask;
    new (get(pos)) T(std::forward<T>(element));
    return true;
  }

  /**
   *
   * @param element
   * @return true if the element is added to the queue
   */
  [[nodiscard]] bool add(const T& element) noexcept
  {
    std::unique_lock lock(m_data_mutex);

    if (full())
      return false;

    index_t pos = m_end_pos++ & m_mask;
    new (get(pos)) T(element);
    return true;
  }
  /**
   *
   * @return object T if the queue is not empty
   */
  std::optional<T> take() noexcept
  {
    std::unique_lock lock(m_data_mutex);

    if (empty())
      return {};

    index_t pos = m_start_pos++ & m_mask;
    T* obj = get(pos);
    T v{ std::move(*obj) };
    obj->~T();
    return { std::move(v) };
  }

  bool empty() const noexcept
  {
    std::unique_lock lock(m_data_mutex);
    return m_start_pos == m_end_pos;
  }

  bool full() const noexcept
  {
    std::unique_lock lock(m_data_mutex);
    uint16_t mask = ~m_mask;
    uint16_t delta = m_end_pos - m_start_pos;
    return (delta & mask) != 0;
  }

  void reset() noexcept
  {
    std::unique_lock lock(m_data_mutex);

    while (m_start_pos != m_end_pos) {
      get(m_start_pos++)->~T();
    }

    m_start_pos = 0;
    m_end_pos = 0;
  }

  size_t capacity() const { return m_capacity; }

private:
  T* get(index_t index) noexcept { return ((T*)&(m_data[index].blocks)); }
  struct ElementBlock
  {
    uint8_t blocks[sizeof(T)];
  };

  index_t m_start_pos = 0;
  index_t m_end_pos = 0;
  const index_t m_capacity = SIZE;
  const index_t m_mask = m_capacity - 1;
  mutable std::recursive_mutex m_data_mutex;
  std::unique_ptr<ElementBlock[]> m_data;
};
