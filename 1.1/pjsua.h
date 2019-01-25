#ifndef __MAIN_H__
#define __MAIN_H__

#define THIS_FILE 			"APP"
 
#define SIP_DOMAIN 			"example.com"
#define SIP_USER 			"alice"
#define SIP_PASSWD 			"secret"

#define SAMPLES_PER_FRAME   64
#define ON_DURATION	    	1000
#define OFF_DURATION	    0

#include <unistd.h>
#include <pjsua-lib/pjsua.h>

#endif