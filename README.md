# camilladsp-setrate
Automatic sample rate switcher for CamillaDSP

camilladsp-setrate automatically updates the sample rate of CamillaDSP when the sample rate of the audio stream being captured changes.
It is meant for use on a USB gadget capture device. I have tested it on my Raspberry Pi 4. It expect it to also work on other boards supporting USB gadget, such as Raspberry Pi Zero, Raspberry PI 3A. Raspberry CM4 and BeagleBones, but I have no means of doing tests on such platforms.
This project was developed on DietPi 64-bit, but other Debian-based Linux distribution should work. It is coded in C language with use o asound and libwebsockets C library API's.
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
