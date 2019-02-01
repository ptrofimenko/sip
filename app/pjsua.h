#ifndef __MAIN_H__
#define __MAIN_H__

#define THIS_FILE 			"APP"
#define WAVE_FILE 			"Sound.wav"
#define CONFIG_FILE			"config.xml" 

#define SIP_DOMAIN 			"10.25.72.55"
#define SIP_USER1 			"alice"
#define SIP_USER2 			"bob"
#define SIP_USER3           "boblice"
#define SIP_PASSWD 			"secret"

#define NUMBER_OF_USERS		3

#define SAMPLES_PER_FRAME   64
#define ON_DURATION	    	1000
#define OFF_DURATION	    3000
#define ON_DURATION_DIG	   	500
#define OFF_DURATION_DIG    100

#define NUM_OF_TONEGENS		2

#define MAX_ENTRIES			64
#define MAX_CALLS			PJSUA_MAX_CALLS
#define MAX_ONCALL			20

/*duration of ringing in msec*/
#define RINGING_DURATION 	2000
/*hangup timer*/
#define ONCALL_DURATION		4000

/*free slot value in calls table*/
#define FREE -5
/*answer means callee is busy*/
#define BUSY 486
#define URI_NOT_FOUND 404

/*for sprintf*/
#include <stdio.h>	
#include <string.h>
#include <unistd.h>
#include <pjsua-lib/pjsua.h>


pj_oshandle_t file;

pjsua_conf_port_id conf_slot[NUM_OF_TONEGENS];
pjsua_conf_port_id wav_slot;

pj_caching_pool cp;
pj_pool_t *pool;
pj_timer_heap_t *timer;

pj_caching_pool cp_tone[NUM_OF_TONEGENS], cp_wav;
pjmedia_endpt *med_endpt[NUM_OF_TONEGENS], *med_endpt_wav;
pj_pool_t *pool_tone[NUM_OF_TONEGENS], *pool_wav;
pjmedia_port *port[NUM_OF_TONEGENS], *port_wav;

pj_str_t cmp_name[NUMBER_OF_USERS];

typedef struct {
	pjsua_call_id call_id;
	pjsua_conf_port_id conf_slot;
	pj_timer_entry timer_entry;
} call_info_table;

call_info_table call_info[MAX_ONCALL];

void call_treatment(int table_slot);
void acc_add(char acc_name[], pjsua_acc_id *acc_id);
void error_exit(const char *title, pj_status_t status);
void create_wav_port();
void create_tonegen_port(u_int8_t port_num);
void create_tonegen_dig_port(u_int8_t port_num);


void timer_callback2(void *user_data);
void timer_hangup_callback(pj_timer_heap_t *ht, pj_timer_entry *e);
void disconnect_conf_cb(void *user_data);
void connect_conf_cb(void *user_data);
void on_incoming_call(pjsua_acc_id, pjsua_call_id, pjsip_rx_data *);
void on_call_state(pjsua_call_id, pjsip_event *);
void on_call_media_state(pjsua_call_id);

#endif