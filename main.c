#include "uart.h"
#include "ym2149.h"

#define LED PORTB5  // Digital port 13 noted the board has an led on pin 13 already(should be yellow and be right below the pin)
void set_led_out(void) {
  DDRB |= 1 << LED;
}

void clear_registers(void) {
  int i;
  for (i=0; i<14; i++) {
    send_data(i, 0);
  }
}

int main() {
  unsigned int i;
  unsigned char data[16];

  set_ym_clock();
  set_bus_ctl();
  initUART();
  set_led_out();
  clear_registers();

  for/*ever*/(;;) {
    for (i=0; i<16; i++) {
      data[i] = getByte();
    }

    // Working around envelope issue (kind of). When writing on the
    // envelope shape register, it resets the envelope. This cannot be
    // properly expressed with the YM file format. So not using it.
    // Thanks sebdel: https://github.com/sebdel
    for (i=0; i<13; i++) {
      send_data(i, data[i]);
    }

    // Have LED blink with noise (drums)
    if (~data[7] & 0x38) {
      PORTB |=   1 << LED;
    } else {
      PORTB &= ~(1 << LED);
    }
  }

  return 0;
}
