#include "../header/driver/keyboard.h"
#include "../header/text/framebuffer.h"
#include "../header/cpu/portio.h"
#include "../header/stdlib/string.h"

static struct KeyboardDriverState keyboard_state;

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
  if(!keyboard_state.keyboard_input_on) return;
  uint8_t scancode = in(KEYBOARD_DATA_PORT);
   
  char key = keyboard_scancode_1_to_ascii_map[scancode];

  if (key == 0) {
    pic_ack(IRQ_KEYBOARD);
    return;
  }
  
  else if (key == '\b')
  {
    if (keyboard_state.cursorColumn > keyboard_state.template_length) {
      keyboard_state.keyboard_buffer = key;
      /*
      framebuffer_write(keyboard_state.cursorRow, keyboard_state.cursorColumn-1, ' ', 0xF, 0x0);
      framebuffer_set_cursor(keyboard_state.cursorRow,keyboard_state.cursorColumn-1);
      keyboard_state.cursorColumn--;
      if(keyboard_state.cursorColumn==0){
        keyboard_state.cursorRow+=24;
        keyboard_state.cursorRow %=25;
        keyboard_state.cursorColumn = 80;
      }
      */
    }
  }
  else if (key == '\n'){
    keyboard_state.keyboard_buffer = key;
    //keyboard_state.cursorRow++ ;
    //keyboard_state.cursorRow %=25;
    //keyboard_state.cursorColumn = 0;
    //framebuffer_set_cursor(keyboard_state.cursorRow,keyboard_state.cursorColumn);
  }
  else{
    //framebuffer_write(keyboard_state.cursorRow, keyboard_state.cursorColumn, key, 0xFF, 0x0);
    //framebuffer_set_cursor(keyboard_state.cursorRow,keyboard_state.cursorColumn+1);
    keyboard_state.keyboard_buffer = key;
    //keyboard_state.cursorColumn++;
    /*
    if(keyboard_state.cursorColumn==81){
      keyboard_state.cursorRow++;
      keyboard_state.cursorColumn = 1;
      if(keyboard_state.cursorRow==25){
        keyboard_state.cursorRow=0;
      }
    }
    */
  }
  
  
  pic_ack(IRQ_KEYBOARD);
}

void puts(char* charp, uint8_t charcnt, uint8_t color){
  for(uint8_t i = 0; i < charcnt; i++){
    if (charp[i] == '\0') {
      break;
    }

    putchar(charp[i],color);
  }
}

void putchar(char c, uint8_t color){
  if(c == '\b'){
    if (keyboard_state.cursorColumn > 0) {
      framebuffer_write(keyboard_state.cursorRow, keyboard_state.cursorColumn-1, ' ', 0xF, 0x0);
      keyboard_state.cursorColumn--;
    }
    else if (keyboard_state.cursorColumn == 0 && keyboard_state.cursorRow > 0) {
      keyboard_state.cursorRow--;
      keyboard_state.cursorColumn = 80;
    }
  }else{
    if (c != '\n') {
      framebuffer_write(keyboard_state.cursorRow, keyboard_state.cursorColumn, c, color, 0x0);
      keyboard_state.cursorColumn++;
    }

    if (c == '\n' || keyboard_state.cursorColumn == 81) {
      keyboard_state.cursorRow++;
      keyboard_state.cursorRow %= 25;
      keyboard_state.cursorColumn = 0;
    }
  }
  framebuffer_set_cursor(keyboard_state.cursorRow, keyboard_state.cursorColumn);
}

// Activate keyboard ISR / start listen keyboard & save to buffer
void keyboard_state_activate(void) {
  keyboard_state.keyboard_input_on = true;
  keyboard_state.cursorColumn = 0;
  keyboard_state.cursorRow = 0;
  keyboard_state.keyboard_buffer = '\0';
  keyboard_state.template_length = 0;
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

int get_keyboard_col(){
  return (keyboard_state.cursorColumn);
}

int get_keyboard_row(){
  return (keyboard_state.cursorRow);
}

void change_keyboard_template_length(uint8_t x){
  keyboard_state.template_length = x;
}

void clear_screen(){
  framebuffer_clear();
  keyboard_state.cursorColumn = 0;
  keyboard_state.cursorRow = -1;
}

void testing(char c){
  framebuffer_write(24,79,c,0xA,0);
}