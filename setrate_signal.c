////////////////////////////////////////////////////
// camilladsp-setrate
// Automatic sample rate switcher for CamillaDSP
//
// setrate_signal.c
// Signal management
//
////////////////////////////////////////////////////

#include <string.h>
#include <signal.h>
#include <libwebsockets.h>
#include <errno.h>
#include "setrate.h"

extern struct lws_context *context;    // Global context
extern enum states state;

int dacready_signal = SIGHUP;          // Signal used to notify USB DAC avilability


///////////////////////////////////////
// Enable/Disable signal catching
///////////////////////////////////////
void signal_control(int enable)
{
    void *err;
    
    if (enable)
	signal(dacready_signal, signal_handler);
    else
	signal(dacready_signal, NULL);

    if (err == SIG_ERR)
    {
	lwsl_err("Cannot catch signal: %s\n", strerror(errno));
	exit(FAIL);
    }
}


///////////////////////////////////////
// Handle incoming signal
///////////////////////////////////////
void signal_handler(int sig)
{
    static int err;

    // Cancel websocket servicing 
    lws_cancel_service(context);

    lwsl_user("%s: DAC availability signalled\n", decode_state(state));

    // Unsubscribe from alsa events 
    alsa_subscribe(DISABLE);

    lwsl_notice("%s: Alsa events unsubscribed\n", decode_state(state));

    // Require websocket servicing again 
    lws_service(context, 0);

}


