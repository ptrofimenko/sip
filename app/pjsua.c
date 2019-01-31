#include "pjsua.h" 
 
pjsua_conf_port_id conf_slot[NUM_OF_TONEGENS];
pjsua_conf_port_id wav_slot;

pj_pool_t *pool;
pj_timer_heap_t *timer;

pj_caching_pool cp_tone[NUM_OF_TONEGENS], cp_wav;
pjmedia_endpt *med_endpt[NUM_OF_TONEGENS], *med_endpt_wav;
pj_pool_t *pool_tone[NUM_OF_TONEGENS], *pool_wav;
pjmedia_port *port[NUM_OF_TONEGENS], *port_wav;

int cnt_calls = 0;

call_info_table call_info[MAX_ONCALL];


 pj_str_t cmp_name[NUMBER_OF_USERS];
 

static void connect_conf_cb(void *user_data) {
  pjsua_conf_port_id *call_slot = (pjsua_call_id *) user_data;
  if(*call_slot != FREE) {
    pjsua_conf_connect(conf_slot[1], *call_slot);
    pjsua_schedule_timer2(&disconnect_conf_cb, user_data, 1800);
  }
}

static void disconnect_conf_cb(void *user_data) {
  pjsua_conf_port_id *call_slot = (pjsua_call_id *) user_data;
  if(*call_slot != FREE) {
    pjsua_conf_disconnect(conf_slot[1], *call_slot);
    pjsua_schedule_timer2(&connect_conf_cb, user_data, 5000);
  }
}



static void create_tonegen_port(u_int8_t port_num) {

  pj_status_t status;

  pj_caching_pool_init(&cp_tone[port_num], &pj_pool_factory_default_policy, 0);
  status = pjmedia_endpt_create(&cp_tone[port_num].factory, NULL, 1, &med_endpt[port_num]);
 
  /* Create memory pool for our file player */
  pool_tone[port_num] = pj_pool_create( &cp_tone[port_num].factory,     /* pool factory     */
      "tone",     /* pool name.     */
      1000,      /* init size      */
      1000,      /* increment size     */
      NULL       /* callback on error    */
  );

  status = pjmedia_tonegen_create(pool_tone[port_num], 8000, 1, SAMPLES_PER_FRAME, 16, PJMEDIA_TONEGEN_LOOP, &port[port_num]);
}

static void create_wav_port() {
  pj_status_t status;

  pj_caching_pool_init(&cp_wav, &pj_pool_factory_default_policy, 0);

  status = pjmedia_endpt_create(&cp_wav.factory, NULL, 1, &med_endpt_wav);

  /* Create memory pool for our file player */
  pool_wav = pj_pool_create( &cp_wav.factory,     /* pool factory     */
       "wav",     /* pool name.     */
       4000,      /* init size      */
       4000,      /* increment size     */
       NULL       /* callback on error    */
  );

  status = pjmedia_wav_player_port_create(  pool_wav, /* memory pool      */
       WAVE_FILE,  /* file to play     */
       20, /* ptime.     */
       0,  /* flags      */
       0,  /* default buffer   */
       &port_wav/* returned port    */
  );

  status = pjsua_conf_add_port(pool_wav, port_wav, &wav_slot);
  pj_assert(status == PJ_SUCCESS);
}


static void acc_add(char acc_name[], pjsua_acc_id *acc_id) {
  pj_status_t status;

  pjsua_acc_config cfg;
  char *id = malloc(sizeof(char) * (strlen(acc_name) + strlen(SIP_DOMAIN) + 5));

  sprintf(id, "sip:%s@%s", acc_name, SIP_DOMAIN);

  pjsua_acc_config_default(&cfg);
  //cfg.id = pj_str("sip:" acc_name "@" SIP_DOMAIN);
  cfg.id = pj_str(id);
  cfg.reg_uri = pj_str("sip:" SIP_DOMAIN);
  cfg.cred_count = 1;
  cfg.cred_info[0].realm = pj_str(SIP_DOMAIN);
  cfg.cred_info[0].scheme = pj_str("digest");
  cfg.cred_info[0].username = pj_str(acc_name);
  cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
  cfg.cred_info[0].data = pj_str(SIP_PASSWD);
 
  status = pjsua_acc_add(&cfg, PJ_TRUE, acc_id);
  if (status != PJ_SUCCESS) error_exit("Error adding account", status);
  free(id);
}

static void call_treatment(int table_slot) {
  pjsua_call_answer(call_info[table_slot].call_id, 180, NULL, NULL);
  /*accept(200) timer*/
  pjsua_schedule_timer2(&timer_callback2, (void *)&call_info[table_slot].call_id, RINGING_DURATION);
  /*hangup timer*/
  //pjsua_schedule_timer2(&timer_hangup_callback, (void *)&call_info[table_slot].call_id, RINGING_DURATION + ONCALL_DURATION);
}

static void timer_hangup_callback(void *user_data)
{
    pjsua_call_id *call_id = (pjsua_call_id *) user_data;
      
    if (*call_id != FREE) {
      pjsua_call_hangup(*call_id, 200, NULL, NULL);
    }

}

/*template callback pjsua_schedule_timer*/
/*static void timer_callback(pj_timer_heap_t *ht, pj_timer_entry *e)
{
    pjsua_call_id *call_id = (pjsua_call_id *) e->user_data; 
    pjsua_call_answer(*call_id, 200, NULL, NULL);
}*/

static void timer_callback2(void *user_data)
{
    pjsua_call_id *call_id = (pjsua_call_id *) user_data;
    pjsua_call_answer(*call_id, 200, NULL, NULL);
}



/* Callback called by the library upon receiving incoming call */
static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id,
pjsip_rx_data *rdata) {
  
  pjsua_call_info ci;

  PJ_UNUSED_ARG(acc_id);
  PJ_UNUSED_ARG(rdata);

  pjsua_call_get_info(call_id, &ci);

  PJ_LOG(3,(THIS_FILE, "Incoming call from %.*s!!",
  (int)ci.remote_info.slen,
  ci.remote_info.ptr));


  u_int8_t is_404 = 1;
  for (int i = 0; i < NUMBER_OF_USERS; i++) {
      
    //if(pj_strcmp(&cmp_name[i], &ci.local_info) == 0) {
      if(cnt_calls < MAX_ONCALL) {
        cnt_calls++;
        /*search for free slot in call table*/
        u_int8_t table_slot;
        for (table_slot = 0; table_slot < MAX_ONCALL; table_slot++) {
            if(call_info[table_slot].call_id == FREE) {
              call_info[table_slot].call_id = call_id;
              break;
           }
        }

        /*treatment of incoming call*/
        call_treatment(table_slot);
      }
      /* if MAX_CALLS reached - answer 486 (BUSY) */
      else {
        pjsua_call_answer(call_id, BUSY, NULL, NULL);
      }
    is_404 = 0;
    break;
    //}
  }
  if(is_404) {
      pjsua_call_answer(call_id, URI_NOT_FOUND, NULL, NULL);
  }
}

/* Callback called by the library when call's state has changed */
static void on_call_state(pjsua_call_id call_id, pjsip_event *e) {

  pjsua_call_info ci;

  PJ_UNUSED_ARG(e);
  
  pjsua_call_get_info(call_id, &ci);
  if(ci.state == PJSIP_INV_STATE_DISCONNECTED) {
    for(u_int8_t table_slot = 0; table_slot < MAX_ONCALL; table_slot++) {
      if(call_info[table_slot].call_id == call_id) {
        /*free slot in call_info table*/
        call_info[table_slot].call_id = FREE;
        call_info[table_slot].conf_slot = FREE;
        cnt_calls--;
        break;
      }
    }
  }

  PJ_LOG(3,(THIS_FILE, "Call %d state=%.*s", call_id,
  (int)ci.state_text.slen,
  ci.state_text.ptr));
}

/* Callback called by the library when call's media state has changed */
static void on_call_media_state(pjsua_call_id call_id) {
    
    pjsua_call_info ci;

    pjsua_call_get_info(call_id, &ci);

    u_int8_t table_slot;
    for( table_slot = 0; table_slot < MAX_ONCALL; table_slot++) {
      if(call_info[table_slot].call_id == call_id) {
        call_info[table_slot].conf_slot = ci.conf_slot;
        break;
      }
    }
    
    /*dtmf digits connect to all accs*/
    pjsua_conf_connect(conf_slot[1], call_info[table_slot].conf_slot);
    pjsua_schedule_timer2(&disconnect_conf_cb, (void *)&call_info[table_slot].conf_slot, 1800);

    if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
      /*joining media sources based on account name*/
      if(pj_strcmp(&ci.local_info, &cmp_name[0]) == 0) {
        /*tonegen connect*/
        pjsua_conf_connect(conf_slot[0], ci.conf_slot);
      }
      if(pj_strcmp(&ci.local_info, &cmp_name[1]) == 0) {
        /*wav connect*/
        pjsua_conf_connect(wav_slot, ci.conf_slot);
      }
      if(pj_strcmp(&ci.local_info, &cmp_name[2]) == 0) {
        /*tonegen and wav connect*/
        pjsua_conf_connect(wav_slot, ci.conf_slot);
        pjsua_conf_connect(conf_slot[0], ci.conf_slot);
      }
    }
}
 
/* Display error and exit application */
static void error_exit(const char *title, pj_status_t status) {
  
  pjsua_perror(THIS_FILE, title, status);
  pjsua_destroy();
  exit(1);
}


/*
 * main()
 *
 * argv[1] may contain URL to call.
 */
int main(int argc, char *argv[]) {
  
  pj_status_t status;
  pjsua_acc_id acc_id[NUMBER_OF_USERS];

  /*init call table*/
  {
    int i;
    for (i = 0; i < MAX_ONCALL; i++) {
      call_info[i].call_id = FREE;
    }
  }
  /* Create pjsua first! */
  status = pjsua_create();
  if (status != PJ_SUCCESS) error_exit("Error in pjsua_create()", status);

  /* If argument is specified, it's got to be a valid SIP URL */
  if (argc > 1) {
    status = pjsua_verify_url(argv[1]);
    if (status != PJ_SUCCESS) error_exit("Invalid URL in argv", status);
  }

 /* Init pjsua */
  {
    pjsua_config cfg;
    pjsua_logging_config log_cfg;
    pjsua_media_config med_cfg;

    pjsua_config_default(&cfg);
    cfg.cb.on_incoming_call = &on_incoming_call;
    cfg.cb.on_call_media_state = &on_call_media_state;
    cfg.cb.on_call_state = &on_call_state;

    /*set max calls at the same time*/
    cfg.max_calls = MAX_CALLS;
    
    pjsua_logging_config_default(&log_cfg);
    log_cfg.console_level = 2;

    pjsua_media_config_default(&med_cfg);

    status = pjsua_init(&cfg, &log_cfg, &med_cfg);
    if (status != PJ_SUCCESS) error_exit("Error in pjsua_init()", status);
  }

 /* Add UDP transport. */
  {
    pjsua_transport_config cfg;

    pjsua_transport_config_default(&cfg);
    cfg.port = 5060;
    status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &cfg, NULL);
    if (status != PJ_SUCCESS) error_exit("Error creating transport", status);
  }

 /* Initialization is done, now start pjsua */
  status = pjsua_start();
  if (status != PJ_SUCCESS) error_exit("Error starting pjsua", status);

  cmp_name[0] = pj_str("<sip:" SIP_USER1 "@" SIP_DOMAIN ">");
  cmp_name[1] = pj_str("<sip:" SIP_USER2 "@" SIP_DOMAIN ">");
  cmp_name[2] = pj_str("<sip:" SIP_USER3 "@" SIP_DOMAIN ">");

  /*register defined users*/
  char *acc_name[NUMBER_OF_USERS] = {SIP_USER1, SIP_USER2, SIP_USER3};
  for(int i = 0; i < NUMBER_OF_USERS; i++) {
    acc_add(acc_name[i], &acc_id[i]);
  }
  /*wav doesn't work without it*/
  pjsua_set_null_snd_dev();
   
  /*create wav slot*/
  create_wav_port();

  pjmedia_tone_desc tones[1];

  /*tone config*/
  tones[0].freq1 = 425;
  tones[0].freq2 = 0;
  tones[0].on_msec = ON_DURATION;
  tones[0].off_msec = OFF_DURATION;

  /*create tone port*/
  create_tonegen_port(0);
  status = pjmedia_tonegen_play(port[0], 1, tones, 0);

  pjmedia_tone_digit digits[3];

  /*digits config*/
  digits[0].digit = '0';
  digits[0].on_msec = ON_DURATION_DIG;
  digits[0].off_msec = OFF_DURATION_DIG;

  digits[1].digit = '1';
  digits[1].on_msec = ON_DURATION_DIG;
  digits[1].off_msec = OFF_DURATION_DIG;

  digits[2].digit = '2';
  digits[2].on_msec = ON_DURATION_DIG;
  digits[2].off_msec = OFF_DURATION_DIG;

  /*create digits port*/
  create_tonegen_port(1);
  status = pjmedia_tonegen_play_digits(port[1], 3, digits, 0);
  PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);
    

  /*get tonegen slot*/
  status = pjsua_conf_add_port(pool_tone[0], port[0], &conf_slot[0]);
  pj_assert(status == PJ_SUCCESS);
   
  /*get digits slot*/
  status = pjsua_conf_add_port(pool_tone[1], port[1], &conf_slot[1]);
  pj_assert(status == PJ_SUCCESS);
    
  /* Wait until user press "q" to quit. */
  for (;;) {
    char option[10];
    puts("Press 'h' to hangup all calls, 'q' to quit");
    
    if (fgets(option, sizeof(option), stdin) == NULL) {
      puts("EOF while reading stdin, will quit now..");
      break;
    }

    if (option[0] == 'q')
    break;

    if (option[0] == 'h')
      pjsua_call_hangup_all();
  }  
  
  /*destroy tonegens*/
  for(int i = 0; i < NUM_OF_TONEGENS; i++) {
    pjmedia_port_destroy(port[i]);  
    pj_pool_release(pool_tone[i]);
    pjmedia_endpt_destroy(med_endpt[i]);
    pj_caching_pool_destroy(&cp_tone[i]);
  }

  /*destroy wav*/
  pjmedia_port_destroy(port_wav);  
  pj_pool_release(pool_wav);
  pjmedia_endpt_destroy(med_endpt_wav);
  pj_caching_pool_destroy(&cp_wav);


  /*release app pool*/
  pj_pool_release(pool);


  pjsua_destroy();
  pj_shutdown();

  return 0;
}