# Oculus-Face-Shield-Fan

Updated to use an Arduino Nano.

Use .ino file in src/, examples are other projects I found that use arduino timers for PWM.
Arduino version uses a 25kHz PWM, stepped 20% at a time. Default is 40%.
Controls a noctua fan, PWM out is on pin 9.
Pin 3 is a pullup input for a push button. Input is debounced.