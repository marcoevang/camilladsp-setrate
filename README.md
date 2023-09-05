# camilladsp-setrate
Automatic sample rate switcher for CamillaDSP

**_camilladsp-setrate_** performs two useful actions:
1. **It automatically updates the sample rate of CamillaDSP when the sample rate of the audio stream being captured changes.**

This is obtained by subscribing to Alsa events, reading the current sample rate when it changes, and updating CamillaDSP configuration. To this end, some commands of the [websocket interface]( https://github.com/HEnquist/camilladsp/blob/master/websocket.md) are issued via websocket. The command `GetConfig` provides the current configuration; if the current configuration is not valid, the `GetPreviousConfig` command is issued. Finally, the updated configuration is flushed to the DSP with the command `SetConfig` .

This part of the project is inspired by `pavhofman`'s [gaudio_ctl](https://github.com/pavhofman/gaudio_ctl)

2. **It forces reload of a valid configuration whenever a USB DAC becomes available.**

This is useful, for example, when switching the input of your DAC from S/P-DIF to USB (whithout camilladsp-setrate, CamillaDSP would hang with an invalid configuration).
It is meant for use on a USB gadget capture device. I have tested it on my Raspberry Pi 4. It expect it to also work on other boards supporting USB gadget, such as Raspberry Pi Zero, Raspberry PI 3A. Raspberry CM4 and BeagleBones, but I have no means of doing tests on such platforms.
This project was developed on DietPi 64-bit, but it should work on other Debian-based Linux distribution and, probably, on other Linux distributions.
The software is coded in C language with use of the asound and libwebsockets C library API's.
## Requirements
- Linux operating system
- C language development environment
- Asound C library
- Libwebsocket C library
## Installation
1. Install the development environment and the required libraries
> sudo apt update

> sudo apt install build-essential libasound2-dev libwebsockets
2. Build the executable file
>make
3. Copy the executable file to `/usr/local/bin`
> make install
4. Copy the file `camilladsp-setrate.service` to `/etc/systemd/system` and enable the service
> sudo cp camilladsp-setrate.service /etc/systemd/system
> 
> sudo systemctl enable camilladsp-setrate
> 5. Reboot the system
> sudo reboot
