#ifndef __MAIN_H__
#define __MAIN_H__

#define THIS_FILE 			"APP"
#define WAVE_FILE 			"Sound.wav"
 
#define SIP_DOMAIN 			"10.25.72.55"
#define SIP_USER1 			"alice"
#define SIP_USER2 			"bob"
#define SIP_USER3           "boblice"
#define SIP_PASSWD 			"secret"

#define NUMBER_OF_USERS		3

#define SAMPLES_PER_FRAME   64
#define ON_DURATION	    	1000
#define OFF_DURATION	    3000

#define MAX_ENTRIES			64
#define MAX_CALLS			20

/*duration of ringing in msec*/
#define RINGING_DURATION 	2000
/*hangup timer*/
#define ONCALL_DURATION		8000

/*free slot value in calls table*/
#define FREE -10
/*answer means callee is busy*/
#define BUSY 486
#define URI_NOT_FOUND 404

/*for sprintf*/
#include <stdio.h>	
#include <string.h>
#include <unistd.h>
#include <pjsua-lib/pjsua.h>

typedef struct {
	pjsua_call_id call_id;
} call_info_table;

static void call_treatment(int table_slot);
static void timer_hangup_callback(void *user_data);
static void timer_callback2(void *user_data);
static void acc_add(char acc_name[], pjsua_acc_id *acc_id);
static void error_exit(const char *title, pj_status_t status);
static void create_wav_port();

#endif