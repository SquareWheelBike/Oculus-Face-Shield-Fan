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

// pwm memory
unsigned int PWM_freq = 25000;   // 25kHz
unsigned int TMR2PRESCALE; // need to set in PWM_Initialize()

PWM_Initialize()
{
    switch(T2CON & 3){
        case 0:
            TMR2PRESCALE = 1;
            break;
        case 1:
            TMR2PRESCALE = 4;
            break;
        default:
            TMR2PRESCALE = 16;
    }
  PR2 = (_XTAL_FREQ/(PWM_freq*4*TMR2PRESCALE)) - 1; //Setting the PR2 formulae using datasheet // Makes the PWM work in 5KHZ
    CCP1M3 = 1; CCP1M2 = 1;  //Configure the CCP1 module 
    T2CKPS0 = 1;T2CKPS1 = 0; TMR2ON = 1; //Configure the Timer module
    TRISC2 = 0; // make port pin on C as output
}

PWM_Duty(unsigned int duty)
{
  if(duty<1024)
  {
    duty = ((float)duty/1023)*(_XTAL_FREQ/(PWM_freq*TMR2PRESCALE)); // On reducing //duty = (((float)duty/1023)*(1/PWM_freq)) / ((1/_XTAL_FREQ) * TMR2PRESCALE);
    CCP1X = duty & 1; //Store the 1st bit
    CCP1Y = duty & 2; //Store the 0th bit
    CCPR1L = duty>>2;// Store the remaining 8 bit
  }
}


void main(void) {
    // FOR PC FAN 25khz, duty cycle controlled, 20% minimum before movement
    // TODO: figure out how to get processor cycle count
    
    // set latch states
    LED_TRIS = 0;   // set LED latch as output
    Motor_TRIS = 0; // set motor PWM to output
    Switch_TRIS = 1;    // set switch latch as input
    
    // set initial output states
    LED = 1;
    Motor = 0;
    
    int mode = 2; // changing this changes the initial mode
    int numBlinks = 0;  // blink counter
    int donePulsing = 0;    // flag for blink pattern
    unsigned long stopper = ULONG_MAX / 2 - 1;  // cursed way to stop timer overflow
    
    PWM_Initialize();
    PWM_Duty((1023 / 5) * mode); // start at 40% (mode 2)
    
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
            PWM_Duty(mode * (1020/5));  // set pwm for next mode
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
        /*
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
         */
        __delay_ms(1);
        milliseconds++; // increment millis() return. Cursed, I know, but does not need to be accurate
        if (milliseconds == stopper) // make sure it doesnt hit an overflow
            milliseconds = 0;
    }
    return;
}