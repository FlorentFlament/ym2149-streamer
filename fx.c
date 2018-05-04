#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#include "fx.h"
#include "ym2149.h"

#define MFP_CLOCK 2475600L
#define MFP2AVR (16000000/MFP_CLOCK)    // Arduino@16MHz / MFP@2.4576MHz

void fx_read(uint8_t *regs, uint16_t code, uint16_t prediv, uint16_t count);

// MFP predivisor
static const int mfpPrediv[8] = {0, 4, 10, 16, 50, 64, 100, 200};

//Timer
const unsigned char *sampleOffset;
char sampleChannel;
int sampleCounter = 0;
int sampleLength = 0;

uint16_t dd_size;
uint8_t *drums;
uint16_t nbDrums;
uint16_t dd_offsets[32], dd_lengths[32];

ISR(TIMER1_COMPA_vect)
{
  if (sampleCounter < sampleLength) {
    send_data(sampleChannel, sampleOffset[sampleCounter++]);
  }
}

void fx_drumStart(uint8_t __attribute__((unused))voice, uint8_t *__attribute__((unused))pDrumBuffer, uint32_t __attribute__((unused))drumSize,
 __attribute__((unused))uint16_t drumFreq) {
	if ((pDrumBuffer) && (drumSize)) {
		// specialEffect[voice].drumData = pDrumBuffer;
		// specialEffect[voice].drumPos = 0;
		// specialEffect[voice].drumSize = drumSize;
		// specialEffect[voice].drumStep = (drumFreq<<DRUM_PREC)/replayFrequency;
		// specialEffect[voice].bDrum = YMTRUE;
	}
}

void fx_drumStop(uint8_t __attribute__((unused))voice) {
		// specialEffect[voice].bDrum = YMFALSE;
}

void fx_sidStart(uint8_t __attribute__((unused))voice, uint16_t __attribute__((unused))timerFreq, uint16_t __attribute__((unused))vol) {
		// float tmp = (float)timerFreq * ((float)(1 << 31)) / (float)replayFrequency;

		// specialEffect[voice].sidStep = (ymu32)tmp;
		// specialEffect[voice].sidVol = vol&15;
		// specialEffect[voice].bSid = YMTRUE;
}

void fx_sidSinStart(uint8_t __attribute__((unused))voice, uint16_t __attribute__((unused))timerFreq, uint16_t __attribute__((unused))vol) {
	// TODO
}

void fx_sidStop(uint8_t __attribute__((unused))voice) {
		// specialEffect[voice].bSid = YMFALSE;
}

void fx_syncBuzzerStart(uint16_t __attribute__((unused))timerFreq, uint16_t __attribute__((unused))_envShape) {
		// ymfloat tmp = (ymfloat)timerFreq*((ymfloat)(1<<31))/(ymfloat)replayFrequency;

		// envShape = _envShape&15;
		// syncBuzzerStep = (ymu32)tmp;
		// syncBuzzerPhase = 0;
		// bSyncBuzzer = YMTRUE;
}

void fx_syncBuzzerStop(void) {
		// bSyncBuzzer = YMFALSE;
		// syncBuzzerPhase = 0;
		// syncBuzzerStep = 0;
}

void fx_playYM6(uint8_t *regs) {
  fx_read(regs, 1, 6, 14);
  fx_read(regs, 3, 8, 15);
}

void fx_read(uint8_t *regs, uint16_t code, uint16_t prediv, uint16_t count) {
  uint16_t voice;
  uint16_t ndrum;

  code = regs[code] & 0xf0;
  prediv = (regs[prediv] >> 5) & 7;
  count = regs[count];

  if (code & 0x30) {
    uint32_t tmpFreq;

    voice = ((code & 0x30) >> 4) - 1;
    prediv = mfpPrediv[prediv];
    prediv *= count;
    if (prediv) {
      tmpFreq = MFP_CLOCK / prediv;
      
      switch (code & 0xc0)
      {
        case 0x00:		// SID
          fx_sidStart(voice, tmpFreq, regs[voice + 8] & 15);
          break;
        case 0x80:		// Sinus-SID
          fx_sidSinStart(voice, tmpFreq, regs[voice + 8] & 15);
          break;
        case 0x40:		// DigiDrum
          ndrum = regs[voice + 8] & 31;
          if (ndrum < nbDrums) {
            fx_drumStart(voice, drums/*[ndrum].data*/, 0 /*drums[ndrum].size*/, tmpFreq);
          }
          break;
        case 0xc0:		// Sync-Buzzer.
          fx_syncBuzzerStart(tmpFreq, regs[voice + 8] & 15);
          break;
      }
    }
  }
}

void fx_loadDigidrum() {
    int n, length;

    length = getByte();
    length = (length << 8) & getByte();

    if (1500 - dd_size < (unsigned int)length) {  // If we're out of memory, repeat last sample
        dd_offsets[nbDrums] = dd_size;
        dd_lengths[nbDrums] = dd_lengths[nbDrums - 1];
        nbDrums ++;
        // We still have to flush the cmd
        for (n = 0; n < length; n++) {
            getByte();
        }
        return;
    }
    // load sample  
    dd_offsets[nbDrums] = dd_size;
    dd_lengths[nbDrums ++] = length;
    for (n = 0; n < length; n++) {
        drums[dd_size++] = getByte();
    }
}

void fx_playDigidrum(char index, char channel, unsigned int divider)
{
    cli();
    TCCR1A = 0;              //timer reset
    TCCR1B = 0;              //timer reset
    OCR1A = divider;
    TCCR1B |= (1 << WGM12);  //CTC mode
    TCCR1B |= (1 << CS10);   // timer ticks = clock ticks
    TIMSK1 |= (1 << OCIE1A); // enable compare

    sampleCounter = 0;
    sampleChannel = channel;
    sampleOffset = drums + dd_offsets[(int)index];
    sampleLength = dd_lengths[(int)index];
    sei();
}
