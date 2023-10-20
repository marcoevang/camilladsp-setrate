////////////////////////////////////////////////////
//
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

extern char   device_name[];	    // Name of the capture device
extern struct lws_context *context; // Websocket context
extern int    server_port;	    // Server IP port
extern char   server_address[];	    // Server IP address
extern states state;		    // Global state

int rate;			    // Sample rate

char level[10];
int  logmask;

// Command line options
static int logerr    = 0;
static int logwarn   = 0;
static int loguser   = 0;
static int lognotice = 0;
static int timestamp = FALSE;
static int syslog    = FALSE;
static int device    = FALSE;
static int address   = FALSE;
static int port      = FALSE;
static int loglevel  = FALSE;

int capture	     = FALSE;

static struct option long_options[] =
{
    {"err",       no_argument,       &logerr,    LLL_ERR},
    {"warn",      no_argument,       &logwarn,   LLL_WARN},
    {"user",      no_argument,       &loguser,   LLL_USER},
    {"notice",    no_argument,       &lognotice, LLL_NOTICE},
    {"timestamp", no_argument,       &timestamp, TRUE},
    {"syslog",    no_argument,       &syslog,    TRUE},
    {"capture",   no_argument,       &capture,   TRUE},
    {"device",    required_argument, &device,    TRUE},
    {"address",   required_argument, &address,   TRUE},
    {"port",      required_argument, &port,      TRUE},
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
    pid_t child_pid;    // pid of child process
    int err;
    int ret;
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
	    printf("\n%s v%s\n\n", argv[0], VERSION);
	    exit(SUCCESS);
	}
	else if (!strcmp(long_options[option_index].name, "device"))
	    strcpy(device_name, optarg);
	else if (!strcmp(long_options[option_index].name, "address"))
	    strcpy(server_address, optarg);
	else if (!strcmp(long_options[option_index].name, "port"))
	    server_port = atoi(optarg);
	else if (!strcmp(long_options[option_index].name, "loglevel"))
	    strcpy(level, optarg);
    }

    // Device name defaults to hw:UAC2Gadget
    if (!device)
	strcpy(device_name, "hw:UAC2Gadget");

    // Server address defaults to localhost
    if (!address)
	strcpy(server_address, "localhost");

    // Server port defaults to 1234
    if (!port)
	server_port = 1234;

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

    writelog(NOTICE, "%s %s\n", argv[0], VERSION);
    writelog(NOTICE, "An automatic sample rate switcher for CamillaDSP\n");

    // Initialise the finite-state machine
    fsm_init();

    // Initialise websocket environment
    websocket_init();

    // Enable signal catching 
    signal_control(ENABLE);

    // Initialise alsa control
    alsa_init();

    err = 0;

    // Request websocket servicing
    while (err >= 0)
	err = lws_service(context, 0);

    // Destroy websocket context
    lws_context_destroy(context);

    exit(SUCCESS);
}


/////////////////////////////////////////////
// Print command usage
/////////////////////////////////////////////
void print_usage(char *cmd)
{
    printf("%s v%s\n", cmd, VERSION);
    printf("An automatic sample rate switcher for CamillaDSP\n");
    printf("\nUSAGE:\n    %s [FLAGS] [OPTIONS]\n", cmd);
    printf("\nFLAGS:\n");
    printf("    -c, --capture                   Update capture_samplerate instead of samplerate\n");
    printf("    -e, --err                       Enable logging of errors\n");
    printf("    -w, --warn                      Enable logging of warnings\n");
    printf("    -u, --user                      Enable logging of key events\n");
    printf("    -n, --notice                    Enable logging of internal events\n");
    printf("    -t, --timestamp                 Prepend timestamp to log messages\n");
    printf("    -s, --syslog                    Redirect logging to syslog [if omitted, logging emits to stderr]\n");
    printf("    -v, --version                   Print software version\n");
    printf("    -h, --help                      Print this help\n");
    printf("\nOPTIONS:\n");
    printf("    -d, --device=<capture device>   Set alsa capture device name [default: hw:UAC2Gadget]\n");
    printf("    -a, --address=<address>         Set server IP address [default: localhost]\n");
    printf("    -p, --port=<port>               Set server IP port [default: 1234]\n");
    printf("    -l, --loglevel=<log level>      Set log level - Override flags [values: err, warn, user, notice]\n");
    printf("\n");
}



