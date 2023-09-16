# Automatic sample rate switcher for [CamillaDSP](https://github.com/HEnquist/camilladsp)
This tool provides two useful services:
1. **Automatic updating of the sample rate when that of the audio stream being captured changes.**

This is obtained by subscribing to Alsa events, reading the current sample rate when it changes, and updating CamillaDSP configuration accordingly. To this end, some commands of the [CamillaDSP websocket interface]( https://github.com/HEnquist/camilladsp/blob/master/websocket.md) are issued. The command `GetConfig` provides the current configuration; if the current configuration is not valid, the `GetPreviousConfig` command is issued. Then the _sample rate_ value is replaced with the value provided by the alsa control. The _chunk size_ value is as well updated as a function of the sample rate.  Finally, the updated configuration is flushed to the DSP with the command `SetConfig`.  

2. **Automatic reloading of a valid configuration whenever a USB DAC becomes available.**    

This is useful, for example, when you switch the input of your DAC from USB to non-USB. When this happens, _CamillaDSP_ locks out as the playback device is no longer valid, and the situation remains even after switching back to the USB input.  `camilladsp-setrate` reloads a valid configuration as soon as the USB DAC becomes available again, thus unlocking _CamillaDSP_. This result is obtained by performing the same process described above for sample rate. In this case, however, the process is initiated by a `SIGHUP` signal sent to the _camilladsp-setrate_ process by means of an `udev rule` (see the `88-DAC.rules` file).  
This feature is only applicable to USB DACs.

## Context
**_camilladsp-setrate_** is meant for use with a USB gadget capture device. I have tested it on my Raspberry Pi 4. I expect it may also work on other boards supporting USB gadget, such as Raspberry Pi Zero, Raspberry Pi 3A+, Raspberry Pi CM4 and BeagleBones, but I have no means of doing tests on such platforms.  
This project was developed on DietPi 64-bit. It should also work on other Debian-based Linux distributions and arguably on other Linux flavors as well.  
The software is coded in C language with use of the alsa and libwebsockets C library API's.
## Requirements
- A working gadget USB capture device
- Linux operating system
- C language development environment
- Alsa sound system
- Alsa C library
- Libwebsockets C library
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
5. Edit the file `85-DAC.rules` and replace the values of the parameters `ID_VENDOR_ID` and `ID_MODEL_ID` with those of your DAC. You can obtain those values with the following command :
```
usb-devices
```
(_Vendor_ corresponds to `ID_VENDOR_ID` and _ProdID_ corresponds to `ID_MODEL_ID`)  

6. Copy the file `85-DAC.rules` to the `udev` rules folder
```
sudo cp 85-DAC.rules /etc/udev/rules.d
```
7. Reboot the system
```
sudo shutdown -r now
```
## Running
**_camilladsp-setrate_** provides a few options to enable log messages.  
The executable is launched as follows:  
```
camilladsp-setrate [OPTIONS]
```
OPTIONS:  
- `--err`       enables error log messages
- `--warn`      enables warning log messages
- `--user`      enables log messages concerning key events
- `--notice`    enables log messages useful to track what's happening inside the _camilladsp-setrate_ process
- `--timestamp` causes a timestamp to be prepended to each log message
- `--syslog`    redirects log messages to _syslog_ (otherwise messages are sent to standard error)
  
Usually the process starts as a service at boot. You can edit the file `camilladsp-setrate.service` to set the desired options.  
After modifications to the service file you have to make the `udev` daemon reload the rules:
```
sudo systemctl daemon-reload
sudo systemctl restart camilladsp-setrate
```
or reboot the system.
## Final notes
Instructions for installing the packages are valid on debian-based Linux distributions. On other Linux flavors (e.g. Fedora) the package manager might differ, and the name of the libraries might also differ slightly.  
Comments in the source code will, hopefully, help to understand the what and the how.  
If your _CamillaDSP_ configuration is big, you may need to change the `BUFLEN` value in the file `setrate.h`.  
_camilladsp-setrate_ also works with alpha 2 release of CamillaDSP v2.
