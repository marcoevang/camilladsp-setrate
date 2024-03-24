# **camilladsp-setrate  version 2.1.1**

## Automatic sample rate switcher for [CamillaDSP](https://github.com/HEnquist/camilladsp)

This tool provides two useful services:

1. **Automatic updating of the sample rate when that of the audio stream being captured changes.**

*CamillaDSP* works at a fixed sample rate. However, sample rate may change during a session, e.g. when listening to a playlist. In this case, if the sample rate from the audio source does not match the sample rate *CamillaDSP* is using for processing,  playback will not be correct.  

***camilladsp-setrate*** solves this problem by changing *CamillaDSP*'s configuration on-the-fly to match the sampling rate of the captured audio. This is obtained by subscribing to *alsa* events, reading the incoming sample rate when it changes, reading the current configuration of *CamillaDSP* and overwriting the *samplerate* and/or *capture_samplerate* parameters. To this end, some commands of the [CamillaDSP websocket interface]( https://github.com/HEnquist/camilladsp/blob/master/websocket.md) are issued. The command `GetConfig` provides the current configuration; if the current configuration is not valid, the `GetPreviousConfig` command is issued. Then the _samplerate_ and/or *capture_samplerate* values in the configuration are replaced with the correct ones. The _chunksize_ parameter value is as well updated as a function of the sample rate, calculating the value suggested in the section "Devices" of the [*CamillaDSP* home page](https://github.com/HEnquist/camilladsp).  Finally, the updated configuration is flushed to the DSP with the command `SetConfig.`

***camilladsp-setrate*** may also allow resampling to a fixed playback rate and upsampling by a fixed factor. Details are provided below in the "Running" section (see `--capture` flag and `--uppsampling` option).  

<ins>This feature has only been tested with USB gadget capture devices</ins>.

2. **Automatic reloading of a valid configuration whenever the playback device becomes available.**    

*CamillaDSP* stops working when the playback device is no longer available. This happens, for example, when your DAC is switched off or you switch to another input. Unfortunately, *CamillaDSP* remains blocked even when the playback device becomes available again. 

***camilladsp-setrate*** reloads a valid configuration as soon as the playback device reappears, thus unlocking _CamillaDSP_. This result is obtained by going through the same procedure described above for sample rate. In this case, however, the procedure is initiated by a signal sent by the operating system to the _**camilladsp-setrate**_ process when the playback device is detected. This is obtained by means of an `udev rule` (see the `88-DAC.rules` file).  

<ins>This feature has only been tested with USB playback devices</ins>.

## Context
I have tested **_camilladsp-setrate_**  on my Raspberry Pi 4 in gadget mode with its USB-C port configured for audio capture. I expect it may also work on other boards supporting USB gadget mode, such as Raspberry Pi Zero, Raspberry Pi 3A+, Raspberry Pi CM4 and BeagleBones.  Feel free to do your own experiments with other types of capture devices.

It should be noted that drivers for devices other than USB gadgets may not feature the alsa control that provides information on sample rate change.  
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

2. Clone the *git* repository and move to the home of the project:
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

- `-c, --capture`          Update *capture_samplerate* instead of *samplerate*
- `-t, --timestamp`      Causes a timestamp to be prepended to log messages
- `-s, --syslog`            Redirect log messages to _syslog_ (if this flag is omitted, messages are sent to standard error)
- `-v, --version`          Print software version
- `-h, --help`                Print help

OPTIONS:

- `-d, --device <capture device>`    Set alsa capture device [default: hw:UAC2Gadget]
- `-a, --address <address>       `                Set server IP address [default: localhost]
- `-p, --port <port>`                            Set server IP port [default: 1234]
- `-u, --upsampling <factor>`            Set upsampling factor [default: 1]
- `-l, --loglevel <log level>`          Set log level [values: err, warn, user, notice, off. Default: err]



All options require an argument. If an option is omitted, the default value is applied. 

Arguments must not be specified for flags.

The `--capture` flag and `--upsampling` options change the way ***camilladsp-setrate*** processes the *samplerate*, *chunksize* and *capture_samplerate* parameters, as follows:

- [ ] if `--capture` and `--upsampling` are both omitted, *samplerate* is set to that of the audio being captured and *chunksize* is updated as a function of *samplerate*. The *capture_samplerate* parameter is left unchanged.  

  <u>In this case resampling shall be disabled and *capture_samplerate* shall not be set in the configuration file</u>.

- [ ] if `--capture` is used, *capture_samplerate* is set to that of the audio being captured. The *samplerate* and *chunksize* parameters are left unchanged. This flag should be used to achieve resampling of the captured audio to the fixed playback rate set by the *samplerate* parameter in the configuration file.  <ins>It is up to the user to enable resampling in the configuration file</ins>.

- [ ] If `--upsampling` is used, *capture_samplerate* is set to that of the audio being captured and *samplerate* is set equal to the input rate multiplied by the specified upsampling factor (the latter must be positive). The *chunksize* parameter is as well updated as a function of *samplerate*. This option should be used to obtain in playback upsampled audio by a constant factor. For example, this flag can be used to instruct CamillaDSP to perform 2X or 4X oversampling regardless of the sample rate of the incoming audio. Make sure your playback device supports the resulting upsampled rate. 

  <ins>It is up to the user to enable resampling in the configuration file</ins>.

Note that `--capture` and `--upsampling` cannot be used at the same time.

If the `--device` option is used, the name of the capture device shall be given in the format required by the`arecord`command. 

The `--loglevel` option sets the following logging levels: 

- [ ] ​    `err`: only errors are logged
- [ ] ​    `warn`: errors and warnings are logged

- [ ] ​    `user`: errors, warnings and key events are logged, such as sample rate change
- [ ] ​    `notice`: errors, warnings, key events and debugging information are logged.

***camilladsp-setrate*** should start as a service at boot time. To this end, the `camilladsp-setrate.service` file is provided. You can edit that file to set the desired options.  
After modifications to the service file you have to make the `udev` daemon reload the rules:

```
sudo systemctl daemon-reload
sudo systemctl restart camilladsp-setrate
```
or reboot the system.

I strongly recommend not running ***camilladsp-setrate*** as *super-user*.

## Final notes
- This tool is useful if the audio player and *CamillaDSP* run on distinct computers. In case they run on the same computer, I recommend using the [alsa_cdsp](https://github.com/scripple/alsa_cdsp) plugin instead to get automatic sample rate switching.
- Devices other than USB gadgets may not provide the necessary information on sample rate change.
- Starting with version 2.0.0 the sample rate change process is driven by a finite-state machine whose diagram is provided under the *doc* folder.  You can find tons of information about this technique on the Internet, just search for "finite-state machine".  
- Starting with version 2.1.0 the flags `--err`, `--warn`, `--user` and `--notice` have been removed as unnecessary. The `--loglevel` option can be used instead.


- If your _CamillaDSP_ configuration is big, you may need to increase the size of the message buffer by updating the `BUFLEN` value in the `setrate.h` file.


- ***camilladsp-setrate***  works with all released versions of CamillaDSP.
- Comments in the source code will, hopefully, help to understand the what and the how.  

