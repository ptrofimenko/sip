#ifndef __MAIN_H__
#define __MAIN_H__

#define THIS_FILE 			"APP"
 
#define SIP_DOMAIN 			"example.com"
#define SIP_USER 			"alice"
#define SIP_PASSWD 			"secret"

#define SAMPLES_PER_FRAME   64
#define ON_DURATION	    	1000
#define OFF_DURATION	    0

#define MAX_ENTRIES			64
#define MAX_CALLS			20

/*duration of ringing in msec*/
#define RINGING_DURATION 	2000
/*hangup timer*/
#define ONCALL_DURATION		4000

/*free slot value in calls table*/
#define FREE -10
/*answer means callee is busy*/
#define BUSY 486


#include <unistd.h>
#include <pjsua-lib/pjsua.h>

typedef struct {
	pjsua_call_id call_id;
} call_info_table;

void call_treatment(int table_slot);
static void timer_hangup_callback(void *user_data);
static void timer_callback2(void *user_data);

#endif