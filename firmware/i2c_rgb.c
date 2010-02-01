/********************************************************************************

I2C-controllable RBG LED driver in an ATTiny25/45/85

Created by Andrew T. Dodd
atd7 at cornell dot edu

---------------------------------------------------------------------------------

Based on Atmel Application Note AVR136: Low-Jitter Multi-Channel Software PWM

Uses Donald Blake's port of Atmel Application Note AVR312: Using the USI Module
as an I2C slave.  See usiTwiSlave.c for details.

This program is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

---------------------------------------------------------------------------------

Change Activity:

    Date       Description
   ------      -------------
  26 Dec 2009  Initial public release.
  27 Dec 2009  Added I2C timeout code
  28 Dec 2009  Added sigma-delta modulation and gamma curve application
  08 Jan 2010  Added #defines for common anode vs. common cathode

********************************************************************************/



/********************************************************************************

                                    includes

********************************************************************************/


#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>


/*
  include file for the USI I2C slave library
*/
#include "usiTwiSlave.h"

/*
  include file for the gamma curve, 8 bits t 12 bits
*/
#include <avr/pgmspace.h>
#include "gammacurve_8to12.h"

/*
  CPU fuses must be set to generate a 16 MHz internal clock by the PLL
  Set the CPU fuses with avrdude as follows for Tiny25/45/85,
  change -c and -p as appropriate for your programmer/part
  
  avrdude -c usbtiny -p t85 -U lfuse:w:0xf1:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m 
*/

#define F_CPU 16000000UL

/*
  3 dimming channels - R, G, B.  This is the most you can do along with I2C on a tiny85 without
  using the RSTDISBL fuse.  Set that fuse and say bye bye to ICSP programming - you will need a
  high voltage programmer to make any more changes.
*/

#define CHMAX 3

/*
  These were originally called CHn_CLEAR, but if you're driving common anode LEDs
  (low = on), you want to set bits HIGH to turn them off.

  Current code is for active-high, due to the fact that I only have one common
  cathode LED at the moment.  The 50 common anodes are still in the mail.
  
  CH0 is Red
  CH1 is Green
  CH2 is Blue
*/

//#define COMMON_CATHODE


#ifdef COMMON_CATHODE
#define CH0_CMPTGL (pinlevelB &= ~_BV(PB1)) /* map CH0 to PB1 */
#define CH1_CMPTGL (pinlevelB &= ~_BV(PB3)) /* map CH1 to PB3 */
#define CH2_CMPTGL (pinlevelB &= ~_BV(PB4)) /* map CH2 to PB4 */
#else
#define CH0_CMPTGL (pinlevelB |= _BV(PB1)) /* map CH0 to PB1 */
#define CH1_CMPTGL (pinlevelB |= _BV(PB3)) /* map CH1 to PB3 */
#define CH2_CMPTGL (pinlevelB |= _BV(PB4)) /* map CH2 to PB4 */
#endif

/*
  This is from AVR 136, however it is tweaked to also set PORTB/DDRB to "output"
  for the I2C ports.  They are usually in this state, the I2C driver will handle
  changing from these defaults.

  I am continually concerned that the PWM code and the I2C code will conflict,
  however the I2C code only directly touches PORTB/DDRB during slave init.
  After slave init, it seems that DDRB/PORTB remain high for the I2C pins.

  I2C pins are PB0 and PB2.
*/

#define PORTB_DDR _BV(PB4) | _BV(PB3) | _BV(PB1) |_BV(PB2) |_BV(PB0)

#ifdef COMMON_CATHODE
#define PORTB_MASK _BV(PB4) | _BV(PB3) | _BV(PB1) |_BV(PB2) |_BV(PB0)
#else
#define PORTB_MASK _BV(PB2) |_BV(PB0)
#endif

/*
  FIXME:  Move this to eeprom so it can be set during programming by avrdude from a script!
*/

#define DEFAULT_I2C_ADDRESS 0x32


/*
  Compare buffers used by the AVR136 softpwm code
*/

unsigned char compare[CHMAX];
volatile unsigned char compbuff[CHMAX];


/*
  Timer 0 ISR - handles the software PWM task.

  Based on the AVR136 ISR, but ported to GCC-AVR and reduced to three channels
  and PORTB only.

  AVR136 gave this a full 256 clock cycles per firing, but triple channel needs
  far less.  Change PWM_ISR_CYCLES define below to increase the number of ISR
  cycles allocated to the timer if you change the ISR to take longer.
  This will slow down your PWM base frequency.

  Also changed CHn_CLEAR to CHn_CMPTGL - See comments above in their #defines
*/

#define PWM_ISR_CYCLES 100

/*
  Flag raised by the PWM code to indicate an SDM update is needed
*/

uint8_t update_sdm = 0;

ISR (TIMER0_COMPA_vect)
{
  static unsigned char pinlevelB=PORTB_MASK;
  static unsigned char softcount=0xFF;

  PORTB = pinlevelB;

  if(++softcount == 0){         /* increment modulo 256 counter and update
				   the compare values only when counter = 0. */
    compare[0] = compbuff[0];   /* verbose code for speed */
    compare[1] = compbuff[1];
    compare[2] = compbuff[2];

    update_sdm = 1;             /* flag that it's time to update the SDM
				   for next cycle */

    pinlevelB = PORTB_MASK;     /* set all affected port pins high */
  }
  
  /*
    Toggle the appropriate channels when they compare
  */

  if(compare[0] == softcount) CH0_CMPTGL;
  if(compare[1] == softcount) CH1_CMPTGL;
  if(compare[2] == softcount) CH2_CMPTGL;
}

/*
  General task handling ISR.  Fires once per millisecond.
  
  Previously this handled an automated color cycling algorithm, now it handles I2C input.
  
  Processes one byte from the I2C buffer per cycle if present.

  Bad Things happened when I tried to do a full buffer flush in one cycle by changing
  if(usiTwiDataInReceiveBuffer()) to a while()
  
  Will reset the nbytes pointer if we haven't received any bytes in the last
  LASTBYTE_TIMEOUT_MS milliseconds.  Currently set to 10 ms - 100 kHz I2C should
  take less than a millisecond to transfer all bytes, but I am not planning on
  supporting 100 Hz update rates.
*/

#define LASTMSG_TIMEOUT_MS 5

#define TASKISRS_PER_MS 2

uint8_t lastmsgtime = 0;

uint16_t sdm_dimval[] = {4080,4080,4080}; /* Default to fullbright until we receive control */

uint8_t bytebuff[] = {0,0,0}; /* red, green, blue */

ISR (TIMER1_COMPA_vect)
{

  static uint8_t lastbytecount = 0;
  uint8_t idx2;



  /*
    variables only needed by the sigma delta modulator
    NOTE:  Due to a slight nonoptimality in my quantization
    approach, 4080 is fullbright and not 4095.  4095 will
    give WEIRD results, and from 4080 to 4094 will actually
    be dimmer than 4080

    This issue isn't visible to an I2C user, since they only
    provide indices into the gammacurve LUT, which was generated
    with this limitation in mind.
  */

  static int8_t sdm_error[] = {0,0,0};  /* Initialize the error accumulators to zero error */
  uint8_t temp;
  
  /*
    Update the sigma-delta modulator for the next PWM cycle
    This temporally dithers the 8-bit PWM output to achieve
    12-bit dimming
  */
  
  if(update_sdm)
    {
      /* Unset the SDM update flag */
      update_sdm = 0;
      
      /* Update the SDM for each channel */
      for(idx2 = 0; idx2 < 3; idx2++)
	{
	  /*
	    Perform quantization
	    In this case it will round down...
	  */
	  temp = (uint8_t) (sdm_dimval[idx2] >> 4);

	  /*
	    If our error accumulator is greater than or
	    equal to zero, increase the PWM output level
	    by one

	    Else leave it at the rounded down value
	  */
	  if(sdm_error[idx2] >= 0)
	    {
	      compbuff[idx2] = temp+1;
	    }
	  else
	    {
	      compbuff[idx2] = temp;
	    }

	  /*
	    Accumulate the error for this channel
	  */
	  sdm_error[idx2] += (sdm_dimval[idx2] - (compbuff[idx2] << 4));
	}
    }


  /*
    Check if there is any new data in the I2C receive buffer
    
    Reset the timer if there is
  */
  
  if(usiTwiBytesInReceiveBuffer() > lastbytecount)
    {
      lastmsgtime = 0;
      lastbytecount = usiTwiBytesInReceiveBuffer();
    }
  
  /*
    Flush if we have stuff but not complete and nothing new for 10 ms
  */

  if((++lastmsgtime == (LASTMSG_TIMEOUT_MS * TASKISRS_PER_MS)) && usiTwiBytesInReceiveBuffer())
    {
      lastmsgtime = 0;
      usiTwiFlushBuffer();
    }

}

/*
  Basic I/O initialization routine.  The idea of ionit() is borrowed from the AVR-LIBC demo code.
*/

void ioinit (void)		       
{
  uint8_t i2caddress;
  /*
    Make sure clock prescaler is disabled (clock is maximum)
    If fuses were set properly this shouldn't matter, but doing
    it just in case.  First thing to remove if you hit the 2k
    barrier of the Tiny25
  */
  CLKPR = _BV(CLKPCE);        // enable clock prescaler update
  CLKPR = 0;                    // set clock to maximum (= crystal)

  /*
    Set up timer 1 to generate interrupts every 0.5 ms

    This will become our "general tasks" interrupt.
  */

  /* clear timer on compare mode for timer 1 */
  TCCR1 = _BV(CTC1);

  /* Divide by 64 prescaler.  With F_CPU at 16 MHz, this timer will hit 125 after 0.5 ms */
  TCCR1 |= _BV(CS12) | _BV(CS11) | _BV(CS10);

  /*
    Set output compare register to 250/TASKISRS_PER_MS, so a compare interrupt will trigger
    every 1/TASKISRS_PER_MS milliseconds
    We should be fine with 1 ms as a full PWM cycle is 1.6 ms when PWM_ISR_CYCLES = 100
    but let's be safe.
   */
  OCR1A = 250/TASKISRS_PER_MS;

  /* Set timer 1 compare interrupt to enabled */
  TIMSK = _BV(OCIE1A);


  /*
    Set up timer 0 to fire an int every PWM_ISR_CYCLES clocks
  */

  /* No prescaler */
  TCCR0B = _BV(CS00);

  /* Clear timer on compare mode */
  TCCR0A = _BV(WGM01);

  /* Fire every PWM_ISR_CYCLES clock ticks */
  OCR0A = PWM_ISR_CYCLES;

  /* Int on compare A */
  TIMSK |= _BV(OCIE0A);

  
  /* Set LED pins as outputs */
  DDRB = PORTB_DDR;

  /*
    Initialize the I2C slave library with our address
  */

  /*
    Address stored in the first byte of the EEPROM
  */

  i2caddress = eeprom_read_byte(0);
  if((i2caddress < 2) || (i2caddress > 125))
    {
      i2caddress = DEFAULT_I2C_ADDRESS;
    }
  usiTwiSlaveInit(i2caddress);


  /*
    Power down the ADC.  We don't need it
  */

  PRR = _BV(PRADC);

  /*
    Enable interrupts
  */

  sei ();
}

int main (void)
{
  unsigned char idx;
  unsigned char initpwm = 0x0; //Default to off for all channels until the SDM update runs.

  /*
    Set up all I/O and timers/ISRs
  */

  ioinit ();

  /*
    FIXME:  Move this to ioinit()
  */

  for(idx=0 ; idx<CHMAX ; idx++)      // initialise all channels
    {
      compare[idx] = initpwm;           // set default PWM values
      compbuff[idx] = initpwm;          // set default PWM values
    }
  

  /*
    Loop forever, changing brightness whenever we get enough RGB data
    This was originally in the task ISR but weird things occasionally happened
    Bad news is we now consume 8 mA at all times, instead of going into sleepmode
    between ISRs.

    In theory it's a drop in the bucket compared to the LEDs, but in reality that's
    another 5 watts per string of 100 nodes.

    Things get interesting when you start scaling things into the hundreds!
  */
 

  for (;;)
    {
      if(usiTwiBytesInReceiveBuffer() >= 3)
	{
	  /* reset our last-byte timer */
	  lastmsgtime = 0;
	  
	  /* Pull data off of the buffer */
	  bytebuff[0] = usiTwiReceiveByte();
	  bytebuff[1] = usiTwiReceiveByte();
	  bytebuff[2] = usiTwiReceiveByte();
	  
	  /* Once we receive 3 bytes:
	     reset nbytes to 0
	     apply the gamma curve
	     copy the gamma expanded value to our sigma-delta dimming value
	     
	     Our gamma curve is in PROGMEM (flash) so we need to use
	     pgm_read_word to fetch the values.
	  */
	  
	  for(idx = 0; idx < 3; idx++)
	    {
	      sdm_dimval[idx] = pgm_read_word(&gammacurve[bytebuff[idx]]);
	    }
	  
	}
    }
  

  /*
    If you hit this line, something went HORRIBLY wrong!
    Keeps the compiler happy though.
  */
  return (0);
}
