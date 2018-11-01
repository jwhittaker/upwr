# upwr
Micro power meter for 0-20V, 0-5A DC monitoring built around Mega32U4, ACS723, OLED, and serial.

This project is suitable for measuring small DC device power draw, such as a laptop or embedded system.

Voltage is measured with a voltage divider using E48 resistors 14.7k and 4.64k, putting 12V around 2.8V for near the center of the 0-5V 10-bit ADC range of the Mega32U4.

Current is measured with the hall effect using the ACS723 built into a kit by SparkFun. It allows for adjustable Vref and Gain for controlling precision around certain ranges or measuring AC current. Leaving those alone gives a wider 0-5 A current range.

Sensor readings are averaged through five samples in the default code.

Values are displayed on a 128x32 pixel matrix SSD1306 OLED on the i2c bus.

Values can be logged externally using the USB serial port. Code the for the host end (perhaps a RPi Zero) is a WIP.
