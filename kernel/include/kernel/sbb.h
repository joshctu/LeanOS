#ifndef _SBB_H
#define _SBB_H
#include <stddef.h>
#include <stdint.h>

#define SBB_SIZE 1000

// We use this to assert which direction to shift the sbb.
typedef enum {
  DIR_UP,
  DIR_DOWN,
} Direction;

typedef uint16_t cell_t;
typedef cell_t line_t[80];
typedef line_t scrollback_buffer_t[SBB_SIZE];

void sbb_init(void);
int get_newest_written();
int get_oldest_written();
void set_newest_written();
void set_oldest_written();
int get_sbb_head();
void viewport_sync();
void sbb_write_char(int sbb_row_idx, int sbb_col_idx, uint16_t value);
void sbb_write_line(int sbb_row_idx, line_t *line);
void sbb_reset();
void shift_head(int shift_by, Direction dir);
void snap_to_active_line(int sbb_row_idx);

#endif
