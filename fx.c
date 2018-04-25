#include <avr/io.h>
#include <avr/interrupt.h>

#include "fx.h"
#include "samples.h"
#include "ym2149.h"

//Timer
const char *sampleOffset;
char sampleChannel;
int sampleCounter = 0;
int sampleLength = 0;

// ISR(TIMER1_COMPA_vect)
// {
//     if (sampleCounter < sampleLength) {
//         send_data(sampleChannel, pgm_read_byte_near(sampleOffset + sampleCounter++));
//     }
// }

void fx_setupTimer()
{
    //timer1 : sample player
    cli();
    TCCR1A = 0;              //timer reset
    TCCR1B = 0;              //timer reset
    OCR1A = 1451;            //period for 11025 Hz at 16Mhz
    TCCR1B |= (1 << WGM12);  //CTC mode
    TCCR1B |= (1 << CS10);   // timer ticks = clock ticks
    TIMSK1 |= (1 << OCIE1A); // enable compare
    sei();
}

void fx_playDigidrum(char index, char channel)
{
    cli();
    sampleCounter = 0;
    sampleChannel = channel;
    if (index == 0)
    {
        sampleOffset = s0;
        sampleLength = s0Length;
    }
    else if (index == 1)
    {
        sampleOffset = s1;
        sampleLength = s1Length;
    }
    else if (index == 2)
    {
        sampleOffset = s2;
        sampleLength = s2Length;
    }
    else if (index == 3)
    {
        sampleOffset = s3;
        sampleLength = s3Length;
    }
    else if (index == 4)
    {
        sampleOffset = s4;
        sampleLength = s4Length;
    }
    sei();
}
