#include "../header/driver/keyboard.h"
#include "../header/text/framebuffer.h"
#include "../header/cpu/portio.h"
#include "../header/stdlib/string.h"

static struct KeyboardDriverState keyboard_state;

int cursorRow = 0;
int cursorColumn =0;



const char keyboard_scancode_1_to_ascii_map[256] = {
      0, 0x1B, '1', '2', '3', '4', '5', '6',  '7', '8', '9',  '0',  '-', '=', '\b', '\t',
    'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',  'o', 'p', '[',  ']', '\n',   0,  'a',  's',
    'd',  'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\',  'z', 'x',  'c',  'v',
    'b',  'n', 'm', ',', '.', '/',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '-',    0,    0,   0,  '+',    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,

      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};

void keyboard_isr(void) {
  uint8_t scancode = in(KEYBOARD_DATA_PORT);
   
  char key = keyboard_scancode_1_to_ascii_map[scancode];

  if (key == 0) {
    pic_ack(IRQ_KEYBOARD);
    return;
  }
  
  else if (key == '\b')
  {
    if (cursorColumn > 0) {
      framebuffer_write(cursorRow, cursorColumn-1, ' ', 0xF, 0x0);
      framebuffer_set_cursor(cursorRow,cursorColumn-1);
      cursorColumn--;
    }
  }
  else if (key == '\n'){
    cursorColumn = (cursorColumn+80)-(cursorColumn+80)%80;
    framebuffer_set_cursor(cursorRow,cursorColumn);
  }
  else{
    framebuffer_set_cursor(cursorRow,cursorColumn+1);
    keyboard_state.keyboard_buffer = key;
    cursorColumn++;
  }
  
  
  pic_ack(IRQ_KEYBOARD);
}

// Activate keyboard ISR / start listen keyboard & save to buffer
void keyboard_state_activate(void) {
  keyboard_state.keyboard_input_on = true;
}

// Deactivate keyboard ISR / stop listening keyboard interrupt
void keyboard_state_deactivate(void) {
  keyboard_state.keyboard_input_on = false;
}

// Get keyboard buffer value and flush the buffer - @param buf Pointer to char buffer
void get_keyboard_buffer(char *buf) {
  *buf = keyboard_state.keyboard_buffer;
  keyboard_state.keyboard_buffer = '\0';
}