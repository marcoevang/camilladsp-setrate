////////////////////////////////////////////////////
// camilladsp-setrate
// Automatic sample rate switcher for CamillaDSP
//
// setrate_util.c
// Utility functions
//
////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "setrate.h"

extern int capture;


/////////////////////////////////////////////////////////////
// Check if server replied to GetConfig 
// or GetPreviousConfig or SetConfig, 
// and if the received data indicates success or failure.
// Return the detected event.
/////////////////////////////////////////////////////////////
events check_received_data(char *received_data) 
{
    static char tag_response_ok[] = "{\"SetConfig\":{\"result\":\"Ok\"}}";
    static char tag_config_ko[] = "\\n~\\n";

    // Search for the tag identifying a response to 
    // GetConfig or GetProeviousConfig
    if (strstr(received_data, "GetConfig") || strstr(received_data, "GetPreviousConfig"))
    {
	// Search for the tag indicating that the received
	// configuration is not valid
	if (strstr(received_data, tag_config_ko))
	    return(CONFIG_KO);
	else
	    return(CONFIG_OK);
    }
	
    // Search for the tag identifying a response to SetConfig
    if (strstr(received_data, "SetConfig"))
    {
	if (strstr(received_data, tag_response_ok))
	    return(RESPONSE_OK);
	else
	    return(RESPONSE_KO);
    }

    // Received data is unforeseen
    return(-1);
}


///////////////////////////////////////
// Prepare the SetConfig command to be 
// issued to websocket server
///////////////////////////////////////
int prepare_setconfig_command(char *orig, int samplerate) 
{
    char *p1;      // points to the start of the config 
    char *p2;      // points to the start of the samplerate or capture_samplerate tag
    char *p3;      // points to the rest of the config after the samplerate value
    char *p4;      // points to the start of the chunksize tag
    char *p5;      // points to the rest of the config after the  changes
    char *tmpbuf;  // temporary buffer
    int len_tag_samplerate;   // length of the samplerate tag to search for
    int len_tag_chunksize;   // length of the chunksize tag to search for
    int len_tag_capture_samplerate;   // length of the capture_samplerate tag to search for
    int chunksize;
    static char tag_samplerate[] = "samplerate:";
    static char tag_chunksize[]  = "chunksize:";
    static char tag_capture_samplerate[] = "capture_samplerate:";
    char str_samplerate[8];
    char str_chunksize[8];
    int len;
    int n;

    // sanity check
    if (!orig || samplerate <= 0)
        return(FAIL);

    sprintf(str_samplerate, "%d", samplerate);

    tmpbuf = malloc(BUFLEN);

    if (!tmpbuf)
        return(FAIL);

    strcpy(tmpbuf, "{\"SetConfig\": \"");

    p1 = strstr(orig, "---") + 5;

    if (!p1)
        return(FAIL);

    if (!capture)
    {
	// the --capture flag was not used:
	// samplerate and chunksize parameters will be updated

	if (samplerate <= 48000)
	    chunksize = 1024;
	else if (samplerate <= 96000)
	    chunksize = 2048;
	else
	    chunksize = 4092;

	sprintf(str_chunksize, "%d", chunksize);

	len_tag_samplerate         = strlen(tag_samplerate);
	len_tag_chunksize          = strlen(tag_chunksize);

	p2 = strstr(orig, tag_samplerate);

	if (!p2)
	    return(FAIL);

	p3 = strchr(p2, '\\');

	if (!p3)
	    return(FAIL);

	p4 = strstr(orig, tag_chunksize);

	if (!p4)
	    return(FAIL);

	p5 = strchr(p4, '\\');

	if (!p5)
	    return(FAIL);

	n = p2 - p1 + len_tag_samplerate + 1;

	strncat(tmpbuf, p1, n);
	strcat(tmpbuf, str_samplerate);

	n = p4 - p3 + len_tag_chunksize + 1;

	strncat(tmpbuf, p3, n);
	strcat(tmpbuf, str_chunksize);
	strcat(tmpbuf, p5);

	len = strlen(tmpbuf);
	tmpbuf[len - 1] = '\0';
    }
    else
    {
	// the --capture flag was used:
	// capture_samplerate parameter only will be updated

	len_tag_capture_samplerate = strlen(tag_capture_samplerate);

	p2 = strstr(orig, tag_capture_samplerate);

	if (!p2)
	    return(FAIL);

	p3 = strchr(p2, '\\');

	if (!p3)
	    return(FAIL);

	n = p2 - p1 + len_tag_capture_samplerate + 1;

	strncat(tmpbuf, p1, n);
	strcat(tmpbuf, str_samplerate);

	strcat(tmpbuf, p3);

	len = strlen(tmpbuf);
	tmpbuf[len - 1] = '\0';

    }

    strcpy(orig, tmpbuf);

    free(tmpbuf);

    return(SUCCESS);
}


///////////////////////////////////////
// Convert numeric state into
// a human-readable string
///////////////////////////////////////
char *decode_state(states state)
{
    switch(state)
    {
        case INIT:
            return("INIT");
        case START:
            return("START");
        case WAIT_RATE_CHANGE:
            return("WAIT_RATE_CHANGE");
        case RATE_CHANGED:
            return("RATE_CHANGED");
        case WAIT_CONFIG:
            return("WAIT_CONFIG");
        case CONFIG_RECEIVED_OK:
            return("CONFIG_RECEIVED_OK");
        case CONFIG_RECEIVED_KO:
            return("CONFIG_RECEIVED_KO");
        case WAIT_PREVIOUS_CONFIG:
            return("WAIT_PREVIOUS_CONFIG");
        case WAIT_RESPONSE:
            return("WAIT_RESPONSE");
        default:
            return("UNKNOWN STATE");
    }
}


///////////////////////////////////////
// Convert numeric event into
// a human-readable string
///////////////////////////////////////
char *decode_event(events event)
{
    switch(event)
    {
        case CONNECT:
            return("CONNECT");
        case DISCONNECT:
            return("DISCONNECT");
        case RATE_CHANGE:
            return("RATE_CHANGE");
        case WRITEABLE:
	    return("WRITEABLE");
        case CONFIG_OK:
	    return("CONFIG_OK");
        case CONFIG_KO:
	    return("CONFIG_KO");
        case RESPONSE_OK:
	    return("RESPONSE_OK");
        case RESPONSE_KO:
	    return("RESPONSE_KO");
        default:
            return("UNKNOWN EVENT");
    }
}


///////////////////////////////////////
// Convert callback pointer into
// a human-readable string
///////////////////////////////////////
char *decode_action(int (*action)())
{
    if (action == callback_on_writeable)
	return("callback_on_writeable");
    else if (action == send_get_config)
	return("send_get_config");
    else if (action == send_get_previous_config)
	return("send_get_previous_config");
    else if (action == send_set_config)
	return("send_set_config");
    else if (action == notify_success)
	return("notify_success");
    else if (action == notify_failure)
	return("notify_failure");
    else if (action == reconnection_request)
	return("reconnection_request");
    else if (action == NULL)
	return("NULL");
    else
	return("Unknown action function");
}
