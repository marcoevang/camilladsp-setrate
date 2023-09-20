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


extern struct lws_context *context;    // Websocket context
extern char device_name[];             // Name of the capture device
extern char server_address[];          // Server IP address
extern int  server_port;               // Server IP port

char level[10];
int  logmask;

enum states state = WAIT_RATE_CHANGE; // Global state

int rate;                              // Sample rate

// Command line options
static int logerr    = 0;
static int logwarn   = 0;
static int loguser   = 0;
static int lognotice = 0;
static int timestamp = FALSE;
static int syslog    = FALSE;
static int device    = FALSE;
static int server    = FALSE;
static int loglevel  = FALSE;

static struct option long_options[] =
{
    {"err",       no_argument,       &logerr,    LLL_ERR},
    {"warn",      no_argument,       &logwarn,   LLL_WARN},
    {"user",      no_argument,       &loguser,   LLL_USER},
    {"notice",    no_argument,       &lognotice, LLL_NOTICE},
    {"timestamp", no_argument,       &timestamp, TRUE},
    {"syslog",    no_argument,       &syslog,    TRUE},
    {"device",    required_argument, &device,    TRUE},
    {"server",    required_argument, &server,    TRUE},
    {"loglevel",  required_argument, &loglevel,  TRUE},
    {"help",      no_argument,       NULL,       0},
    {"version",   no_argument,       NULL,       0},
    {0,           0,                 0,          0}
};



/////////////////////////////////////////////
// MAIN PROGRAM
/////////////////////////////////////////////
int main(int argc, char* argv[]) 
{
    int err;
    int option;

    // Evaluate options
    opterr = 0;

    while (TRUE)
    {
        int option_index = 0;

        option = getopt_long_only(argc, argv, "", long_options, &option_index);

        if (option == -1)
            break;

        if (option == '?')
        {
            printf("\nError: invalid option %s\n", argv[optind-1]);
	    print_usage(argv[0]);
	    exit(FAIL);
        }

	if (!strcmp(long_options[option_index].name, "help"))
	{
	    print_usage(argv[0]);
	    exit(SUCCESS);
	}
	else if (!strcmp(long_options[option_index].name, "version"))
	{
	    printf("\n%s %s\n\n", argv[0], VERSION);
	    exit(SUCCESS);
	}
	else if (!strcmp(long_options[option_index].name, "device"))
	    strcpy(device_name, optarg);
	else if (!strcmp(long_options[option_index].name, "server"))
	    split_address(optarg);
	else if (!strcmp(long_options[option_index].name, "loglevel"))
	    strcpy(level, optarg);
    }

    // Device name defaults to hw:UAC2Gadget
    if (!device)
	strcpy(device_name, "hw:UAC2Gadget");

    // Server address defaults to localhost
    // Server port defaults to 1234
    if (!server)
    {
	strcpy(server_address, "localhost");
	server_port = 1234;
    }

    // Set the log level mask
    if (loglevel)
	if (!strcmp(level, "err"))
	    logmask = ERR;
	else if (!strcmp(level, "warn"))
	    logmask = ERR | WARN;
	else if (!strcmp(level, "user"))
	    logmask = ERR | WARN | USER;
	else if (!strcmp(level, "notice"))
	    logmask = ERR | WARN | USER | NOTICE;
	else
	{
	    printf("Invalid log level: %s\n", level);
	    print_usage(argv[0]);
	    exit(FAIL);
	}
    else
	logmask = logerr | logwarn | loguser | lognotice;

    // Set libwebsockets log level
    if (syslog)
	lws_set_log_level(logmask, lwsl_emit_syslog);
    else
	lws_set_log_level(logmask, timestamp ? NULL : lwsl_emit_stderr_notimestamp);

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

    err = 0;
    do 
    {
	int ret;

        switch (state)
        {
            case WAIT_RATE_CHANGE:
                // Wait for alsa to notify a sample rate change
		writelog(NOTICE, "Waiting for sample rate change...\n"); 
                ret = alsa_getrate();

		// If the new sample rate is zero, nothing needs to be done.
		if (ret == 0)
		    break;

		// If the new sample rate is greater than zero, 
		// the current sample rate must be updated.
		if (ret > 0)
		    rate = ret;

		// If alsa_getrate returned with error (ret < 0) 
		// most likely the signal handler closed the alsa control
		// to force exit from waiting for an alsa event:
		// Alsa control must be re-initialised.
		if (ret < 0)
		{
		    alsa_init();

		    // Delay to give CamillaDSP time to find the
		    // just-appeared USB playback device.
		    sleep(1);
		}

		writelog(USER, "Sample Rate = %d\n", rate); 

		// Request connection to CamillaDSP websocket server.
		if (connection_request() == FAIL)
		{
		    writelog(ERR, "Connecton to server %s on port %d failed.\n", server_address, server_port);
		    continue;
		}

		change_state(CONNECTION_REQUEST);

                break;
            case RECONNECTION_REQUEST:
                // The connection was closed by server while the 
                // config update process was ongoing. Try to reconnect.
                if (connection_request() == FAIL)
                {
		    writelog(ERR, "Failed to connect to server %s on port %d\n", server_address, server_port);
                    change_state(WAIT_RATE_CHANGE);
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


/////////////////////////////////////////////
// Print command usage
/////////////////////////////////////////////
void print_usage(char *cmd)
{
    printf("\n%s %s\n", cmd, VERSION);
    printf("Marco Evangelista <marcoevang@gmail.com>\n");
    printf("An automatic sample rate switcher for CamillaDSP\n");
    printf("\nUSAGE:\n    %s [FLAGS] [OPTIONS]\n", cmd);
    printf("\nFLAGS:\n");
    printf("    --err                       Enable logging of errors\n");
    printf("    --warn                      Enable logging of warnings\n");
    printf("    --user                      Enable logging of key events\n");
    printf("    --notice                    Enable logging of internal events\n");
    printf("    --timestamp                 Prepend timestamp to log messages\n");
    printf("    --syslog                    Redirect logging to syslog [if omitted, logging emits to stderr]\n");
    printf("    --version                   Print software version\n");
    printf("    --help                      Print this help\n");
    printf("\nOPTIONS:\n");
    printf("    --device <capture device>   Set alsa capture device name [default: hw:UAC2Gadget]\n");
    printf("    --server <address>:<port>   Set server IP address and port [default: localhost:1234]\n");
    printf("    --loglevel <log level>      Set log level - Override flags [values: err, warn, user, notice]\n");
    printf("\n");
}


/////////////////////////////////////////////
// Split address:port into address and port
/////////////////////////////////////////////
void split_address(char *address)
{
    char *p;

    // In case server address or port
    // are omitted or are 0
    // they default, respectively, 
    // to localhost and 1234

    p = strchr(address, ':');

    if (p)
    {
	server_port = atoi(p+1);
	
	if (server_port == 0)
	    server_port = 1234;

	*p = '\0';
    }
    else
	server_port = 1234;

    if (strlen(address) == 0)
	strcpy(server_address, "localhost");
    else
	strcpy(server_address, address);

    return;
}

