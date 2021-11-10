
// generate a 25kHz PWM
// duty cycle moves 20% at a time

// button pin is 3, gnd = press
// fan pwm pin is 9

// SUPPORTING FUNCTIONS

int oneShotBits[8]; // oneshot bits available for use for oneshot or toggle calls
int ONSTracker;     // index for oneshot bits

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
    else
        return 0;
}

// timer memories, used for debounce
int timerInSession[8];                                           // for speed, so we only update timer timers when needed
int timerMemory[sizeof(timerInSession) / sizeof(int)];           // make function calls smaller by remembering previous output state
unsigned long timerTimers[sizeof(timerInSession) / sizeof(int)]; // debounce timers available for use
int timerTracker;

int TON(int input, int preset, int timerNumber)
{
    if (input && !timerInSession[timerNumber])
        timerTimers[timerNumber] = millis();
    else if (input && timerMemory[timerNumber])
        return 1;
    else if (input && millis() - timerTimers[timerNumber] >= preset)
    {
        timerMemory[timerNumber] = 1;
        return 1;
    }
    timerMemory[timerNumber] = 0;
    timerInSession[timerNumber] = input;
    return 0;
}

// BEGINNING OF PROGRAM -----------------------------

const word PWM_FREQ_HZ = 25000; //Adjust this value to adjust the freque
const word TCNT1_TOP = 16000000 / (2 * PWM_FREQ_HZ); // 16MHz / (2 * PWM_FREQ_HZ)
const byte OC1A_PIN = 9;
const byte OC1B_PIN = 10;

//configure Timer 1 (pins 9,10) to output 25kHz PWM
void setupTimer1(){
    //Set PWM frequency to about 25khz on pins 9,10 (timer 1 mode 10, no prescale, count to 320)
    pinMode(OC1A_PIN, OUTPUT);
    pinMode(OC1B_PIN, OUTPUT);
    TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);
    TCCR1B = (1 << CS10) | (1 << WGM13);
    ICR1 = TCNT1_TOP;
    OCR1A = 0;
    OCR1B = 0;
}

void setPwmDuty(byte duty)
{
    OCR1A = (word)(duty * TCNT1_TOP) / 100;
}

// do some defines to port pic code over to arduino
uint8_t switchPin = 3;
#define Switch digitalRead(switchPin)

int mode = 2; // start in mode 2
int numBlinks = 0;
int donePulsing = 0;
int LED = 0;

void setup()
{
    Serial.begin(9600);

    // set up pin modes
    pinMode(LED_BUILTIN, OUTPUT);       // mode pin for LED
    pinMode(switchPin, INPUT_PULLUP);   // switch input for mode change
    digitalWrite(LED_BUILTIN, !LED);    // set led to off

    // set these two pins to inputs on pwr and gnd
    // they are hard wired to 5v and gnd, I just dont want them as floating outputs or inputs
    pinMode(6, INPUT);  // gnd pin
    pinMode(7, INPUT);  // pwr pin
    pinMode(8, INPUT);  // rpm pin, unused but could be implemented later

    setupTimer1(); // need to set up timer 1 for PWM
    setPwmDuty(mode * 20); // set duty cycle to 0
    Serial.println("Duty cycle set to " + String(mode * 20));

    Serial.println("Done setup.");
}

void loop()
{
    // start of loop instructions
    ONSTracker = 0; // reset tracker for each loop
    timerTracker = 0;

    // Main code here

    // mode change condition, oneShot pb press with 20ms debounce
    if (oneShot(TON(!Switch, 20, timerTracker++), ONSTracker++))
    {
        mode++; // increment to next mode
        if (mode > 5)
            mode = 0;
        int duty = mode * 20;
        Serial.println("Duty cycle set to " + String(duty));
        setPwmDuty(duty); // update pwm duty cycle based on mode
    }

    // indicate mode on LED
    if (oneShot(millis() % 200 > 100, ONSTracker++) && !donePulsing) // 5 pulses per second, for a 2.5Hz LED pulse
    {
        if (!LED) // if led is on, then turn off, increment pulse counter, and check for done pulsing
        {
            LED = 1; // turn off LED
            digitalWrite(LED_BUILTIN, !LED);
            numBlinks++;
            if (numBlinks >= mode)
            {
                numBlinks = 0;
                donePulsing = 1;
            }
        }
        else if (mode) // if LED is already off, then turn it on
        {
            LED = 0;
            digitalWrite(LED_BUILTIN, !LED);
        }
    }
    if (TON(donePulsing, 1000, timerTracker++))
    {
        donePulsing = 0;
    }

    delay(1); // slow down the processor a bit
}
