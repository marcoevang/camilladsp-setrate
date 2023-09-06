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
#include "setrate.h"

extern int rate;                      // Current sample rate 
extern enum states state;             // Global state


struct lws_context *context = NULL;  // Global websocket context

// WebSocket callback function
// static int websocket_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);
static int websocket_callback(struct lws *, enum lws_callback_reasons, void *, void *, size_t);

// WebSocket protocols
static struct lws_protocols protocols[] = 
{ 
    {
        "MyProtocol",            // Protocol name
        websocket_callback,      // Callback function
        0,                       // Per-session data size
        BUFLEN,                  // Receive buffer
        0,                       // ID number (reserved)
        NULL,                    // User data
        0                        // Attach count
    },
    { NULL, NULL, 0, 0 }         // Terminate the list
};

struct lws_context_creation_info info;             // libwebsocket context creation info
struct lws_client_connect_info connect_info;       // libwebsocket connection info
struct lws *websocket;                             // Pointer to websocket struct
const char *full_address = "ws://localhost:1234";  // URL of CamillaDSP Websocket Server

enum states state;                            // Previous global state
enum states prev_state;                       // Previous global state
char command[BUFLEN];                         // Global buffer for command string
char cutstr[CUTLEN + 1];                      // Cut string for state change trace
int config_is_ok = TRUE;                      // Flag indicating the received config is valid

void notice_state_change(void);               // Logs a state change at NOTICE level
int connection_request(void);                 // Attempt to connect to server


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
	lwsl_err("Failed to create libwebsockets context\n");
	exit(FAIL);
    }

    // Set up libwebsockets connection info
    memset(&connect_info, 0, sizeof(connect_info));
    connect_info.context = context;
    connect_info.address = "localhost";
    connect_info.port = 1234;
    connect_info.path = "/";
    connect_info.host = full_address;
    connect_info.origin = full_address;
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

    // Save current state to trace state change
    prev_state = state;

    // Handle callback depending on reason
    switch (reason)
    {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            lwsl_notice("%s: Connected to CamillaDSP Websocket Server\n", decode_state(state));

            if (state == CONNECTION_REQUEST || state == RECONNECTION_REQUEST)
            {
                state = CONNECTED;
                notice_state_change();

                // Ask for a callback when server can accept the GetConfig command
                lws_callback_on_writable(wsi);
            }

            break;
        case LWS_CALLBACK_CLIENT_WRITEABLE:
            lwsl_notice("%s: Client writeable\n", decode_state(state));

            switch(state)
            {
                case CONNECTED:
                    // The server can accept the GetConfigCommand.
                    strcpy(command + LWS_PRE, "\"GetConfig\"");
                    lwsl_user("%s: Command  = %s\n", decode_state(state), command + LWS_PRE);
                    state = WAIT_GET_CONFIG_RESPONSE;
                    notice_state_change();
                    lws_write(wsi, command + LWS_PRE, strlen(command + LWS_PRE), LWS_WRITE_TEXT);
                    break;
                case GET_CONFIG_RESPONSE_RECEIVED:
                    // The server can accept a command.
                    if (prepare_setconfig_command(command + LWS_PRE, rate) == TRUE)
                    {
                        // Received configuration is valid
                        config_is_ok = TRUE;
                        // Current config is valid. Send SetConfig command.
                        state = WAIT_SET_CONFIG_RESPONSE;
                        notice_state_change();
                    }
                    else
                    {
                        // Received configuration is None or invalid
                        if (config_is_ok == TRUE)
                        {
                            // Current config is None or invalid. Request previous config.
                            config_is_ok = FALSE;
                            lwsl_warn("%s: Current configuration is None or Invalid. Trying with previous config.\n", decode_state(state));
                            strcpy(command + LWS_PRE, "\"GetPreviousConfig\"");
                            state = WAIT_GET_CONFIG_RESPONSE;
                            notice_state_change();
                        }
                        else
                        {
                            // Current and previous configs are both None ore invalid. Config update process abort.
                            lwsl_warn("%s: Current and previous configurations are both None or Invalid. Config update process abort.\n\n", decode_state(state));
                            state = WAIT_RATE_CHANGE;
                            notice_state_change();
                            lws_cancel_service(context);
                            lws_close_reason(wsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);
                            return (-1);
                        }
                    }

                    strncpy(cutstr, command + LWS_PRE, CUTLEN);
                    lwsl_user("%s: Command  = %s\n", decode_state(state), cutstr);
                    lws_write(wsi, command + LWS_PRE, strlen(command + LWS_PRE), LWS_WRITE_TEXT);
                    break;
                default:
                    lwsl_notice("%s: Unforeseen state in LMS_CALLBACK_CLIENT_WRITABLE callback\n", decode_state(state));
                    break;
            }

            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            strncpy(cutstr, (char *)in, CUTLEN);
            lwsl_notice("%s: Data received\n", decode_state(state));
            lwsl_user("%s: Response = %s\n", decode_state(state), cutstr);

            switch(state)
            {
                case WAIT_GET_CONFIG_RESPONSE:
                    // Copy the received response to the command string
                    // for later processing.
                    strcpy(command + LWS_PRE, (char *)in);
                    state = GET_CONFIG_RESPONSE_RECEIVED;
                    notice_state_change();
                    // Ask for a callback when the server can accept the SetConfig command
                    lws_callback_on_writable(wsi);
                    break;
                case WAIT_SET_CONFIG_RESPONSE:
                    // The config update procedure ended. 
                    // State is updated to wait for the next rate change.
                    // Service is canceled and connection is closed.
                    state = WAIT_RATE_CHANGE;
                    notice_state_change();
                    lws_cancel_service(context);
                    lws_close_reason(wsi, LWS_CLOSE_STATUS_CLIENT_TRANSACTION_DONE, NULL, 0);
                    lwsl_user("%s: Config update success.\n\n", decode_state(state));
		    // Re-enable catching of signal
		    signal_control(ENABLE); 
                    return(-1);
                default:
                    lwsl_warn("%s: Process terminated due to unforeseen data received = %s: %s\n\n", decode_state(state), (char *)in);
                    state = WAIT_RATE_CHANGE;
                    notice_state_change();
                    lws_cancel_service(context);
                    lws_close_reason(wsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);
		    // Re-enable catching of SIGHUP signal
		    signal_control(ENABLE); 
                    return (-1);
            }

            break;
        case LWS_CALLBACK_CLOSED:
            lwsl_warn("%s: Connection terminated by the webSocket Server\n\n", decode_state(state));

            // The connection was terminated by server.
            // a new connection is requested and the config update process is restarted.
            state = RECONNECTION_REQUEST;
            notice_state_change();

            break;
        default:
            lwsl_notice("%s: Unmanaged Callback. Reason = %d\n", decode_state(state), reason);
            break;
    }

    // Re-enable catching of SIGHUP signal
    signal_control(ENABLE);

    return 0;
}


/////////////////////////////////////////////
// Logs global state change
/////////////////////////////////////////////
void notice_state_change()
{
    char str_prev_state[40];

    strcpy(str_prev_state, decode_state(prev_state));

    lwsl_notice("%s: State changed to %s\n", str_prev_state, decode_state(state));

    return;
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


