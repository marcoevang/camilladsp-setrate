////////////////////////////////////////////////////
// camilladsp-setrate
// Automatic sample rate switcher for CamillaDSP
//
// setrate_main.c
// Main program
//
////////////////////////////////////////////////////

#include <libwebsockets.h>
#include <getopt.h>
#include "setrate.h"


extern enum states state;             // Global state
extern struct lws_context *context;   // Websocket context

int rate;                             // Sample rate

// Command line optiions
static int logerr    = 0;
static int logwarn   = 0;
static int loguser   = 0;
static int lognotice = 0;
static int timestamp = FALSE;
static int syslog    = FALSE;

static struct option long_options[] =
{
    {"err",       no_argument, &logerr,    LLL_ERR},
    {"warn",      no_argument, &logwarn,   LLL_WARN},
    {"user",      no_argument, &loguser,   LLL_USER},
    {"notice",    no_argument, &lognotice, LLL_NOTICE},
    {"timestamp", no_argument, &timestamp, TRUE},
    {"syslog",    no_argument, &syslog,    TRUE},
    {0,           0,           0,          0}
};

int main(int argc, char* argv[]) 
{
    int err;
    int option;

    // Evaluate options
    while (TRUE)
    {
        int option_index = 0;

        option = getopt_long(argc, argv, "", long_options, &option_index);

        if (option == -1)
            break;

        if (option == '?')
        {
            fprintf(stderr, "Unknown option %s\n", long_options[option_index].name);
            exit(FAIL);
        }
    }

    // Set libwebsockets log level
    if (syslog)
	lws_set_log_level(logerr | logwarn | loguser | lognotice, lwsl_emit_syslog);
    else
	lws_set_log_level(logerr | logwarn | loguser | lognotice, timestamp ? NULL : lwsl_emit_stderr_notimestamp);

    // Enable catching of the signal used to notify USB DAC availability
    signal_control(ENABLE);

    // Initialise websocket environment
    websocket_init();

    // Initialise alsa control
    alsa_init();
	
    // MAIN LOOP 
    // Wait for a sample rate change.
    // If the sample rate is not zero, request a connection
    // to CamillaDSP websocket server.
    // Then the config update process starts:
    // 1) Once connected, Send the GetConfig command
    // 2) Wait for server to send the current config
    // 3) Once received, replace the samplerate and chunksize values
    //    with the new ones and send the SetConfig command
    // 4) Wait for server to send the response
    // 5) Once received, disconnect from server 
    // Then loop again waiting for another sample rate change
    // Note: Steps from 1) to 5) are entirely handled by 
    // the libwebsocket callback function.

    state = WAIT_RATE_CHANGE;
    err = 0;
    do 
    {
	int ret;

        switch (state)
        {
            case WAIT_RATE_CHANGE:
                // Wait for alsa to notify a sample rate change
		lwsl_notice("%s: Waiting for sample rate change...\n", decode_state(state)); 
                ret = alsa_getrate();

		// If the new sample rate is zero, nothing needs to be done.
		if (ret == 0)
		    break;

		// If the new sample rate is greater than zero, 
		// the current sample rate must be updated.
		if (ret > 0)
		    rate = ret;

		// If alsa_getrate returned with error (ret < 0) this means that
		// the signal handler cancelled subscription to alsa events,
		// thus causing abort of event catching:
		// Subscription must be renewed. 
		if (ret < 0)
		    alsa_subscribe(ENABLE);

		lwsl_user("%s: Sample Rate = %d\n", decode_state(state), rate); 

		// Request connection to CamillaDSP websocket server.
		if (connection_request() == FAIL)
		{
		    lwsl_err("%s: Failed to connect to WebSocket server\n", decode_state(state));
		    continue;
		}

		state = CONNECTION_REQUEST;
		notice_state_change();

                break;
            case RECONNECTION_REQUEST:
                // The connection was closed by server while the 
                // config update process was ongoing. Try to reconnect.
                if (connection_request() == FAIL)
                {
                    lwsl_err("%s: Failed to reconnect to WebSocket server\n", decode_state(state));
                    state = WAIT_RATE_CHANGE;
                    notice_state_change();
                }

                break;
            default:
		// Request websocket servicing
                err = lws_service(context, 0);
                break;
        }
    }
    while (err >= 0);

    // Destroy websocket context
    lws_context_destroy(context);

    exit(SUCCESS);
}


