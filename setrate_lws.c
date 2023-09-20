////////////////////////////////////////////////////
// camilladsp-setrate
// Automatic sample rate switcher for CamillaDSP
//
// setrate_lws.c
// Websocket management
//
////////////////////////////////////////////////////

#include <string.h>
#include <libwebsockets.h>
#include <stdarg.h>
#include "setrate.h"

extern int rate;                      // Current sample rate 

struct lws_context *context = NULL;  // Global websocket context

// WebSocket callback function
// static int websocket_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);
static int websocket_callback(struct lws *, enum lws_callback_reasons, void *, void *, size_t);

// WebSocket protocols
static struct lws_protocols protocols[] = 
{ 
    {
        "MyProtocol",			// Protocol name
        websocket_callback,		// Callback function
        0,				// Per-session data size
        BUFLEN,				// Receive buffer
        0,				// ID number (reserved)
        NULL,				// User data
        0				// Attach count
    },
    { NULL, NULL, 0, 0, 0, NULL, 0 }	// Terminate the list
};

struct lws_context_creation_info info;             // libwebsocket context creation info
struct lws_client_connect_info connect_info;       // libwebsocket connection info
struct lws *websocket;                             // Pointer to websocket struct
char   server_address[MAX_SERVER_ADDRESS];         // IP address of CamillDSP Websocket Server
int    server_port;			           // IP port of CamillDSP Websocket Server

extern enum states state;                          // Global state
char command[BUFLEN];                              // Global buffer for command string
int config_is_ok = TRUE;                           // Flag indicating the received config is valid



/////////////////////////////////////////////
// Initialise  websocket environment
/////////////////////////////////////////////
void websocket_init()
{
    // Set up libwebsockets context creation info
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    // Create libwebsockets context
    context = lws_create_context(&info);

    if (!context)
    {
	writelog(ERR, "Failed to create libwebsockets context\n");
	exit(FAIL);
    }

    // Set up libwebsockets connection info
    memset(&connect_info, 0, sizeof(connect_info));
    connect_info.context = context;
    connect_info.address = server_address;
    connect_info.port = server_port;
    connect_info.protocol = protocols[0].name;  // Use the first protocol defined
    connect_info.ietf_version_or_minus_one = -1;

    // END WEBSOCKET INITIALISATION
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
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            writelog(NOTICE, "Connected to Websocket Server %s on port %d\n", server_address, server_port);

            if (state == CONNECTION_REQUEST || state == RECONNECTION_REQUEST)
            {
                change_state(CONNECTED);

                // Ask for a callback when server can accept a command
                lws_callback_on_writable(wsi);
            }

            break;
        case LWS_CALLBACK_CLIENT_WRITEABLE:
            writelog(NOTICE, "Client writeable\n");

            switch(state)
            {
                case CONNECTED:
                    // The server can accept the GetConfigCommand.
                    strcpy(command + LWS_PRE, "\"GetConfig\"");
                    writelog(USER, "Command  = %s\n", command + LWS_PRE);
                    change_state(WAIT_GET_CONFIG_RESPONSE);
                    lws_write(wsi, command + LWS_PRE, strlen(command + LWS_PRE), LWS_WRITE_TEXT);
                    break;
                case GET_CONFIG_RESPONSE_RECEIVED:
                    // The server can accept a command.
                    if (prepare_setconfig_command(command + LWS_PRE, rate) == TRUE)
                    {
                        // Received configuration is valid
                        config_is_ok = TRUE;
                        // Current config is valid. Send SetConfig command.
                        change_state(WAIT_SET_CONFIG_RESPONSE);
                    }
                    else
                    {
                        // Received configuration is None or invalid
                        if (config_is_ok == TRUE)
                        {
                            // Current config is None or invalid. Request previous config.
                            config_is_ok = FALSE;
                            writelog(WARN, "Current configuration is None or Invalid. Trying with previous config.\n");
                            strcpy(command + LWS_PRE, "\"GetPreviousConfig\"");
                            change_state(WAIT_GET_CONFIG_RESPONSE);
                        }
                        else
                        {
                            // Current and previous configs are both None ore invalid. Config update process abort.
                            writelog(WARN, "Current and previous configurations are both None or Invalid. Config update process abort.\n");
                            change_state(WAIT_RATE_CHANGE);
                            lws_cancel_service(context);
                            lws_close_reason(wsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);
			    signal_control(ENABLE); 
                            return (-1);
                        }
                    }

                    writelog(USER, "Command  = %s\n", command + LWS_PRE);
                    lws_write(wsi, command + LWS_PRE, strlen(command + LWS_PRE), LWS_WRITE_TEXT);
                    break;
                default:
                    writelog(NOTICE, "Unforeseen state in LMS_CALLBACK_CLIENT_WRITABLE callback\n");
                    break;
            }

            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            writelog(NOTICE, "Data received\n");
            writelog(USER, "Response = %s\n", (char *)in);

            switch(state)
            {
                case WAIT_GET_CONFIG_RESPONSE:
                    // Copy the received response to the command string
                    // for later processing.
                    strcpy(command + LWS_PRE, (char *)in);
                    change_state(GET_CONFIG_RESPONSE_RECEIVED);
                    // Ask for a callback when the server can accept the SetConfig command
                    lws_callback_on_writable(wsi);
                    break;
                case WAIT_SET_CONFIG_RESPONSE:
                    // The config update procedure ended. 
                    // State is updated to wait for the next rate change.
                    // Service is canceled and connection is closed.
                    change_state(WAIT_RATE_CHANGE);
                    lws_cancel_service(context);
                    lws_close_reason(wsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);
                    writelog(USER, "Config update success.\n");
		    // Re-enable catching of signal
		    signal_control(ENABLE); 
                    return(-1);
                default:
                    writelog(WARN, "Config update abort due to unforeseen data received\n");
                    change_state(WAIT_RATE_CHANGE);
                    lws_cancel_service(context);
                    lws_close_reason(wsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);
		    // Re-enable catching of SIGHUP signal
		    signal_control(ENABLE); 
                    return (-1);
            }

            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            // Connection ended with error.
            writelog(ERR, "Callback: Connection to server %s on port %d failed.\n", server_address, server_port);
	    change_state(WAIT_RATE_CHANGE);
	    break;
        case LWS_CALLBACK_CLIENT_CLOSED:
            // Connection closed by client.
            writelog(NOTICE, "Callback: LWS_CALLBACK_CLIENT_CLOSED\n");
	    break;
        case LWS_CALLBACK_CLOSED:
            writelog(WARN, "Callback: LWS_CALLBACK_CLOSED\n");
            // Connection closed by server.
            // A new connection is requested and the config update process is restarted.
	    // TO BE MANAGED AND TESTED IN FUTURE VERSION.
            // change_state(RECONNECTION_REQUEST);
	    change_state(WAIT_RATE_CHANGE);
            break;
        default:
            writelog(NOTICE, "Unmanaged Callback. Reason = %d\n", reason);
            break;
    }

    // Re-enable catching of SIGHUP signal
    signal_control(ENABLE);

    return 0;
}


/////////////////////////////////////////////
// Attempt connection to websocket server
/////////////////////////////////////////////
int connection_request()
{
    int count;

    for (count = 0; count < MAX_CONNECT; count++)
    {
        websocket = lws_client_connect_via_info(&connect_info);

        if (websocket)
            return(SUCCESS);
        else
            sleep(0.1);
    }

    return(FAIL);
}


/////////////////////////////////////////////
// Change global state 
/////////////////////////////////////////////
void change_state(enum states newstate)
{
    char str_newstate[40];

    strcpy(str_newstate, decode_state(newstate));

    writelog(NOTICE, "State changed to %s\n", str_newstate);

    state = newstate;


    return;
}


/////////////////////////////////////////////
// Write log messages to stderr or syslog 
// depending on the --syslog option
/////////////////////////////////////////////
void writelog(int level, char *format, ...)
{
    static char buff[BUFLEN + 30];
    va_list args;

    sprintf(buff, "%s: ", decode_state(state));
    va_start(args, format);
    vsprintf(buff + strlen(buff), format, args);
    buff[CUTLEN - 2] = '\n';
    buff[CUTLEN - 1] = '\0';
    va_end(args);

    _lws_log(level, buff);
}
