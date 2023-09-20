# **camilladsp-setrate  version 1.0.0**

## Automatic sample rate switcher for [CamillaDSP](https://github.com/HEnquist/camilladsp)

This tool provides two useful services:

1. **Automatic updating of the sample rate when that of the audio stream being captured changes.**

This is obtained by subscribing to *Alsa* events, reading the current sample rate when it changes, and updating *CamillaDSP* configuration accordingly. To this end, some commands of the [CamillaDSP websocket interface]( https://github.com/HEnquist/camilladsp/blob/master/websocket.md) are issued. The command `GetConfig` provides the current configuration; if the current configuration is not valid, the `GetPreviousConfig` command is issued. Then the _sample rate_ value is replaced with the value provided by the *alsa control*. The _chunk size_ value is as well updated as a function of the sample rate.  Finally, the updated configuration is flushed to the DSP with the command `SetConfig`.  
<u>This feature has only been tested with USB gadget capture devices</u>.

2. **Automatic reloading of a valid configuration whenever the playback device becomes available.**    

This is useful, for example, when your DAC goes off or when unplugging or switching off the input  of your DAC corresponding to the playback device configured in *CamillaDSP*. When this happens, _CamillaDSP_ locks out as the playback device is no longer valid, and the situation remains even after the DAC comes back on or a valid input is switched back on.  `camilladsp-setrate` reloads a valid configuration as soon as the playback device becomes available again, thus unlocking _CamillaDSP_. This result is obtained by going through the same procedure described above for sample rate. In this case, however, the procedure is initiated by a `SIGHUP` signal sent to the _camilladsp-setrate_ process by means of an `udev rule` (see the `88-DAC.rules` file).  
<u>This feature has only been tested with USB playback devices</u>.

## Context
I have tested **_camilladsp-setrate_**  on my Raspberry Pi 4 in gadget mode with its USB-C port configured for audio capture. I expect it may also work on other boards supporting USB gadget mode, such as Raspberry Pi Zero, Raspberry Pi 3A+, Raspberry Pi CM4 and BeagleBones. Feel free to experiment with other types of capture devices as well.
This project was developed on DietPi 64-bit. It should also work on other Debian-based Linux distributions and arguably on other Linux flavors as well. 
The software is coded in C language with use of the *alsa* and *libwebsockets* C API's.

## Requirements
- Linux operating system
- C language development environment
- Alsa sound system
- Alsa C library
- Libwebsockets C library
- [CamillaDSP](https://github.com/HEnquist/camilladsp) up and running

## Building

1. Install *git*, the C development environment and the required libraries:
```
sudo apt update  
sudo apt install git build-essential libasound2-dev libwebsockets-dev
```
2. Clone the git repository and move to the home of the project:
```
git clone https://github.com/marcoevang/camilladsp-setrate
cd camilladsp-setrate
```
3. Build the executable file:

```
make
```
## Installing

1. Copy the executable file to `/usr/local/bin`

```
make install
```
If the executable file is already running, stop it before issuing *make install*.

2. Edit the file `camilladsp-setrate.service` and replace the values of the parameters _User_ and _Group_ with yours. You might also want to change the options on the command line of the _ExecStart_ parameter (see below for a description of options).

3. Copy the file `camilladsp-setrate.service` to the system services folder and enable that service

```
sudo cp camilladsp-setrate.service /etc/systemd/system
sudo systemctl enable camilladsp-setrate
```
7. Edit the file `85-DAC.rules` and replace the values of the parameters `ID_VENDOR_ID` and `ID_MODEL_ID` with those of your USB DAC.
You can obtain those values with the following command :
```
usb-devices
```
(_Vendor_ corresponds to `ID_VENDOR_ID` and _ProdID_ corresponds to `ID_MODEL_ID`)  

8. Copy the file `85-DAC.rules` to the `udev` rules folder
```
sudo cp 85-DAC.rules /etc/udev/rules.d
```
9. Reboot the system
```
sudo shutdown -r now
```
## Running
USAGE: 

```
  camilladsp-setrate [FLAGS] [OPTIONS]
```
FLAGS:

- `--err`                Enable logging of errors
- `--warn`              Enable logging of warnings
- `--user`              Enable logging of key events
- `--notice`          Enable logging of internal events
- `--timestamp`    Causes a timestamp to be prepended to log messages
- `--syslog`          redirect log messages to _syslog_ (if this option is omitted, messages are sent to standard error)
- `--version`        Print software version
- `--help`              Print help

OPTIONS:

- `--device <capture device>`   Set alsa capture device [default: hw:UAC2Gadget]
- `--server <address>:<port>`   Set server IP address and port [default: localhost:1234]
- `--loglevel <log level>`         Set log level - Override flags [values: err, warn, user, notice]

***camilladsp-setrate*** should start as a service at boot time. You can edit the file `camilladsp-setrate.service` to set the desired options.  
After modifications to the service file you have to make the `udev` daemon reload the rules:

```
sudo systemctl daemon-reload
sudo systemctl restart camilladsp-setrate
```
or reboot the system.

I strongly recommend not running ***camilladsp-setrate*** as *super-user*.

## Final notes
Instructions for installing the packages are valid on debian-based Linux distributions. On other Linux flavors (e.g. Fedora) the package manager might differ, and the name of the libraries might also differ slightly.  
The name of the capture device is given in the format required by `arecord`. If the `--device` option is omitted, the name is set to "hw:UAC2Gadget".
The IP address and port of CamillaDSP websocket server are given in the format <address>:<port>. If port only is given (colon required), address is set to "localhost". If address only is given (colon may be omitted), port is set to 1234. If the `--server` option is omitted, address and port are set to "localhost:1234"
If your _CamillaDSP_ configuration is big, you may need to change the `BUFLEN` value in the file `setrate.h`.  
Comments in the source code will, hopefully, help to understand the what and the how.  
_camilladsp-setrate_ also works with alpha releases of CamillaDSP v2.
