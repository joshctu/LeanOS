#ifndef BUFFER_UTILS_H
#define BUFFER_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SBB_SIZE 1000

// We use this to assert which direction to shift the sbb.
typedef enum {
  DIR_UP,
  DIR_DOWN,
} Direction;

typedef uint16_t cell_t;
typedef cell_t line_t[80];
typedef line_t sbbuffer_instance_t[SBB_SIZE];

typedef struct scrollback_buffer {
  sbbuffer_instance_t *sbb_instance;
  int input_start;
  int sbb_head;
  int sbb_row_idx;
  int sbb_col_idx;
  int oldest_written_line;
  int newest_written_line;

} scrollback_buffer_t;

void sbb_init(scrollback_buffer_t *sbb);
int get_newest_written(scrollback_buffer_t *sbb);
int get_oldest_written(scrollback_buffer_t *sbb);
void set_newest_written(scrollback_buffer_t *sbb);
void set_oldest_written(scrollback_buffer_t *sbb);
int get_sbb_head(scrollback_buffer_t *sbb);
bool is_at_bottom(scrollback_buffer_t *sbb);
bool is_at_top(scrollback_buffer_t *sbb);
int sbb_write_line(scrollback_buffer_t *sbb, line_t *line);
int sbb_cursor_right(scrollback_buffer_t *sbb);
void sbb_write_char(scrollback_buffer_t *sbb, uint16_t value);
int sbb_reset(scrollback_buffer_t *sbb);
int shift_head(scrollback_buffer_t *sbb, int shift_by, Direction dir);
int snap_to_active_line(scrollback_buffer_t *sbb);
int sbb_new_line(scrollback_buffer_t *sbb);
void viewport_sync();

size_t get_buf_idx(size_t value, size_t buf_size);
size_t decrement_buf_idx(size_t value, size_t buf_size);
size_t increment_buf_idx(size_t value, size_t buf_size);

#endif
