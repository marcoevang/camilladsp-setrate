////////////////////////////////////////////////////
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
#define BUFLEN   2048                          	// Max size of websocket receive buffer
#define CUTLEN    160				// Size of a cutted string
#define MAX_CONNECT 5				// Maximum number of connection attempts 

#define MAX_DEVICE_NAME     40			// Maximum lenght of alsa device name
#define MAX_SERVER_ADDRESS 128			// Maximum lenght of server full address

#define VERSION     "1.0.0"			// Version number 

#define ERR         LLL_ERR
#define WARN        LLL_WARN
#define USER        LLL_USER
#define NOTICE      LLL_NOTICE


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

void alsa_init(void);			        // Initialise alsa control
void alsa_close(void);			        // Close alsa control
int alsa_getrate(void);				// Get sample rate from alsa control when it changes 
void alsa_subscribe(int);			// Subscribe to alsa controls
void websocket_init(void);			// Initialise websocket environment
int connection_request(void);			// Attempt to connect to server
void change_state(enum states);			// cwChange state and log at NOTICE level
void signal_control(int);			// Enable/disable signal catching
void signal_handler(int);			// Signal handler (DAC availability)
int prepare_setconfig_command(char *, int);	// Prepare the SetConfig command
char *decode_state(enum states);		// Return a description of the state
void writelog(int, char *, ...);                // Write log messages
void print_usage(char *);                       // Print command line usage
void split_address(char *);			// Split address:port into address and port
