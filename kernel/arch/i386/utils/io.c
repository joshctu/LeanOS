#include <kernel/io.h>
#include <stdint.h>

// The outb asm instruction writes 1 byte from somewhere to to an io port.
//"a"(value) is saying use value and write it to the low byte of the eax
// register (al). "Nd"(port) is saying use port and write that as a 16 bit value
//(word) to the dx register
void outb(uint16_t port, uint8_t value) {
  __asm__ __volatile__("outb %b0, %w1"
                       :
                       : "a"(value), "Nd"(port)
                       : "memory"

  );
};

void outw(uint16_t port, uint16_t value) {
  __asm__ __volatile__("outw %w0, %w1"
                       :
                       : "a"(value), "Nd"(port)
                       : "memory"

  );
};

uint8_t inb(uint16_t port) {
  uint8_t ret;
  __asm__ __volatile__("inb %w1, %b0"
                       : "=a"(ret)
                       : "Nd"(port)
                       : "memory"

  );

  return ret;
};

uint16_t inw(uint16_t port) {
  uint16_t ret;
  __asm__ __volatile__("inw %w1, %w0"
                       : "=a"(ret)
                       : "Nd"(port)
                       : "memory"

  );
  return ret;
}

void io_wait(void) { outb(0x80, 0); };
