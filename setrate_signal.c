////////////////////////////////////////////////////
//
// camilladsp-setrate
// Automatic sample rate switcher for CamillaDSP
//
// setrate_signal.c
// Signal management
//
////////////////////////////////////////////////////

#include <signal.h>
#include <libwebsockets.h>
#include "setrate.h"

extern states state;    // Global state
int interrupted = 0;	// SIGINT signal flag

///////////////////////////////////////
// Enable/Disable signal catching
///////////////////////////////////////
void signal_control(int enable)
{
    if (enable)
    {
	signal(SOUNDCARD_UP_SIG, soundcard_up_handler);
	signal(SIGINT, sigint_handler);
    }
    else
    {
	signal(SOUNDCARD_UP_SIG, SIG_IGN);
	signal(SIGINT, SIG_IGN);
    }
}


///////////////////////////////////////
// Handle incoming signal notifying
// availability of the playback device
///////////////////////////////////////
void soundcard_up_handler(int sig)
{
    writelog(USER, "%20s: Playback device is up\n", decode_state(state));

    // Delay to give CamillaDSP time to access
    // the just appeared playback device
    sleep(1);

    // Trigger a transition of the finite-state machine
    // as if a sample rate change had occurred
    fsm_transit(RATE_CHANGE);
}

///////////////////////////////////////
// Handle incoming signal SIGINT 
///////////////////////////////////////
void sigint_handler(int sig)
{
    interrupted = 1;
}
