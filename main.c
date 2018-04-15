#include "uart.h"
#include "ym2149.h"

#define LED PORTB5  // Digital port 13 noted the board has an led on pin 13 already(should be yellow and be right below the pin)
void set_led_out(void) {
  DDRB |= 1 << LED;
}

void clear_registers(void) {
  send_data(6, 0);
  send_data(7, 0x80);
  send_data(9, 0);
  send_data(10, 0);
}

int main() {
  unsigned int i;
  unsigned char data[14];
  unsigned char cban_leds;

  set_ym_clock();
  set_bus_ctl();
  initUART();
  set_led_out();
  clear_registers();

  for/*ever*/(;;) {
    for (i=0; i<14; i++) {
      data[i] = getByte();
    }

    // Set IOA port as output
    data[7] |= 0x80;

    // Do not write r13 if it equals 0xff (prevent enveloppe reset)
    set_registers(data, data[13] == 0xff ? 12 : 13);

    // Have LED blink with noise (drums)
    cban_leds = (~data[7] & 0x38) ? 0x01 : 0x00;
    // Add ABC volume LEDs
    cban_leds |= ((data[8] & 0x08) >> 2) | ((data[9] & 0x08) >> 1) | (data[10] & 0x08);
    // Set IOA
    send_data(14, cban_leds);
  }

  return 0;
}
