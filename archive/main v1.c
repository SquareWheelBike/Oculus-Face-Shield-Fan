/*
 * File:   main.c
 * Author: colefuerth
 *
 * Created on April 6, 2021, 2:56 PM
 */


#include <xc.h>
// #include <math.h>
#include <limits.h>


// function call memory bits available
	int oneShotBits[8];			// oneshot bits available for use for oneshot or toggle calls
										// BITS 32-63 ARE FOR FAULTS ONLY!!	
	int ONSTracker;
    unsigned long milliseconds = 0;
    #define _XTAL_FREQ 48000000 // 48MHz must be declared to use _delay() function
    unsigned long millis() { return milliseconds; }
    

    
// defines for XC8 compiler
#define Switch PORTBbits.RB6    // read from the port register
#define Switch_TRIS TRISBbits.TRISB6 // data direction control register
#define Motor LATCbits.LATC0    // read from the port register
#define Motor_TRIS TRISCbits.TRISC0 // data direction control register
#define LED LATCbits.LATC6    // read from the port register
#define LED_TRIS TRISCbits.TRISC6 // data direction control register


    /*
int FlasherBit(float hz)
{
	int T = round(1000.0 / hz);
	if ( millis() % T >= T/2 ) return 1;
	else return 0;
}*/

int oneShot(int precond, int OSR)
{
	// use global memory to keep track of oneshot bits
	if (precond == 1 && oneShotBits[OSR] == 0)
	{
		oneShotBits[OSR] = 1;
		return 1;
	}
	else if (precond == 0 && oneShotBits[OSR] == 1)
	{
		oneShotBits[OSR] = 0;
		return 0;
	}
	else return 0;
}

	int timerInSession[8];		// for speed, so we only update timer timers when needed
	int timerMemory[sizeof(timerInSession) / sizeof(int)];			// make function calls smaller by remembering previous output state
	unsigned long timerTimers[sizeof(timerInSession) / sizeof(int)];	// debounce timers available for use
	int timerTracker;

int TON(int input, int preset, int timerNumber)
{
	if (input && !timerInSession[timerNumber]) timerTimers[timerNumber] = millis();
	else if (input && timerMemory[timerNumber]) return 1;
	else if (input && millis() - timerTimers[timerNumber] >= preset) 
	{
		timerMemory[timerNumber] = 1;
		return 1;
	}
	else;
	timerMemory[timerNumber] = 0;
	timerInSession[timerNumber] = input;
	return 0;
}


void main(void) {
    
    // set latch states
    LED_TRIS = 0;   // set LED latch as output
    Motor_TRIS = 0; // set motor PWM to output
    Switch_TRIS = 1;    // set switch latch as input
    
    // set initial output states
    LED = 1;
    Motor = 0;
    
    int mode = 0;
    int numBlinks = 0;
    int donePulsing = 0;
    unsigned long stopper = ULONG_MAX / 2 - 1;
    
    while(1)
    {
        // start of loop instructions
        ONSTracker = 0; // reset tracker for each loop
        timerTracker = 0;
        
        // Main code here
        
        // mode change condition
        if (oneShot(TON(Switch, 20, timerTracker++), ONSTracker++))
        {
            mode++; // increment to next mode
            if (mode > 5)   
                mode = 0;
        }
        
        // indicate mode on LED
        if (oneShot(millis() % 200 > 100, ONSTracker++) && !donePulsing) // 5 pulses per second, for a 2.5Hz LED pulse
        {
            if (!LED) // if led is on, then turn off, increment pulse counter, and check for done pulsing
            {
                LED = 1; // turn off LED
                numBlinks++;
                if (numBlinks >= mode)
                {
                    numBlinks = 0;
                    donePulsing = 1;
                }
            }
            else if (mode)  // if LED is already off, then turn it on
                LED = 0;
        }
        if (TON(donePulsing, 1000, timerTracker++))
        {
            donePulsing = 0;
        }
        
        // generate PWM duty cycle output on a 10ms wave
        if (millis() % 10 >= mode * 2)   // off condition
        {
            Motor = 0;
            //LED = 1;
        }
        else
        {
            Motor = 1;
            //LED = 0;
        }
        
        __delay_ms(1);
        milliseconds++; // increment millis() return. Cursed, I know, but does not need to be accurate
        if (milliseconds == stopper) // make sure it doesnt hit an overflow
            milliseconds = 0;
    }
    return;
}