# TEL320
For sharing work in TEL320, group 0

This repository includes:
1. One python file called "live_plotting.py". It handles the live plotting of the data. It plots the intenisty of the radar dat live, as well as the water level and flow rate over time.
2. Two folders:
   - "Src" which contains the sourcecode for the firmware that is loaded onto the STM32 MCU.
   - "Inc" contains the header files needed for the firmware to work. The params.h file has all
     parameters that can be manipulated to alter the signal processing and to adapt the
     measurements to the physical setup.

Setup
To make this code work, it is necessary to set up the stm32 controller board correctly according to the instructions from acconeer. When the stm32 project is configured correctly, these files can be included in the project.
