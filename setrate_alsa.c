////////////////////////////////////////////////////
// camilladsp-setrate
// Automatic sample rate switcher for CamillaDSP
//
// setrate_alsa.c
// Alsa control management
//
////////////////////////////////////////////////////

#include <alsa/asoundlib.h>
#include <libwebsockets.h>
#include "setrate.h"

const char *device_name = "hw:UAC2Gadget";   // Sound card name 

snd_ctl_t *ctl;                              // Pointer to sound card control handle 

snd_ctl_event_t *event;                      // Pointer to alsa event 
const char *event_name;                      // Pointer to event name
unsigned int event_numid;                    // Numeric event id
snd_ctl_elem_value_t *elem_value;            // Pointer to event element value

///////////////////////////////////////
// Initialise alsa control
///////////////////////////////////////
void alsa_init()
{
    int err;

    // Open the sound card (capture device) alsa control
    err = snd_ctl_open(&ctl, device_name, SND_CTL_READONLY);

    if (err < 0) 
    {
        lwsl_err("Cannot open ctl for the device %s: %s\n", device_name, snd_strerror(err));
        exit(FAIL);
    }

    // Subscribe to alsa events 
    err = snd_ctl_subscribe_events(ctl, 1);

    if (err < 0) 
    {
        lwsl_err("Cannot subscribe to alsa events: %s\n", snd_strerror(err));
        snd_ctl_close(ctl);
        exit(FAIL);
    }

    return;
}

///////////////////////////////////////
// Subscribe to alsa events
///////////////////////////////////////
void alsa_subscribe(int enable)
{
    int err;

    err = snd_ctl_subscribe_events(ctl, enable);

    if (err < 0) 
    {
	lwsl_err("Cannot subscribei/unsubscribe alsa events: %s\n", snd_strerror(err));
	snd_ctl_close(ctl);
	exit(FAIL);
    }
}


///////////////////////////////////////
// Get sample rate on change
///////////////////////////////////////
int alsa_getrate() 
{

    int err; 
    int rate; 

    // Allocate memory space for alsa events and the related element value 
    snd_ctl_event_alloca(&event);
    snd_ctl_elem_value_alloca(&elem_value); 

    // Catch alsa events until a sample rate change is detected
    do 
    {
        // Read alsa event - block until an event is notified
        err = snd_ctl_read(ctl, event);

        if (err < 0) 
	    // Subscription to alsa events was canceled by
	    // signal handler after notification of DAC availability,
	    // thus causing abort of event catching.
            return(err);

        // Get event name
        event_name = snd_ctl_event_elem_get_name(event);
    } 
    while(strcmp(event_name, "Capture Rate"));

    // Prepare reading of event element by setting event numid (mandatory)
    event_numid = snd_ctl_event_elem_get_numid(event); 
    snd_ctl_elem_value_set_numid(elem_value, event_numid);

    // Read event element
    err = snd_ctl_elem_read(ctl, elem_value);

    if (err < 0) 
    {
        lwsl_err("Cannot read event element: %s\n", snd_strerror(err));
        exit(FAIL);
    }
   
    // The event element info includes an array of integers. 
    // The first integer (index = 0) stores the sample rate
    rate = snd_ctl_elem_value_get_integer(elem_value, 0); 

    return(rate);
}

