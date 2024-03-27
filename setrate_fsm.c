////////////////////////////////////////////////////
//
// camilladsp-setrate
// Automatic sample rate switcher for CamillaDSP
//
// setrate_fsm.c
// Finite-state Machine
//
////////////////////////////////////////////////////

#include <stddef.h>
#include <string.h>
#include <libwebsockets.h>
#include "setrate.h"

extern int rate;		// Current sample rate
extern int capture;		// capture option
extern int upsampling;		// upsampling option
extern int upsampling_factor;	// upsampling factor

// A cell of the transition table
typedef struct 
{
    int (*action)(void);  // Pointer to function to be called when a transition occurs
    states next_state;	  // Next state to be reached 
} cell;

// Transition table of the finite-state machine
static cell transition_table[LAST_STATE + 1][LAST_EVENT + 1]; 

states state;	// Global state

int fsm_ret;	// Value returned after a transition
		// of the Finite-State Machine

////////////////////////////////////////////////////////////
// Initialise the finite-state machine by assigning
// action function and next state in the transition table
////////////////////////////////////////////////////////////
void fsm_init()
{
    int i, j;

    for (i = FIRST_STATE; i <= LAST_STATE; i++)
	for (j = FIRST_EVENT; j <= LAST_EVENT; j++)
	{
	    transition_table[i][j].action = NULL;
	    transition_table[i][j].next_state = INIT;
	}

    transition_table[START][CONNECT].action			 = NULL;
    transition_table[START][CONNECT].next_state			 = WAIT_RATE_CHANGE;

    transition_table[START][DISCONNECT].action			 = reconnection_request;
    transition_table[START][DISCONNECT].next_state		 = START;

    transition_table[WAIT_RATE_CHANGE][RATE_CHANGE].action       = callback_on_writeable;
    transition_table[WAIT_RATE_CHANGE][RATE_CHANGE].next_state   = RATE_CHANGED;

    transition_table[WAIT_RATE_CHANGE][DISCONNECT].action        = reconnection_request;
    transition_table[WAIT_RATE_CHANGE][DISCONNECT].next_state    = START;

    transition_table[RATE_CHANGED][WRITEABLE].action             = send_get_config;
    transition_table[RATE_CHANGED][WRITEABLE].next_state         = WAIT_CONFIG;

    transition_table[RATE_CHANGED][DISCONNECT].action	         = reconnection_request;
    transition_table[RATE_CHANGED][DISCONNECT].next_state	 = START;

    transition_table[WAIT_CONFIG][CONFIG_OK].action              = callback_on_writeable;
    transition_table[WAIT_CONFIG][CONFIG_OK].next_state          = CONFIG_RECEIVED_OK;

    transition_table[WAIT_CONFIG][CONFIG_KO].action              = callback_on_writeable;
    transition_table[WAIT_CONFIG][CONFIG_KO].next_state          = CONFIG_RECEIVED_KO;

    transition_table[WAIT_CONFIG][DISCONNECT].action	         = reconnection_request;
    transition_table[WAIT_CONFIG][DISCONNECT].next_state	 = START;

    transition_table[CONFIG_RECEIVED_OK][WRITEABLE].action       = send_set_config;
    transition_table[CONFIG_RECEIVED_OK][WRITEABLE].next_state   = WAIT_RESPONSE;

    transition_table[CONFIG_RECEIVED_OK][DISCONNECT].action	 = reconnection_request;
    transition_table[CONFIG_RECEIVED_OK][DISCONNECT].next_state	 = START;

    transition_table[CONFIG_RECEIVED_KO][WRITEABLE].action       = send_get_previous_config;
    transition_table[CONFIG_RECEIVED_KO][WRITEABLE].next_state   = WAIT_PREVIOUS_CONFIG;

    transition_table[CONFIG_RECEIVED_KO][DISCONNECT].action	 = reconnection_request;
    transition_table[CONFIG_RECEIVED_KO][DISCONNECT].next_state	 = START;

    transition_table[WAIT_PREVIOUS_CONFIG][CONFIG_OK].action     = callback_on_writeable;
    transition_table[WAIT_PREVIOUS_CONFIG][CONFIG_OK].next_state = CONFIG_RECEIVED_OK;

    transition_table[WAIT_PREVIOUS_CONFIG][CONFIG_KO].action     = notify_failure;
    transition_table[WAIT_PREVIOUS_CONFIG][CONFIG_KO].next_state = WAIT_RATE_CHANGE;

    transition_table[WAIT_PREVIOUS_CONFIG][DISCONNECT].action	 = reconnection_request;
    transition_table[WAIT_PREVIOUS_CONFIG][DISCONNECT].next_state= START;

    transition_table[WAIT_RESPONSE][RESPONSE_OK].action          = notify_success;
    transition_table[WAIT_RESPONSE][RESPONSE_OK].next_state      = WAIT_RATE_CHANGE;
 
    transition_table[WAIT_RESPONSE][RESPONSE_KO].action          = notify_failure;
    transition_table[WAIT_RESPONSE][RESPONSE_KO].next_state      = WAIT_RATE_CHANGE;

    transition_table[WAIT_RESPONSE][DISCONNECT].action		 = reconnection_request;
    transition_table[WAIT_RESPONSE][DISCONNECT].next_state	 = START;

    writelog(NOTICE, "Finite-state machine initialised\n");

    // Set the initial state
    state = START;
}


/////////////////////////////////////////////////////////
// Trigger a transition of the finite-state machine
// depending on the current state and the incoming event
// (Mealy machine)
/////////////////////////////////////////////////////////
int fsm_transit(events event)
{
    int (*action)();
    states next_state;
    int ret;
  
    // Sanity checks
    if (state < FIRST_STATE || state > LAST_STATE)
    {
	writelog(ERR, "Fatal error: State %d out of range\n", state);
	exit(FAIL);
    }

    if (event < FIRST_EVENT || event > LAST_EVENT)
    {
	writelog(ERR, "Fatal error: Event %d out of range\n", event);
	exit(FAIL);
    }

    // Pick next state in the transition table
    next_state = transition_table[state][event].next_state;

    // Pick action function in the transition table
    action = transition_table[state][event].action;

    // If next state is INIT there is nothing to be done
    if (next_state == INIT)
	return(SUCCESS);
   
    writelog(NOTICE, "Event     : %s\n", decode_event(event));
    writelog(NOTICE, "Action    : %s\n", decode_action(action));
    writelog(NOTICE, "Next State: %s\n", decode_state(next_state));

    // Carry out the action, if any
    if (action)
	ret = action();
    else
	ret = SUCCESS;

    // Update the current state
    state = next_state;
    
    return(ret);
}


//////////////////////////////////////////////
//////////////////////////////////////////////
//// Actions of the finite-state machine  ////
//////////////////////////////////////////////
//////////////////////////////////////////////


///////////////////////////////////////////////
// Notify success of the configuration update
//////////////////////////////////////////////
int notify_success(void)
{
    if (capture)
	writelog(USER, "END       : capture_samplerate set to %d\n", rate);
    else if (upsampling)
	writelog(USER, "END       : capture_samplerate set to %d, samplerate set to %d\n", rate, rate*upsampling_factor);
    else
	writelog(USER, "END       : samplerate set to %d\n", rate);

    return(SUCCESS);
}


//////////////////////////////////////////////
// Notify failure of the configuration update
//////////////////////////////////////////////
int notify_failure(void)
{
    writelog(WARN, "END       : Configuration update abort\n");

    return(SUCCESS);
}



