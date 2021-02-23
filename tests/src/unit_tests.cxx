#include <chrono>
#include <condition_variable>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include <blocking_queue.h>
auto sort_int = [](int i, int j) { return i < j; };

TEST(SimpleAddTake, BoundedBlockingQueue)
{
  size_t max_size = 64;
  BoundedBlockingQueue<int> queue(max_size);

  std::vector<int> data;
  data.resize(max_size);
  for (int i = 0; i < max_size; i++)
    data[i] = i;

  for (auto n : data)
    queue.add(n);

  std::vector<int> grabbed_data;
  for (int i = 0; i < max_size; i++) {
    int n = queue.take();
    grabbed_data.push_back(n);
  }
  std::sort(grabbed_data.begin(), grabbed_data.end(), sort_int);

  EXPECT_THAT(data, ::testing::ContainerEq(grabbed_data));
}

class SingleBoundedBlockingQueue : public testing::TestWithParam<::std::tuple<int, int, bool>>
{
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_P(SingleBoundedBlockingQueue, ReaderLock)
{
  size_t max_size = std::get<0>(GetParam());
  size_t element_count = std::get<1>(GetParam());
  bool sync = std::get<2>(GetParam());

  BoundedBlockingQueue<int> test(max_size);

  std::vector<int> data;
  data.resize(element_count);
  for (int i = 0; i < element_count; i++)
    data[i] = i;

  std::vector<int> grabbed_data;

  std::atomic<int32_t> flag;
  flag.store(-1);

  auto processor = [&] {
    for (int i = 0; i < element_count; i++) {
      int n = test.take();
      if (sync) {
        flag.store(i);
      }
      grabbed_data.push_back(n);
    }
  };

  std::thread thread(processor);

  while (flag != -1)
    ;
  for (int i = 0; i < element_count; i++) {
    if (sync) {
      if (i % 32) {
        while (flag != (i - 1))
          ; // blocks a thread to force the worker thread to wait for a new data
      }
    }
    test.add(data[i]);
  }

  thread.join();

  std::sort(grabbed_data.begin(), grabbed_data.end(), sort_int);

  EXPECT_THAT(data, ::testing::ContainerEq(grabbed_data));
}

INSTANTIATE_TEST_CASE_P(BoundedBlockingQueue,
                        SingleBoundedBlockingQueue,
                        ::testing::Combine(::testing::Values(512), ::testing::Values(512, 1024), ::testing::Values(false, true)));

TEST(QueueOverflow, BoundedBlockingQueue)
{
  size_t max_size = 128;
  size_t element_count = 512;
  bool sync = false;

  BoundedBlockingQueue<int> queue(max_size);

  std::vector<int> data;
  data.resize(element_count);
  for (int i = 0; i < element_count; i++)
    data[i] = i;

  std::vector<int> grabbed_data;

  std::atomic<int32_t> flag;
  flag.store(0);

  auto processor = [&] {
    for (int i = 0; i < element_count; i++) {
      queue.add(data[i]);
      flag++;
    }
  };

  std::thread thread(processor);

  while (flag < max_size)
    ;
  EXPECT_EQ(flag.load(), max_size);
  for (int i = 0; i < element_count; i++) {
    int n = queue.take();
    if (sync) {
      flag.store(i);
    }
    grabbed_data.push_back(n);
  }

  thread.join();

  std::sort(grabbed_data.begin(), grabbed_data.end(), sort_int);

  EXPECT_THAT(data, ::testing::ContainerEq(grabbed_data));
}

TEST(MultiThread, BoundedBlockingQueue)
{
  int max_size = 8;
  int element_count = 1024*1024;
  int producer_count = 4;
  int consumer_count = 5;

  BoundedBlockingQueue<int> queue(max_size);

  std::vector<int> data;
  data.resize(element_count);
  for (int i = 0; i < element_count; i++)
    data[i] = i;

  auto producer = [&](int begin, int end) {
    for (; begin < end; begin++) {
      queue.add(data[begin]);
    }
  };

  std::vector<std::thread*> producer_workers;
  producer_workers.resize(producer_count);
  int producer_chunk_size = element_count / producer_count;
  for (int i = 0; i < producer_count; i++) {
    int begin = producer_chunk_size * i;

    int end = producer_chunk_size * (i + 1);
    if ((producer_count - 1) == i)
      end = element_count;
    producer_workers[i] = new std::thread(producer, begin, end);
  }

  std::vector<int> grabbed_data;
  std::mutex grabbed_data_mutex;
  auto consumer = [&](int chunk_size) {
    std::vector<int> data;
    while (chunk_size--)
      data.push_back(queue.take());

    std::unique_lock lock(grabbed_data_mutex);
    grabbed_data.insert(grabbed_data.end(), data.begin(), data.end());
  };

  int consumer_chunk_size = element_count / producer_count;
  std::vector<std::thread*> consumer_workers;
  consumer_workers.resize(consumer_count);
  for (int i = 0; i < consumer_count; i++) {
    int chunk_size = consumer_chunk_size;
    if ((consumer_count - 1) == i)
      chunk_size = element_count - consumer_chunk_size * i;
    consumer_workers[i] = new std::thread(consumer, chunk_size);
  }

  for (auto p : producer_workers) {
    p->join();
    delete p;
  }

  for (auto c : consumer_workers) {
    c->join();
    delete c;
  }

  std::sort(grabbed_data.begin(), grabbed_data.end(), sort_int);

  EXPECT_THAT(data, ::testing::ContainerEq(grabbed_data));
}

TEST(RingQueue, BoundedBlockingQueue)
{
  RingQueue<uint32_t> ring(16);

  uint32_t add_counter = 0;
  uint32_t take_counter = 0;

  for (int j = 0; j < std::numeric_limits<int16_t>::max(); j++) {

    for (int i = 0; i < 16; i++) {
      ring.add(add_counter++);
    }

    for (int i = 0; i < 6; i++) {
      EXPECT_EQ(ring.take(), take_counter++);
      ring.add(add_counter++);
    }

    EXPECT_EQ(ring.size(), 16);

    for (int i = 0; i < 16; i++) {
      EXPECT_EQ(ring.take(), take_counter++);
    }
    EXPECT_EQ(ring.empty(), true);
  }
}