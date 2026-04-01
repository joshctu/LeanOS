#include <kernel/io.h>
#include <kernel/pic.h>

void PIC_remap(void) {

  outb(PIC1_DATA, 0xFF);
  outb(PIC2_DATA, 0xFF);

  outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); // Send init bit and icw4 to follow
  io_wait();
  outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();
  outb(PIC1_DATA, PIC1_OFFSET); // Establish offset
  io_wait();
  outb(PIC2_DATA, PIC2_OFFSET);
  io_wait();
  outb(PIC1_DATA, 4); // Slave at IRQ2
  io_wait();
  outb(PIC2_DATA, 2); // Send IRQ for master to IRQ2
  io_wait();

  outb(PIC1_DATA, ICW4_8086); // 8086 mode
  io_wait();
  outb(PIC2_DATA, ICW4_8086);
  io_wait();

  outb(PIC1_DATA, 0xFD); // mask everything except keyboard
  outb(PIC2_DATA, 0xFF);
}
