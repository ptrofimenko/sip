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

/*duration of ringing in msec*/
#define RINGING_DURATION 	2000

#include <unistd.h>
#include <pjsua-lib/pjsua.h>

#endif