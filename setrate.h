////////////////////////////////////////////////////
// camilladsp-setrate
// Automatic sample rate switcher for CamillaDSP
//
// setrate.h
// Defines, data structures and function templates 
//
////////////////////////////////////////////////////

#define TRUE  1
#define FALSE 0
#define FAIL   -1
#define SUCCESS 0
#define DISABLE 0
#define ENABLE 1
#define BUFLEN 2048                          	// Max size of websocket receive buffer
#define CUTLEN 120				// Size of a cutted string
#define MAX_CONNECT   5				// Maximum number of connection attempts 

// States of the config update process
enum    states
        {
            CONNECTION_REQUEST,                 // A connection to CamillaDSP server has been requested
            CONNECTED,                          // Connected to CamillaDSP websocket server
            WAIT_RATE_CHANGE,                   // Waiting for a sample rate change 
            WAIT_GET_CONFIG_RESPONSE,           // Waiting for the server to send the current config
            GET_CONFIG_RESPONSE_RECEIVED,       // The current config has been received
            WAIT_SET_CONFIG_RESPONSE,           // Waiting for response to config update command 
            RECONNECTION_REQUEST                // Reconnection attempt if server closed the connection 
        };

void alsa_init();			        // Initialise alsa control
int alsa_getrate();				// Get sample rate from alsa control when it changes 
void alsa_subscribe(int);			// Subscribe to alsa controls
void websocket_init();				// Initialise websocket environment
int connection_request(void);			// Attempt to connect to server
void notice_state_change(void);			// Log a state change at NOTICE level
void signal_control(int);			// Enable/disable signal catching
void signal_handler(int);			// Signal handler (DAC availability)
int prepare_setconfig_command(char *, int);	// Prepare the SetConfig command
char *decode_state(enum states);		// Return a description of the state
