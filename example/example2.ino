#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged in4to digital pin 2 on the Arduino
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);

const int buttonPin = 2;
const byte OC1A_PIN = 9;
const byte OC1B_PIN = 10;
const word PWM_FREQ_HZ = 25000; //Adjust this value to adjust the frequency
const word TCNT1_TOP = 16000000 / (2 * PWM_FREQ_HZ);
const int SETTEMPINTERVAL = 30000;

int buttonState = 0;
void setup()
{
    pinMode(buttonPin, INPUT);
    pinMode(OC1B_PIN, OUTPUT);

    // PWM settings
    // Clear Timer1 control and count registers
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    // Set Timer1 configuration
    // COM1A(1:0) = 0b10   (Output A clear rising/set falling)
    // COM1B(1:0) = 0b00   (Output B normal operation)
    // WGM(13:10) = 0b1010 (Phase correct PWM)
    // ICNC1      = 0b0    (Input capture noise canceler disabled)
    // ICES1      = 0b0    (Input capture edge select disabled)
    // CS(12:10)  = 0b001  (Input clock select = clock/1)

    TCCR1A |= (1 << COM1A1) | (1 << WGM11);
    TCCR1B |= (1 << WGM13) | (1 << CS10);
    ICR1 = TCNT1_TOP;

    // start serial port
    Serial.begin(9600);
    // Start up the temperature library
    sensors.begin();
}

void loop()
{
    tempToPwmDuty();
    delay(SETTEMPINTERVAL);
}

void setPwmDuty(byte duty)
{
    OCR1A = (word)(duty * TCNT1_TOP) / 100;
    Serial.println("Fan duty cycle");
    Serial.println(duty);
}

void tempToPwmDuty()
{
    // Override temp sensor with button
    buttonState = digitalRead(buttonPin);
    if (buttonState == HIGH)
    {
        Serial.println("Button ON, overriding temp sensor");
        setPwmDuty(100);
        return;
    }

    sensors.requestTemperatures();
    int temp = sensors.getTempCByIndex(0);
    Serial.println("Setting temp via sensor");
    Serial.println(temp);
    switch (temp)
    {
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
        setPwmDuty(25);
        break;
    case 26:
        setPwmDuty(50);
        break;
    case 27:
        setPwmDuty(75);
    case 28:
        setPwmDuty(100);
        break;
    default:
        setPwmDuty(100);
    }
}
