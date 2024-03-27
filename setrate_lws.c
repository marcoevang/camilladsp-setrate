////////////////////////////////////////////////////
//
// camilladsp-setrate
// Automatic sample rate switcher for CamillaDSP
//
// setrate_lws.c
// Websocket management
//
////////////////////////////////////////////////////

#include <string.h>
#include <libwebsockets.h>
#include "setrate.h"

extern int rate;	// Current sample rate 
extern states state;	// Global state
extern int fsm_ret;	// Value returned after a tranistion
			// of the Finite-State Machine 

struct lws_context *context = NULL;  // Global websocket context

// WebSocket callback function
static int websocket_callback(struct lws *, enum lws_callback_reasons, void *, void *, size_t);

// WebSocket protocols
static struct lws_protocols protocols[] = 
{ 
    {
        "MyProtocol",			// Protocol name
        websocket_callback,		// Callback function
        0,				// Per-session data size
        //BUFLEN,				// Receive buffer
        0,				// Receive buffer
        0,				// ID number (reserved)
        NULL,				// User data
        0				// Attach count
    },
    { NULL, NULL, 0, 0, 0, NULL, 0 }	// Terminate the list
};

static struct lws *websocket;		// Pointer to websocket struct
char   server_address[MAX_ADDRESS_LEN]; // IP address of CamillDSP Websocket Server
int    server_port;			// IP port of CamillDSP Websocket Server
static lws_sorted_usec_list_t sul;	// Sorted list for libwebsockets scheduler

// Parameters for libwebsockets ping-pong protocol 
static const lws_retry_bo_t retry = 
{
    .secs_since_valid_ping   = 300,	// Interval between pings (in seconds)
    .secs_since_valid_hangup = 370,	// Hangup timeout if server does not ping back with a pong
};					// i.e. within 10 seconds after ping (370-360=10)


char command[BUFLEN + LWS_PRE + 1];	// Global buffer for command string


/////////////////////////////////////////////
// Attempt connection to websocket server
/////////////////////////////////////////////
static void connection_request(lws_sorted_usec_list_t *_sul)
{
    struct lws_client_connect_info connect_info;    // libwebsocket connection info

    // Set up libwebsockets connection info
    memset(&connect_info, 0, sizeof(connect_info));

    connect_info.protocol                  = protocols[0].name;
    connect_info.ietf_version_or_minus_one = -1;
    connect_info.context                   = context;
    connect_info.port                      = server_port;
    connect_info.address                   = server_address;
    connect_info.retry_and_idle_policy     = &retry;
    connect_info.pwsi                      = &websocket;
 
    // Attempt to connect. In case of failure,
    // a reconnection is scheduled after 1 second
    if (!lws_client_connect_via_info(&connect_info))
	lws_sul_schedule(context, 0, _sul, connection_request, RECONN_INTERVAL);
}


////////////////////////////////////////////
// Initialise  websocket environment
////////////////////////////////////////////
void websocket_init()
{
    struct lws_context_creation_info info;	// libwebsocket context creation info

    // Set up libwebsockets context creation info
    memset(&info, 0, sizeof(info));

    info.port      = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;

    // Create libwebsockets context
    context = lws_create_context(&info);

    if (!context)
    {
	writelog(ERR, "Fatal error: Failed to create libwebsockets context\n");
	exit(FAIL);
    }

    writelog(NOTICE, "Websocket environment initialised\n");

    // Initiate a connection by calling the scheduler
    lws_sul_schedule(context, 0, &sul, connection_request, 100);
}


///////////////////////////////////////
// Handle websocket callbacks
///////////////////////////////////////
static int websocket_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    // Disable catching of signal
    signal_control(DISABLE); 

    // Handle callback depending on reason
    switch (reason)
    {
#ifdef LWS_DEBUG
	case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
	    // A new client websocket instance was created 
            writelog(NOTICE, "A new client websocket instance was created\n");
	    break;
#endif

        case LWS_CALLBACK_CLIENT_ESTABLISHED:
	    // Connection with server established
            writelog(NOTICE, "Connected to websocket server at %s:%d\n", server_address, server_port);

	    // Trigger a transition of the finite-state machine
	    fsm_ret = fsm_transit(CONNECT);
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
	    // Server can accept commands
#ifdef LWS_DEBUG
            writelog(NOTICE, "Websocket server can accept commands\n");
#endif

	    // Trigger a transition of the finite-state machine
	    fsm_ret = fsm_transit(WRITEABLE);
            break;
	
        case LWS_CALLBACK_CLIENT_RECEIVE:
	    // Data received from server
	    strcpy(command + LWS_PRE, (char *)in);

#ifdef DEBUG
            writelog(NOTICE, "Response  : %.*s\n", CUTLEN, (char *)in);
#endif

	    // Trigger a transition of the finite-state machine
	    fsm_ret = fsm_transit(check_received_data((char *)in));
            break;

#ifdef LWS_DEBUG
	case LWS_CALLBACK_CLIENT_RECEIVE_PONG:
	    writelog(NOTICE, "Pong received from websocket server\n");
	    break;
#endif

        case LWS_CALLBACK_CLIENT_CLOSED:
            // Connection closed by client.
            writelog(NOTICE, "Disconnected from websocket server %s:%d\n", server_address, server_port);

	    // Trigger a transition of the finite-state machine
	    fsm_ret = fsm_transit(DISCONNECT);
            break;
	    
	case LWS_CALLBACK_CLOSED:
            // Connection closed by server.
            writelog(WARN, "Websocket server triggered disconnection\n");

	    // Trigger a transition of the finite-state machine
	    fsm_ret = fsm_transit(DISCONNECT);
            break;

#ifdef LWS_DEBUG
        case LWS_CALLBACK_WSI_DESTROY:
            // Websocket instance was destroyed after connection closing
            writelog(NOTICE, "Websocket instance destroyed\n");
            break;
#endif

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            // Connection ended with error
            writelog(ERR, "Connection to Websocket Server at %s:%d failed: %s\n", server_address, server_port, (char *)in);

	    // Trigger a transition of the finite-state machine
	    fsm_ret = fsm_transit(DISCONNECT);
            break;

        default:
#ifdef LWS_DEBUG
	    writelog(NOTICE, "Callback reason %2d (not an error)\n", reason);
#endif
            break;
    }

    // Re-enable catching of signals
    signal_control(ENABLE);

    // Return zero as the client never requests a disconnection
    return(0);
}


//////////////////////////////////////////////
//////////////////////////////////////////////
//// Actions of the finite-state machine  ////
//////////////////////////////////////////////
//////////////////////////////////////////////

//////////////////////////////////////////////
// Send request for callback when the server
// can accept commands
//////////////////////////////////////////////
int callback_on_writeable(void)
{
    lws_callback_on_writable(websocket);

    return(SUCCESS);
}


//////////////////////////////////////////////
// Send "GetConfig" command
//////////////////////////////////////////////
int send_get_config(void)
{
    strcpy(command + LWS_PRE, "\"GetConfig\"");

#ifdef DEBUG
    writelog(NOTICE, "Command   : %s\n", command + LWS_PRE);
#endif

    // Send the command to websocket server
    lws_write(websocket, command + LWS_PRE, strlen(command + LWS_PRE), LWS_WRITE_TEXT);

    return(SUCCESS);
}


//////////////////////////////////////////////
// Send "GetPreviousConfig" command
//////////////////////////////////////////////
int send_get_previous_config(void)
{
    strcpy(command + LWS_PRE, "\"GetPreviousConfig\"");

#ifdef DEBUG
    writelog(NOTICE, "Command   : %s\n", command + LWS_PRE);
#endif

    // Send the command to websocket server
    lws_write(websocket, command + LWS_PRE, strlen(command + LWS_PRE), LWS_WRITE_TEXT);

    return(SUCCESS);
}


//////////////////////////////////////////////
// Send "SetConfig" command
//////////////////////////////////////////////
int send_set_config(void)
{
    int ret;

    ret = prepare_setconfig(command + LWS_PRE, rate);

    if (ret == SUCCESS)
    {
#ifdef DEBUG
	writelog(NOTICE, "Command   : %.*s\n", CUTLEN, command + LWS_PRE);
#endif

	// Send the command to websocket server
	lws_write(websocket, command + LWS_PRE, strlen(command + LWS_PRE), LWS_WRITE_TEXT);
    }
    else
    {
	writelog(ERR, "Error in the preparation of the command SetConfig. ret=%d\n", ret);
	exit(FAIL);
    }

    return(SUCCESS);
}


//////////////////////////////////////////////
// Schedule a new connection 
// to the websocket server
//////////////////////////////////////////////
int reconnection_request(void)
{
    writelog(NOTICE, "Reconnection attempt...\n");

    // Schedule a new connection to websocket server
    lws_sul_schedule(context, 0, &sul, connection_request, RECONN_INTERVAL);

    return(SUCCESS);
}


