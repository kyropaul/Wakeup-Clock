# Wakeup-Clock
ESP8266 Powered Wake Up Clock for Children
## Overview
My kids wakeup clock broke (started turning on lights at wrong time), so instead of buying a new one I made one. Because I didn't want to keep printed new cases for the clock (as their tastes changed) I decided to incorporate lego into the base so they can decorate as they like. The basic operation can be seen in the gif, but here is an overview.
* Web interface for setup of times (so I don't have to change code)
* Get time from the internet so on a power failure it will be correct. The system also updates the time nightly to capture daylight savings
* "night time" light turns on for 1 hour starting at when I want my kids to go to sleep
* "night time" light also turns back on 1 hour before they can wake up
* "soft-wake" light comes on when they can get up but should be quiet and not wake me up
* "wake up" light will come on when it's ok to wake me up
* Working on a newer version that uses (https://github.com/tzapu/WiFiManager) to allow a user to configure the wifi instead of hardcoded, but that's not completed as it doesn't work with the async sever
## Required ESP Libraries
Download and install these into your arduino software
https://github.com/me-no-dev/ESPAsyncWebServer

https://github.com/me-no-dev/ESPAsyncWebServer
## Hardware
* ESP8266 NODEMCU board (I went with these but any should work https://www.amazon.ca/gp/product/B08C52BCZQ/ref=ppx_yo_dt_b_asin_title_o01_s00?ie=UTF8&psc=1)
* 3x LED (I used a green, yellow, and blue)
* 3x resistors (I used a 220ohm but you should be fine with a range of values)
* 6x10 lego board
## 3D Printed Parts
* See stl directory for file list
## Basic Process
After start up the ESP loops every 10 seconds and goes through the following steps
1. Read a couple files from spiffs containing the variables
2. If the time is 2:24am refresh the time clock (this sets a temporary variable that is reset after 2:24 so it only updates once)
3. Get time using <time.h> and convert the current time to minutes past midnight
4. If statement to determine if it is between one of the important time ranges
5. Turn on or off lights as appropriate

The Web portal is an asynchronous website that <ESPAsyncTCP.h> and <ESPAsyncWebServer.h>. When you hit it you can save a new value.
## Projects used to create this
* Turning LED on and OFF by time of day (https://github.com/jumejume1/NodeMCU_ESP8266/blob/master/TURNON_OFF_WITH_TIME/TURNON_OFF_WITH_TIME.ino)
* Automatically getting daylight savings (https://werner.rothschopf.net/202011_arduino_esp8266_ntp_en.htm)
* Web server to update values (https://randomnerdtutorials.com/esp32-esp8266-input-data-html-form/)
* STL used for sun (https://www.thingiverse.com/thing:285154/files) 
