#include <blocking_queue.h>
#include <ring_queue.h>

int
main()
{
  RingQueue<uint32_t, 16> ring;
  ring.init();

  uint32_t add_counter = 0;
  uint32_t take_counter = 0;

  for (int j = 0; j < std::numeric_limits<int16_t>::max(); j++) {

    for (int i = 0; i < 16; i++) {
      ring.add(add_counter++);
    }

    for (int i = 0; i < 6; i++) {
      if (ring.take() != take_counter++)
        return -1;

      ring.add(add_counter++);
    }

    for (int i = 0; i < 16; i++) {
      if (ring.take() != take_counter++)
        return -1;
    }
    if (!ring.empty())
      return -1;
  }
  printf("Added %d elements\n", add_counter);
  return 0;
}