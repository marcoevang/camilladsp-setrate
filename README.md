# **camilladsp-setrate  version 2.0.0**

## Automatic sample rate switcher for [CamillaDSP](https://github.com/HEnquist/camilladsp)

This tool provides two useful services:

1. **Automatic updating of the sample rate when that of the audio stream being captured changes.**

*CamillaDSP* works at the fixed sample rate set in its configuration file. If the sample rate of the captured audio is different, and resampling is not enabled,  playback will not be correct. ***camilladsp-setrate*** solves this problem by changing the configuration of CamillaDSP to match the sampling rate of the captured audio. This is obtained by subscribing to *alsa* events, reading the current sample rate when it changes, reading the current configuration of *CamillaDSP* and overwriting the *samplerate* parameter. To this end, some commands of the [CamillaDSP websocket interface]( https://github.com/HEnquist/camilladsp/blob/master/websocket.md) are issued. The command `GetConfig` provides the current configuration; if the current configuration is not valid, the `GetPreviousConfig` command is issued. Then the _samplerate_ parameter value in the configuration is replaced with the value provided by the *alsa* sound system. The _chunksize_ parameter value is as well updated as a function of the sample rate.  Finally, the updated configuration is flushed to the DSP with the command `SetConfig.`It is also possible to have the value of the *capture_samplerate* parameter updated instead; in this case the *chunksize* parameter is left unchanged (see the information below about the *--capture* command flag) .  
<ins>This feature has only been tested with USB gadget capture devices</ins>.

2. **Automatic reloading of a valid configuration whenever the playback device becomes available.**    

*CamillaDSP* stops working when the playback device is no longer available. This happens, for example, when your DAC is switched off or the user switches to another input. Unfortunately, *CamillaDSP* remains blocked even when the playback device becomes available again. ***camilladsp-setrate*** reloads a valid configuration as soon as the playback device reappears, thus unlocking _CamillaDSP_. This result is obtained by going through the same procedure described above for sample rate. In this case, however, the procedure is initiated by a signal sent by the operating system to the _**camilladsp-setrate**_ process when the playback device is detected. This is obtained by means of an `udev rule` (see the `88-DAC.rules` file).  
<ins>This feature has only been tested with USB playback devices</ins>.

## Context
I have tested **_camilladsp-setrate_**  on my Raspberry Pi 4 in gadget mode with its USB-C port configured for audio capture. I expect it may also work on other boards supporting USB gadget mode, such as Raspberry Pi Zero, Raspberry Pi 3A+, Raspberry Pi CM4 and BeagleBones. Feel free to experiment with other types of capture devices as well.  
This project was developed on DietPi 64-bit. It should also work on other Debian-based Linux distributions and arguably on other Linux flavors as well.   The software is coded in C language with use of the *alsa* and *libwebsockets* C API's.

## Requirements
- Linux operating system
- C language development environment
- Alsa sound system
- Alsa C library
- Libwebsockets C library
- [*CamillaDSP*](https://github.com/HEnquist/camilladsp) up and running

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
Instructions for installing the required packages are valid on debian-based Linux distributions. On other Linux flavors (e.g. Fedora) the package manager might differ, and the name of the libraries might also differ slightly.   

If you use this tool on a Raspberry Pi 4 64-bit OS, you can probably directly use the ***camilladsp-setrate*** executable file provided under the *bin* folder of this repository. In that case, skip steps 1. and 3., complete the Install phase and then check if the provided executable file works.  

## Installing

1. Copy the executable file to `/usr/local/bin`:

```
make install
```
If the executable file is already running, stop it before issuing *make install*.

2. Edit the file `camilladsp-setrate.service` and replace the values of the parameters _User_ and _Group_ with yours. You might want to make other changes, e.g. the options on the command line of the _ExecStart_ parameter (see below for a description of command options and flags).

3. Copy the file `camilladsp-setrate.service` to the system services folder and enable that service:

```
sudo cp camilladsp-setrate.service /etc/systemd/system
sudo systemctl enable camilladsp-setrate
```
4. Make sure the user that runs the service (e.g. dietpi in the example .service file) is a member of the *audio* group, otherwise the started process cannot access the audio devices. If it is not, add it to the *audio* group:

```
sudo usermod -a <user running the camilladsp-setrate service> -G audio
```

   Example:

```
sudo usermod -a dietpi -G audio
```


5. Edit the file `85-DAC.rules` and replace the values of the parameters `ID_VENDOR_ID` and `ID_MODEL_ID` with those of your USB DAC.
   You can obtain those values with the following command :

```
usb-devices
```
(_Vendor_ corresponds to `ID_VENDOR_ID` and _ProdID_ corresponds to `ID_MODEL_ID`)  

6. Copy the file `85-DAC.rules` to the `udev` rules folder:

```
sudo cp 85-DAC.rules /etc/udev/rules.d
```
7. Reboot the system:

```
sudo shutdown -r now
```
## Running
USAGE: 

```
  camilladsp-setrate [FLAGS] [OPTIONS]
```
FLAGS:

- `-c, --capture`         Update *capture_samplerate* instead of *samplerate*
- `-e,--err`                    Enable logging of errors
- `-w, --warn`                Enable logging of warnings
- `-u, --user`                Enable logging of key events
- `-n, --notice`            Enable logging of internal events
- `-t, --timestamp`      Causes a timestamp to be prepended to log messages
- `-s, --syslog`            Redirect log messages to _syslog_ (if this option is omitted, messages are sent to standard error)
- `-v, --version`          Print software version
- `-h, --help`                Print help

OPTIONS:

- `-d, --device <capture device>`   Set alsa capture device [default: hw:UAC2Gadget]
- `-a, --address <address>       `                Set server IP address [default: localhost]
- `-p, --port <port>`                             Set server IP port [default: 1234]
- `-l, --loglevel <log level>`          Set log level - Override flags [values: err, warn, user, notice]

If the *--capture* flag is omitted, this tool updates the *samplerate* and *chunksize* parameters of *CamillaDSP* configuration. If that flag is used, the *capture_samplerate* parameter is updated instead and *chunksize* is left unchanged. This flag should be used if *CamillaDSP* is configured for resampling the captured audio to a fixed playback rate.  

If the `--device` option is used, the name of the capture device shall be given in the format required by the`arecord`command. 

The `--err`, `--warn`, `--user` and `--notice` flags independently enable each type of log message. The `--loglevel` option overrides the settings of those flags, therefore if you use this option, you should not use the flags.

***camilladsp-setrate*** should start as a service at boot time. To this end, the `camilladsp-setrate.service` file is provided. You can edit that file to set the desired options.  
After modifications to the service file you have to make the `udev` daemon reload the rules:

```
sudo systemctl daemon-reload
sudo systemctl restart camilladsp-setrate
```
or reboot the system.

I strongly recommend not running ***camilladsp-setrate*** as *super-user*.

## Final notes
- This tool is useful if the audio player and *CamillaDSP* run on distinct computers. In case they run on the same computer, I recommend using the [alsa_cdsp](https://github.com/scripple/alsa_cdsp) plugin instead to get automatic samplerate switching (follow the link to the github page).
- In this version the sample rate change process is driven by a finite-state machine whose diagram is provided under the *doc* folder.  You can find tons of information about this technique on the Internet, just search for "finite-state machine".  


- If your _CamillaDSP_ configuration is big, you may need to increase the size of the message buffer by updating the `BUFLEN` value in the `setrate.h` file.


- ***camilladsp-setrate***  works with all released versions of CamillaDSP.
- If the `--capture` flag is omitted, ***camilladsp-setrate*** dynamically replaces the value of the *samplerate* parameter set in the configuration file. If the value of the *capture_samplerate* parameter is also assigned in that file, if that value is different from the value of the audio stream being captured, and if resampling is not enabled, *CamillaDSP* detects a contradiction and reports a configuration error. Therefore, the value of the *capture_samplerate* parameter should not be set.
  If, on the other hand, the `--capture` flag is used, ***camilladsp-setrate*** dynamically replaces the value of the *capture_samplerate* parameter, leaving the *samplerate* parameter unaffected. This is useful to achieve resampling at a fixed rate determined by the *samplerate* parameter. In this case, both the *capture_samplerate* and *samplerate* parameters must be specified and resampling must be enabled.
- Comments in the source code will, hopefully, help to understand the what and the how.  

