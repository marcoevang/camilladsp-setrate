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

extern int rate;   // Global state


///////////////////////////////////////
// Prepare the command to be issued
// to websocket server
///////////////////////////////////////
int prepare_setconfig_command(char *orig, int samplerate) 
{
    char *p1;      // points to the start of the received config 
    char *p2;      // points to the start of the samplerate tag
    char *p3;      // points to the rest of the config after the samplerate value
    char *p4;      // points to the start of the chunksize tag
    char *p5;      // points to the rest of the config after the chunksize value
    char *tmpbuf;  // temporary buffer
    int len_tag_samplerate;   // length of the samplerate tag to search for
    int len_tag_chunksize;   // length of the chunksize tag to search for
    int chunksize;
    static char tag_samplerate[] = "samplerate:";
    static char tag_chunksize[]  = "chunksize:";
    static char tag_none[]       = "\\n~\\n";
    char str_samplerate[8];
    char str_chunksize[8];
    int len;
    int n;

    // sanity checks
    if (!orig || !tag_samplerate || !tag_chunksize || !str_samplerate)
        return(FALSE);

    // Search for the tag identifying a None configuration
    tmpbuf = strstr(orig, tag_none);

    // none_tag is founf, config is None
    if (tmpbuf) 
        return(FALSE);

    if (samplerate <= 48000)
        chunksize = 1024;
    else if (rate <= 96000)
        chunksize = 2048;
    else
        chunksize = 4092;

    sprintf(str_samplerate, "%d", samplerate);
    sprintf(str_chunksize, "%d", chunksize);

    len_tag_samplerate = strlen(tag_samplerate);
    len_tag_chunksize = strlen(tag_chunksize);

    if (len_tag_samplerate == 0 || len_tag_chunksize == 0)
        return(FALSE);

    tmpbuf = malloc(BUFLEN);

    if (!tmpbuf)
        return(FALSE);

    strcpy(tmpbuf, "{\"SetConfig\": \"");

    p1 = strstr(orig, "---") + 5;
    p2 = strstr(orig, tag_samplerate);

    if (p2 == NULL)
        return(FALSE);

    p3 = strchr(p2, '\\');
    p4 = strstr(orig, tag_chunksize);

    if (p4 == NULL)
        return(FALSE);

    p5 = strchr(p4, '\\');

    n = p2 - p1 + len_tag_samplerate + 1;

    strncat(tmpbuf, p1, n);
    strcat(tmpbuf, str_samplerate);

    n = p4 - p3 + len_tag_chunksize + 1;

    strncat(tmpbuf, p3, n);
    strcat(tmpbuf, str_chunksize);
    strcat(tmpbuf, p5);

    len = strlen(tmpbuf);
    tmpbuf[len - 1] = '\0';

    strcpy(orig, tmpbuf);

    free(tmpbuf);

    return(TRUE);
}


///////////////////////////////////////
// Convert numeric state into
// a human-readable string
///////////////////////////////////////
char *decode_state(enum states state)
{
    static char str_state[30];

    switch(state)
    {
        case WAIT_RATE_CHANGE:
            //strcpy(str_state, "WAIT_RATE_CHANGE            ");
            strcpy(str_state, "            WAIT_RATE_CHANGE");
            break;
        case CONNECTION_REQUEST:
            //strcpy(str_state, "CONNECTION_REQUEST          ");
            strcpy(str_state, "          CONNECTION_REQUEST");
            break;
        case CONNECTED:
            //strcpy(str_state, "CONNECTED                   ");
            strcpy(str_state, "                   CONNECTED");
            break;
        case WAIT_GET_CONFIG_RESPONSE:
            //strcpy(str_state, "WAIT_GET_CONFIG_RESPONSE    ");
            strcpy(str_state, "    WAIT_GET_CONFIG_RESPONSE");
            break;
        case GET_CONFIG_RESPONSE_RECEIVED:
            strcpy(str_state, "GET_CONFIG_RESPONSE_RECEIVED");
            break;
        case WAIT_SET_CONFIG_RESPONSE:
            //strcpy(str_state, "WAIT_SET_CONFIG_RESPONSE    ");
            strcpy(str_state, "    WAIT_SET_CONFIG_RESPONSE");
            break;
        case RECONNECTION_REQUEST:
            strcpy(str_state, "       RECONNECTION_REQUEST");
            break;
        default:
            sprintf(str_state, "           UNKNOWN STATE %d", state);
            break;
    }

    return(str_state);
}


