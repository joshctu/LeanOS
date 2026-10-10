#include <kernel/io.h>
#include <kernel/pit.h>

// The below function will load the pit with a 16value
// that the pic data port can take.

void pit_mode_set(uint8_t command_flag) { outb(PIT_COMMAND, command_flag); }

void pit_reload(uint16_t count) {

  outb(PIT_COMMAND, SQUARE_COUNT); // Prepare pic for following reload value

  outb(PIT_CHANNEL_0, count & 0xFF);        // Low byte write to data port.
  outb(PIT_CHANNEL_0, (count >> 8) & 0xFF); // High byte write to data port
  return;
}
