#include <string.h>
#include "uart.h"
#include "ym2149.h"
#include "fx.h"

typedef struct _s_frame {
  uint16_t mask;
  uint8_t regs[16];
} FRAME;

// These are shared between the interrupt vector and the main loop,
// so we mark them volatile.
FRAME frame;  // well, expect for this one as it is "read-only"
uint8_t swTCNT0Max = 0;
volatile uint8_t nbFrames = 0;
volatile uint8_t syncFlag = 0;
volatile uint8_t swTCNT0 = 0;

// Song vector
ISR(TIMER0_COMPA_vect) {

  if (swTCNT0 ++ == swTCNT0Max) {
    swTCNT0 = 0;
///// @16MHz/64/(TCNTC0 + 1)/(swTCNT0Max + 1)
    if (nbFrames) {
      set_registers(frame.regs, frame.mask);
      fx_playYM6((uint8_t *)&(frame.regs));
      fx_playYM6((uint8_t *)&(frame.regs));
      nbFrames --;
    }

    syncFlag = 0;
//////
  }
}

void sync() {
  syncFlag = 1;
  while (syncFlag != 0);
}

///// @16MHz/64/(TCNTC0 + 1)/(swTCNT0Max + 1)

// | hwCounter | swCounter |        Freq (Hz) 
// |           |           | prediv=64   prediv=256
// |-----------------------------------|------------
// | 0         | 0         | 250000    | 62500
// | 7         | 7         | 3906.25   | 976.5625
// | 99        | 0         | 2500      | 625
// | 124       | 0         | 2000      | 500
// | 11        | 12        | 1602.56   | 400.64
// | 24        | 24        | 400       | 100
// | 49        | 24        | 200       | 50
// | 49        | 49        | 100       | 25
// | 49        | 99        | 50        | 12.5
// | 99        | 99        | 25        | 6.25
// | 199       | 99        | 12.5      | 3.125
// | 199       | 199       | 6.25      | 1.5625
// | 255       | 255       | 3.8...    | ~1

void setupSongTimer(uint16_t freqHz) {
  uint8_t hwCounter, swCounter;

  hwCounter = 49;
  swCounter = (5000 / freqHz) - 1;

  cli();

  swTCNT0Max = swCounter;
  //timer0 : song interrupt
  TCCR0A = 0;              // timer reset
  TCCR0B = 0;              // timer reset
  OCR0A = hwCounter;       // counter = ((16Mhz / 64) / freqHz) - 1
  TCCR0A |= (1 << WGM01);  //CTC mode
  TCCR0B |= ((1 << CS01) | (1 << CS00));   // timer ticks = clock ticks / 64
  TIMSK0 |= (1 << OCIE0A); // enable compare

  sei();
}

void clear_frame(FRAME *f)
{
  uint8_t rclear[] = {0, 0, 0, 0, 0, 0, 0, 0x40,
                      0, 0, 0, 0, 0, 0, 0, 0};
  f->mask = 0x7fff;
  memcpy(f->regs, rclear, 16);
}

void set_leds(uint8_t value, FRAME *f) {
  f->regs[7] |= 0x40; // Setup IOA as output
  f->regs[7] &= 0x7F; // Setup IOB as input

  f->regs[14] = value;
  // Set regs 7 & 14 for writing
  f->mask |= 0x4080;
}

void refresh_leds(FRAME *f) {
  uint8_t leds;

  // Have LED blink with noise (drums)
  leds = (~f->regs[7] & 0x38) ? 0x01 : 0x00;
  // Add ABC volume LEDs
  leds |= ((f->regs[8] & 0x08) >> 2) | ((f->regs[9] & 0x08) >> 1) | (f->regs[10] & 0x08);
  set_leds(leds, f);
}

int main()
{
  uint16_t i;
  uint8_t cmd;

  set_bus_ctl();
  initUART();
  setupSongTimer(10); // Start timer @10Hz by default (idle)
  // Mute and turn off leds
  clear_frame(&frame);  
  nbFrames = 1;

main_loop: 
  // sync to song interrupt
  sync();

  putByte(0);       // Tell the arduino we are Ready to RCV
  cmd = getByte();  // First byte is the CMD

  switch(cmd) {
    case 0:         // NOP
      break;
    case 1:         // recvRateHz
      setupSongTimer(getUShort());
      break;
    case 2:         // recvFrame
      // if (nbFrames == 1) {
      //   // TODO: error handling
      //   // Buffer overflow
      // }
      frame.mask = getUShort();
      for (i = 0; i < 16; i++) {      // Read masked registers, leave others unchanged
        if (frame.mask & (1 << i))
          frame.regs[i] = getByte();
      }
      refresh_leds(&frame);
      nbFrames = 1;
      break;
    case 3:       // clearFrame
      clear_frame(&frame);
      nbFrames = 1;
      break;
    default:
      // TODO: error handling
      // unknown CMD
      break;  
  }

  goto main_loop; // "Don't tell me what I can't do" - John L.

  return 0;
}
