# Automatic sample rate switcher for Henrik Enquist's [CamillaDSP](https://github.com/HEnquist/camilladsp)
This tool provides two useful services:
1. **Automatic updating of the sample rate when that of the audio stream being captured changes.**

This is obtained by subscribing to Alsa events, reading the current sample rate when it changes, and updating CamillaDSP configuration. To this end, some commands of the [CamillaDSP websocket interface]( https://github.com/HEnquist/camilladsp/blob/master/websocket.md) are issued. The command `GetConfig` provides the current configuration; if the current configuration is not valid, the `GetPreviousConfig` command is issued. Then the _sample rate_ value is replaced with the value provided by the alsa control. The _chunk size_ value is as well updated as a function of the sample rate.  Finally, the updated configuration is flushed to the DSP with the command `SetConfig`.  
This part of the project is inspired by pavhofman's [gaudio_ctl](https://github.com/pavhofman/gaudio_ctl).

2. **Automatic reloading of a valid configuration whenever a USB DAC becomes available.**    

This is useful, for example, when switching the input of your DAC from USB to S/P-DIF. Whithout `camilladsp-setrate`, _CamillaDSP_ would hang up when switching back to USB. This result is obtained by performing the same process described above for sample rate. In this case, however, the process is initiated by a `SIGHUP` signal sent to the _camilladsp-setrate_ process by means of an `udev rule`.   

## Context
**_camilladsp-setrate_** is meant for use with a USB gadget capture device. I have tested it on my Raspberry Pi 4. I expect it may also work on other boards supporting USB gadget, such as Raspberry Pi Zero, Raspberry Pi 3A+, Raspberry Pi CM4 and BeagleBones, but I have no means of doing tests on such platforms.  
This project was developed on DietPi 64-bit. It should also work on other Debian-based Linux distribution and arguably on other Linux distributions as well.  
The software is coded in C language with use of the alsa and libwebsockets C library API's.
## Requirements
- A working gadget USB capture device
- Linux operating system
- C language development environment
- Alsa sound system
- Alsa C library
- Libwebsocket C library
## Installation
1. Install the development environment and the required libraries
```
sudo apt update  
sudo apt install build-essential libasound2-dev libwebsockets-dev
```
2. Build the executable file
```
make
```
3. Copy the executable file to `/usr/local/bin`
```
make install
```
4. Copy the file `camilladsp-setrate.service` to the system services folder and enable that service
```
sudo cp camilladsp-setrate.service /etc/systemd/system
sudo systemctl enable camilladsp-setrate
```
5. Edit the file `85-DAC.rules/etc/udev/rules.d` and replace the values of the parameters `ID_VENDOR_ID` and `ID_MODEL_ID` with those of your DAC. You can obtain those values with the following command (ID_VENDOR_ID=Vendor, ID_MODEL_ID=ProdID):
```
usb-devices
```
6. Copy the file `85-DAC.rules/etc/udev/rules.d` to the `udev` rules folder
```
sudo cp 85-DAC.rules /etc/udev/rules.d
```
7. Reboot the system
```
sudo reboot
```
## Running
**_camilladsp-setrate_** provides a few options to enable log cwmessages.  
The executable is launched as follows:  
```
camilladsp-setrate [OPTIONS]
```
OPTIONS:  
- `--err`       enables output of error log messages
- `--warn`      enables output of warning log messages
- `--user`      enables output of log messages concerning key events
- `--notice`    enables output of log messages useful to track what's happening inside the _camilladsp-setrate_ process
- `--timestamp` causes a timestamp to be prepended to each log message
- `--syslog`    redirects log messages to _syslog_ (otherwise messages are sent to standard error)
  
Usually the process starts as a service at boot. You can edit the file `camilladsp-setrate.service` to set the desired options.  
After modifications to the service file run the following commands:
```
sudo systemctl daemon-reload
sudo restart camilladsp-setrate
```
or reboot the system.
## Final notes
Comments in the source code will, hopefully, help to understand the what and the how.
  
