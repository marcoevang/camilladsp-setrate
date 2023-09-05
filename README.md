# camilladsp-setrate
Automatic sample rate switcher for CamillaDSP


This tool performs two useful tasks:
1. **_camilladsp-setrate_ automatically updates the sample rate of CamillaDSP when the sample rate of the audio stream being captured changes.**

This is obtained by subscribing to Alsa events, reading the current sample rate when it changes, and updating CamillaDSP configuration. To this end, some commands of the [CamillaDSP websocket interface]( https://github.com/HEnquist/camilladsp/blob/master/websocket.md) are issued. The command `GetConfig` provides the current configuration; if the current configuration is not valid, the `GetPreviousConfig` command is issued. Finally, the updated configuration is flushed to the DSP with the command `SetConfig` .

This part of the project is inspired by `pavhofman`'s [gaudio_ctl](https://github.com/pavhofman/gaudio_ctl)

2. **_camilladsp-setrate_ forces reload of a valid configuration whenever a USB DAC becomes available.**

This is useful, for example, when switching the input of your DAC from USB to S/P-DIF; whithout `camilladsp-setrate`, when switching back to USB, CamillaDSP would hang up due to an invalid configuration. This result is obtained by performing the same process described above for sample rate. but this time when a signal is sent to the _camilladsp-setrate_ process by means of an `udev rule`.  
This feature may be enabled with a command line option (see _Running_ section).

This part of the project is inspired by `pohofman`'s [gaudio_ctl](https://github.com/pavhofman/gaudio_ctl).

# Context
**_camilladsp-setrate_** is meant for use on a USB gadget capture device. I have tested it on my Raspberry Pi 4. I expect it may also work on other boards supporting USB gadget, such as Raspberry Pi Zero, Raspberry PI 3A, Raspberry CM4 and BeagleBones, but I have no means of doing tests on such platforms.
This project was developed on DietPi 64-bit. It should also work on other Debian-based Linux distribution and arguably on other Linux distributions as well.
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
# Running
**_camilladsp-setrate_** starts as a service at boot. The executable file provides a few options to log events and to enable the usb monitoring:
- _--err_ enables outout of error log messages
- _--warn_ enables output of warning log messages
- _--user_ enables output of log messages on key events
- _notice_ enables output of log messages useful to track what's happening inside the camilladsp-setrate process
- _--timestamp_ causes a timestamp to be prepended to each log message
- _--syslog_ redirects log messages to _syslog_ (otherwise messages are sent to standard error)
- _--usbmon_ enables catching of signals that notify availabilty of the USB DAC
# Final notes
Comments in the source code will, hopefully, help to understand the what and the how. Users are encouraged to improve the code and to add features.
  
