#include <Wire.h>
#include <SFE_MicroOLED_mod.h>
#include "./upwr.h"


MicroOLED oled(9, 0);                       // SparkFun OLED lib init

const int sample_count = 5;                 // more is smoother transistions but longer time

typedef struct looping {
    const long inv;                         // millisecond interval to poll
    volatile unsigned long now;             // current millis()
    volatile unsigned long past;            // previous millis()
} looping;

typedef struct adc {
    const char pin;                         // input pin number
    const float cal;                        // calibration - voltage divider ratio
    volatile float val;                     // value - average sample
    volatile int total;                     // running samples total storage
    volatile int samples[sample_count];     // samples array storage, length depends on sample_count
    looping poll;                           // interval & timer storage
} adc;

adc voltage = {2, 1.0, 0, 0, {}, {10, 0, 0}};
adc current = {3, 1.0, 0, 0, {}, {10, 0, 0}};
looping oled_refresh = {40, 0, 0};
looping adc_poll = {2, 0, 0};


void
setup(void) {
    pinMode(voltage.pin, INPUT);
    pinMode(current.pin, INPUT);
    oled.begin();
    oled.clear(ALL);
    oled.display();
    oled.setFontType(0);
    oled.setCursor(0, 0);
    Serial.begin(9600);
}

void
loop(void) {
    if (smart_delay(&current.poll) == 1) {
        read_adc(&voltage);
        read_adc(&current);
        output(&voltage);
        output(&current);
    }
    if (smart_delay(&oled_refresh) == 1) {
        oled.setCursor(0, 0);
        oled.print("Hello");
        oled.print(current.poll.now);
        oled.setCursor(1, 0);
        oled.print(current.val);
        oled.display();
    }
    Serial.println("Hello\n");
    delay(10);
}

// not patched for wrap around!
int
smart_delay(looping *ptr) {
    ptr->now = millis();
    if (ptr->now - ptr->past >= ptr->inv) {
        ptr->past = ptr->now;
        return 1;
    }
    return 0;
}

void
read_adc(adc *ptr) {
    for (int idx = 0; idx >= sample_count; idx++) {
        ptr->poll.now = millis();
        if (smart_delay(&ptr->poll) == 1) {
            // fifo: index 0 gets removed first when a new loop is called
            ptr->total -= ptr->samples[idx];
            ptr->samples[idx] = analogRead(ptr->pin);
            ptr->total += ptr->samples[idx];
            ptr->val = ptr->cal * (ptr->total / sample_count);
        }
        // index 4 is now the most recent sample, with 0 the oldest
    }
}

void
output(adc *ptr) {
    Serial.print(ptr->poll.now);
    Serial.print(",");
    Serial.print(ptr->val);
    Serial.print("\n");
}


/*
  Setting Up Calibration Values
  
  1) Analog to Digital Converter Values (ADC)
      Arduino ADC is 10-bit. 2^10 = 1024
      analogRead() returns 0-1023
      5V / 1024 = 0.004883
      adc_ratio ~= 4.883 mV per value (step) is the rule
  
  2) Voltage
      Voltage Divider Equation
          V_out = V_in * (R_2/(R_1 + R_2))
          
                               R₂
          V_out = V_in   ×  ―――――――――
                            (R₁ + R₂)
      Measure your own resistor values with a DMM and plug those into the equation
      Resistors
          R_1 = 14,750 Ω (ohm)
          R_2 =  4,675 Ω (ohm)
          R_1 + R_2 = 19,425
          V_ratio = (4,675 / 19,425) = 0.24067
      Voltages
          V_out = 12V * V_ratio
          V_out = 12,000mV * 0.24067
          V_out = 2888.04 mV is what this voltage divider provides for 12V input
      ADC
          analogRead() = V_out / adc_ratio
          analogRead() = 2888.04 / 4.883 ~= 591 is the raw value the code will present for 2.88V on V_out
          Measure input and output voltages with a DMM to verify
          cal = 0.004883/0.24067
          cal = 0.0202892  is the calibration multiplier for the ADC
      If you have an adjustable power supply, compare the ADC (reading * cal) with a DMM measuring V_in
         ADC         V_in
           5 * cal =  0.1 V
          50 * cal =  1.0 V
         167 * cal =  3.3 V
         247 * cal =  5.0 V
         296 * cal =  6.0 V
         444 * cal =  9.0 V
         592 * cal = 12.0 V
         690 * cal = 14.0 V
         740 * cal = 15.0 V
         937 * cal = 19.0 V
         986 * cal = 20.0 V
        1023 * cal = 20.7 V
                  
                   +---------+
                   |         |
                   |         \
                   |         / R_1
                   |         \  (14.7k)
          (12V) +  |         / 
          V_in  _______      |
                  ___        +---------------o Analog Read Pin
                -  |         |           ^
                   |         |           .
                   |         \           .
                   |         / R_2     V_out (2.88V)
                   |         \  (4.7k)   .
                   |         /           .
                   |         |           .
                   +----+----+ <.......... 
                        |
                       \ /  Ground
                        `
    3) Current
        With the adjustable ACS723, the Vref pot can be set very low to measure DC current with.
        If there is no current flow, Vref will output a fixed value. So 0 for DC is best.
        The gain pot depends on what mA range and what kind of precision in that range you want.
        For the full 5A ability you do not want much gain.
        As shown above, the ADC on the Arduino is 4.883 mV per value.
        Ratio 1.0 100mA/100mV, 100mA/(100mV/4.883) = 4.883 mA per ADC step
          4.883 * 1023 = 4.995 A max reading
        Ratio 0.4 100mA/250mV, 1.953 mA per ADC step
          1.953 * 1023 = 1.997 A max reading
        Ratio 0.2 100mA/500mV, 0.977 mA per ADC step
          0.977 * 1023 = 0.999 A max reading
        
        Current flow measurement is much wider and easier with Vref = 0 and sensitivity = 1.0:
        current = ((4.883 * adc_val) - Vref) * sensitivity
        current = (4.883 * adc_val)
        
        Calibration will require known resistor values or a fixed current power supply.     
*/
