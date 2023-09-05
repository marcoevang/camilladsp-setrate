# camilladsp-setrate
Automatic sample rate switcher for CamillaDSP

camilladsp-setrate automatically updates active sample rate parameter of CamillaDSP when the sample rate of the stream being captured changes.
It is meant for use on a USB gadget capture device. I have tested it on my Raspberry Pi 4. It could also work with other boards supporting USB gadget, such as Raspberry Pi Zero, Raspberry PI 3A and BeagleBones. However, I am not able to test on such boards.
This project is fully developed in C language. 
To build the executable file you need to previously install some packages:
sudo apt update
sudo apt install build-essential libasound2-dev libwebsockets

