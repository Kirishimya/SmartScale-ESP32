#include <cassert>
#include <iostream>
#include <vector>

#include <slave/FlashQueue.h>

int main() {
  FlashQueue queue(4);
  const std::vector<uint8_t> payload{1, 2, 3, 4};

  assert(queue.push(payload));
  assert(queue.size() == 1);

  std::vector<uint8_t> out;
  assert(queue.popReady(out, 0, 1000));
  assert(out == payload);
  assert(queue.size() == 1);

  assert(!queue.popReady(out, 0, 1000));
  assert(queue.popReady(out, 1000, 1000));
  assert(out == payload);

  queue.markAcked();
  assert(queue.empty());

  std::cout << "flash queue retry test passed\n";
  return 0;
}
