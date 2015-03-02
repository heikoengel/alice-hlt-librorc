#include <librorc.h>
#include <iostream>

int main(int argc, char *argv[]) {
  librorc::device *dev = NULL;
  librorc::buffer *buf = NULL;

  uint64_t size = (1ull << 30);
  uint64_t id = 0;
  uint64_t overmap = 1;
  try {
    dev = new librorc::device(0);
  } catch (int e) {
    std::cerr << "device exception " << e << std::endl;
    abort();
  }

  try {
    buf = new librorc::buffer(dev, size, id, overmap);
  } catch (int e) {
    std::cerr << "buffer exception " << e << std::endl;
    abort();
  }

  bool err = false;

  while( !err ) {
      librorc::buffer *b2 = NULL;
      try {
          b2 = new librorc::buffer(dev, 0, 1);
      } catch (...) {
          err = true;
      }
      delete b2;
  }

  delete buf;
  delete dev;

  return 0;
}
