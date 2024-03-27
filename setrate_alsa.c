////////////////////////////////////////////////////
//
// camilladsp-setrate
// Automatic sample rate switcher for CamillaDSP
//
// setrate_alsa.c
// Alsa controls management
//
////////////////////////////////////////////////////

#include <string.h>
#include <alsa/asoundlib.h>
#include <libwebsockets.h>
#include "setrate.h"

extern states state;	// Global state
extern int rate;	// Current sample rate

char device_name[MAX_DEVICE_NAME];  // Capture sound card name 

snd_ctl_t *ctl;                     // Pointer to sound card controls handle 
snd_ctl_event_t *alsa_event;        // Pointer to alsa event 
const char *event_name;             // Pointer to event name
unsigned int event_numid;           // Numeric event id
snd_ctl_elem_value_t *elem_value;   // Pointer to event element value
snd_async_handler_t *async_handler; // Async handler for the alsa controls


////////////////////////////////////////
// Callback function that is triggered
// when an alsa event comes in
////////////////////////////////////////
static void alsa_callback(snd_async_handler_t *handler)
{
    int err;

    // Allocate memory space for alsa events and the related element value 
    snd_ctl_event_alloca(&alsa_event);
    snd_ctl_elem_value_alloca(&elem_value); 

    // Read the incoming event
    err = snd_ctl_read(ctl, alsa_event);

    if (err < 0)
    {
	writelog(ERR, "Error reading event element: %s\n", snd_strerror(err));
	return;
    }

    // Get the event element name
    event_name = snd_ctl_event_elem_get_name(alsa_event);
   
    // If the event is not a sample rate change, there is nothing to be done
    if (strcmp(event_name, ALSA_CONTROL_NAME))
	return;

    // Prepare reading of the event element by setting event numid 
    event_numid = snd_ctl_event_elem_get_numid(alsa_event); 
    snd_ctl_elem_value_set_numid(elem_value, event_numid);

    // Read event element
    err = snd_ctl_elem_read(ctl, elem_value);

    if (err < 0)
    {
        writelog(ERR, "Error reading event element: %s\n", snd_strerror(err));
        return;
    }
   
    // The event element info includes an array of integers. 
    // The first integer (index = 0) stores the sample rate
    err = snd_ctl_elem_value_get_integer(elem_value, 0); 

    // If the value is lower or equal to zero, there is nothing to be done
    if (err <= 0)
	return;

    // update the current sample rate
    rate = err;

    writelog(USER, "BEGIN     : Incoming Sample Rate = %d Hz\n", err);

    // Trigger a transition of the finite-state machine
    fsm_transit(RATE_CHANGE);
}


///////////////////////////////////////
// Initialise alsa controls
///////////////////////////////////////
void alsa_init()
{
    int err; 

    // Open the sound card (capture device) alsa controls
    err = snd_ctl_open(&ctl, device_name, SND_CTL_NONBLOCK);

    if (err < 0)
    {
	writelog(ERR, "Fatal error: Cannot open alsa controls for the device %s: %s\n", device_name, snd_strerror(err));
        exit(FAIL);
    }

    // Add and async handler for the alsa controls
    err = snd_async_add_ctl_handler(&async_handler, ctl, alsa_callback, NULL); 
	
    if (err < 0)
    {
        writelog(ERR, "Fatal error: Cannot add an asynchronous handler to the alsa controls: %s\n", snd_strerror(err));
        snd_ctl_close(ctl);
        exit(FAIL);
    }

    // Subscribe to alsa events 
    err = snd_ctl_subscribe_events(ctl, 1);

    if (err < 0)
    {
        writelog(ERR, "Fatal error: Cannot subscribe to alsa events: %s\n", snd_strerror(err));
        snd_ctl_close(ctl);
        exit(FAIL);
    }

    writelog(NOTICE, "Alsa initialised for the device %s\n", device_name);

    return;
}
