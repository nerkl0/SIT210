# Task 1.1P: Lights On
### Description
---
A multi-light control program designed to control power via a switch or with the in-built automated time-based shut off feature. 

### Installation
---
1. Download [Arduino IDE](https://www.arduino.cc/en/software/).
2. Set up Arduino board as below
3. Upload sketch from this repository to the board*

*Works best with Arduino Uno or Nano boards*

### Diagram
---
![Schematic](/images/Schematic.png "Lights On Schematic")

### Usage
---
1. Switch on to toggle power to the lights
2. LED on pin 4 turns off after 30 seconds
3. LED on pin 8 turns off after 60 seconds
4. Switch off at any time to turn both LEDs off immediately

### Technical Notes 
---
   **loop()**

    When the switch is read as active and the current program state is off, the program enters the first `if` block turning the LEDs on. 
    If the inverse condition is met, the lights are turned off. 
    If the current state is already on, the program instead updates the timers on each LED.

**handleSwitch()**
   
    Takes a boolean argument that triggers the LED power, it then alters the current state of the program to match the argument.

**handleTimers()**

    Handles the automated power off feature.
    Increments the Timer.current struct member; if it exceeds the LED's maximum time allowance, it will trigger the LED to switch off.


### Demonstration
---
[View demo](https://drive.google.com/file/d/1P_KpiVqdhOLfLtwSgYDcmYbPXJCx8dED)

** **Note**: The demonstration example uses 3 and 6 seconds as timing parameters. 