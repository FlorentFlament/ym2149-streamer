#include "uart.h"
#include "ym2149.h"
#include "fx.h"

volatile unsigned char SYNC = 1;

void clear_registers(void)
{
  unsigned char rclear[] = {0, 0, 0, 0, 0, 0, 0, 0x40,
                            0, 0, 0, 0, 0, 0, 0, 0};

  set_registers(rclear, 0xffff);
}

ISR(TIMER1_COMPA_vect) {
  SYNC = 0; 
}

void sync() {
  SYNC = 1;
  while(SYNC == 1);
}

void setup50HzTimer() {
    //timer1 : frameRateHz interrupt
    cli();
    TCCR1A = 0;              //timer reset
    TCCR1B = 0;              //timer reset
    OCR1A = 4999;            //period for 50 Hz at 16Mhz / 64
    TCCR1B |= (1 << WGM12);  //CTC mode
    TCCR1B |= ((1 << CS10) | (1 << CS11));   // timer ticks = clock ticks/64
    TIMSK1 |= (1 << OCIE1A); // enable compare
    sei();
}

int main()
{
  unsigned int i;
  unsigned int mask;
  unsigned char cmd;
  unsigned char r[16];
  // unsigned char dd_cmd, dd_sample;

  set_bus_ctl();
  initUART();
  //fx_setupTimer();
  setup50HzTimer();
  clear_registers();

  for/*ever*/(;;) {
    mask = 0;
    putByte(0); // Ready
    cmd = getByte();
    if (cmd == 0) {
      mask = getByte();               // Read mask MSB
      mask = (mask << 8) | getByte(); // Read mask LSB
      for (i = 0; i < 16; i++) {      // Read masked registers, leave others unchanged
        if (mask & (1 << i))
          r[i] = getByte();
      }
    }


    // // r3 free bits are used to code a DD start.
    // // r3 b5-b4 is a 2bits code wich means:
    // dd_cmd = (r[3] & 0x30) >> 4;

    // // 00:     No DD
    // // 01:     DD starts on voice A
    // // 10:     DD starts on voice B
    // // 11:     DD starts on voice C
    // if (dd_cmd)
    // {
    //   const char voice[4] = {0, 8, 9, 10};
    //   char dd_voice = voice[dd_cmd];

    //   // WARNING:
    //   // If a DD starts on voice V, the volume register corresponding to V (Ex r8 for voice A,
    //   // r9 for B and r10 for C) contains the sample number in 5 low bits (That mean you have
    //   // 32 digiDrum max in a song).
    //   dd_sample = r[(int)dd_voice] & 0x1f & 0;

    //   fx_playDigidrum(dd_sample, dd_voice);
    // }

    // YM Nano hardware I/O on port A and B of the  PSG
    r[7] |= 0x40; // Setup IOA as output
    r[7] &= 0x7F; // Setup IOB as input

    // Have LED blink with noise (drums)
    r[14] = (~r[7] & 0x38) ? 0x01 : 0x00;
    // Add ABC volume LEDs
    r[14] |= ((r[8] & 0x08) >> 2) | ((r[9] & 0x08) >> 1) | (r[10] & 0x08);
    // Set r[14]
    mask |= 0x4000;

    // sync to 50Hz interrupt
    sync();
    set_registers(r, mask);

  }

  return 0;
}
