# ELEC 327: MSP430G2253 Pulse Oximeter with LCD Display


## Materials and Schematics

- MSP430G2553 Luanchpad
- 2 4.7k ohm resistors
- 1 100k ohm resistors
- 1 breadboard
- 2 BC547 BJTs
- Many wires
- Nail clip
- Black tape
### Pulse Oximeter Schematic
![Pulse Oximeter Schematic](PulseOxSchem.png)

### Pulse Oximeter Picture
![Pulse Oximeter Pic](PulseOxPic.jpg)

## Code Architecture

### PWM
The Red and IR emitter LED flash on a 50% duty cycle such that when the Red LED is ON, the IR LED is OFF and vice versa. Timer A1 was utilized for the PWM with a clock speed of 32 kHz, and the LEDs were flashing every 3ms. The In order to accurately collect the sensor data from the photodiode, the readings were measured on the rising edge of the PWM signals so that values measured would indeed correspond to their respective LED. A Port 2 interrupt was utilized to send a flag that the next piece of data could start being collected as well as change the edge trigger of the interrupt (since it was being done of a single PWM signal where the negative edge of one signal is the positive edge of the other).

### Analog to Digital Conversion
Pin 1.1 on the MSP was used to do the analog to digital conversion. The A/D conversion would stall the code until it was completed and then add the value (usually between 400-800 out of 1023) to a variable which would then be averaged over 10 samples. This sampling process would happen 4 times before moving to calculations.

### Calculations
The average of the sampled data points would then be used to find the lowest and highest readings for both the red and the IR emitter diodes. The minimum and maximum values would be constantly be updated to form a value for the ratio R =  ( (REDmax-REDmin) / REDmin) / ( (IRmax-IRmin) / IRmin ). This value would then be used to calculate SpO2 = m * R + b. The values of m (slope) and b (y intercept) need to be calibrated to get accurate readings.

### Display (what I think it'll be)
The display follows the calculation of the SpO2 value. If the reading on the ADC or SpO2 is outlandish, the display will say that no finger is inserted. Once the readings are more reasonable, the display will say that the reading is being calculated. Finally, once the value has stabilized, the oxygen content of the blood will be displayed to the user.

