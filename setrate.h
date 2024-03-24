////////////////////////////////////////////////////
//
// camilladsp-setrate
// Automatic sample rate switcher for CamillaDSP
//
// setrate.h
// Defines, data structures and function templates 
//
////////////////////////////////////////////////////

#define TRUE        1
#define FALSE       0
#define FAIL       -1
#define SUCCESS     0
#define DISABLE     0
#define ENABLE      1
#define BUFLEN  32768		    // Max size of websocket receive buffer
#define CUTLEN    80		    // Size of a truncated string

#define RECONN_INTERVAL    3*LWS_USEC_PER_SEC	// Interval between reconnection attempts

#define MAX_DEVICE_NAME     40	    // Maximum lenght of alsa device name
#define MAX_ADDRESS_LEN     80	    // Maximum lenght of  websocket server IP address

#define VERSION     "2.1.1"	    // Version number 

#define ERR         LLL_ERR
#define WARN        LLL_WARN
#define USER        LLL_USER
#define NOTICE      LLL_NOTICE

#define lwsl_ERR	    lwsl_err
#define lwsl_WARN	    lwsl_warn
#define lwsl_USER	    lwsl_user
#define lwsl_NOTICE	    lwsl_notice
#define writelog(A, ...)    lwsl_ ## A (__VA_ARGS__)


// States of the finite-state machine
typedef enum
{
    INIT,		    // Initialisation
    START,		    // Initial state 
    WAIT_RATE_CHANGE,	    // Waiting for a sample rate change
    RATE_CHANGED,	    // Sample rate changed
    WAIT_CONFIG,	    // Waiting for configuration
    CONFIG_RECEIVED_OK,	    // The received configuration is valid
    CONFIG_RECEIVED_KO,	    // The received configuration is not valid
    WAIT_PREVIOUS_CONFIG,   // Waiting for the previous configuration
    WAIT_RESPONSE	    // Waiting for response to "SetConfig"
} states;


// Events of the finite-state machine
typedef enum
{
    RATE_CHANGE,    // Sample rate changed
    CONNECT,	    // Connected to websocket server
    DISCONNECT,	    // Disconnected from websocket server
    WRITEABLE,	    // Server can receive a command
    CONFIG_OK,	    // Configuration received, and it is valid
    CONFIG_KO,	    // Configuration received, but it is not valid
    RESPONSE_OK,    // Response to SetConfig received, and it is OK
    RESPONSE_KO	    // Response t SetConfig received, but it not OK
} events;

#define FIRST_STATE INIT
#define LAST_STATE  WAIT_RESPONSE
#define FIRST_EVENT RATE_CHANGE
#define LAST_EVENT  RESPONSE_KO

#define SOUNDCARD_UP_SIG  SIGHUP    // Signal used to notify playback sound card availability 


void alsa_init(void);		        // Initialise alsa control
void websocket_init(void);		// Initialise websocket environment
events  check_received_data(char *);	// Check if the received configuration is valid
void fsm_init(void);			// Initialise the finite-state machine
int  fsm_transit(events);		// Trigger a transition of the finite-state machine
int  notify_success(void);		// Notify configuration update success
int  notify_failure(void);		// Notify configuration update failure
int  reconnection_request(void);	// Schedule a reconnection to the websocket server
int  callback_on_writeable(void);       // Request a callback when the server can accept commands 
int  send_get_config(void);             // Send "GetConfig" command
int  send_get_previous_config(void);    // Send "GetPreviousConfig" command
int  send_set_config(void);		// Send "SetConfig" command
void signal_control(int);		// Enable/disable signal catching
void soundcard_up_handler(int);		// Signal handler (Playback device availability)
int  prepare_setconfig(char *, int);	// Prepare the SetConfig command
char *decode_state(states);      		// Return a description of the state
char *decode_event(events);      		// Return a description of the event
char *decode_action(int (*)());      		// Return a description of the callabck function
void print_usage(char *);                       // Print command line usage
