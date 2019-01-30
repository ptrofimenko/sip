#include "pjsua.h" 
 
pjsua_conf_port_id conf_slot;

pj_time_val delay;

pj_pool_t *pool;
pj_timer_heap_t *timer;

int cnt_calls = 0;

call_info_table call_info[MAX_CALLS];


 pj_str_t cmp_name[NUMBER_OF_USERS];
 

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
  pjsua_schedule_timer2(&timer_hangup_callback, (void *)&call_info[table_slot].call_id, ONCALL_DURATION);
}

static void timer_hangup_callback(void *user_data)
{
    pjsua_call_id *call_id = (pjsua_call_id *) user_data;
             
    pjsua_call_hangup(*call_id, 200, NULL, NULL);
    *call_id = FREE;
    cnt_calls--;
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
  
  printf("!!!!!!!!!!!!%s!!!!!!!!!!!!!\n\n\n", ci.local_info);

  PJ_LOG(3,(THIS_FILE, "Incoming call from %.*s!!",
  (int)ci.remote_info.slen,
  ci.remote_info.ptr));

  u_int8_t is_404 = 1;
  for (int i = 0; i < NUMBER_OF_USERS; i++) {
      
    if(pj_strcmp(&cmp_name[i], &ci.local_info) == 0) {
      if(cnt_calls < MAX_CALLS) {

        if(cnt_calls >= PJSUA_MAX_CALLS - 1) {
          pjsua_call_hangup(call_id, 200, NULL, NULL);
          return;
        }
  
        /*search for free slot in call table*/
        u_int8_t table_slot;
        for (table_slot = 0; table_slot < PJSUA_MAX_CALLS; table_slot++) {
            if(call_info[table_slot].call_id == FREE) {
              call_info[table_slot].call_id = call_id;
              cnt_calls++;
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
    }
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
  PJ_LOG(3,(THIS_FILE, "Call %d state=%.*s", call_id,
  (int)ci.state_text.slen,
  ci.state_text.ptr));
}

/* Callback called by the library when call's media state has changed */
static void on_call_media_state(pjsua_call_id call_id) {
    
    pjsua_call_info ci;
 
    pjsua_call_get_info(call_id, &ci);
 
    if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
      // When media is active, connect call to tonegen.
      pjsua_conf_connect(conf_slot, ci.conf_slot);
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
    for (i = 0; i < MAX_CALLS; i++) {
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
    log_cfg.console_level = 4;

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

 /* Register to SIP server by creating SIP account. */
  /*{
    pjsua_acc_config cfg;

    pjsua_acc_config_default(&cfg);
    cfg.id = pj_str("sip:" SIP_USER "@" SIP_DOMAIN);
    cfg.reg_uri = pj_str("sip:" SIP_DOMAIN);
    cfg.cred_count = 1;
    cfg.cred_info[0].realm = pj_str(SIP_DOMAIN);
    cfg.cred_info[0].scheme = pj_str("digest");
    cfg.cred_info[0].username = pj_str(SIP_USER);
    cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
    cfg.cred_info[0].data = pj_str(SIP_PASSWD);
 
    status = pjsua_acc_add(&cfg, PJ_TRUE, &acc_id);
    if (status != PJ_SUCCESS) error_exit("Error adding account", status);
  }*/

  cmp_name[0] = pj_str("<sip:" SIP_USER1 "@" SIP_DOMAIN ">");
  cmp_name[1] = pj_str("<sip:" SIP_USER2 "@" SIP_DOMAIN ">");
  cmp_name[2] = pj_str("<sip:" SIP_USER3 "@" SIP_DOMAIN ">");

  char *acc_name[NUMBER_OF_USERS] = {SIP_USER1, SIP_USER2, SIP_USER3};
  for(int i = 0; i < NUMBER_OF_USERS; i++) {
    acc_add(acc_name[i], &acc_id[i]);
  }
  //pjsua_set_null_snd_dev();


  pj_caching_pool cp_tone;
  pjmedia_endpt *med_endpt;
  pj_pool_t *pool_tone;
  pjmedia_port *port;
    
    
  pj_caching_pool_init(&cp_tone, &pj_pool_factory_default_policy, 0);


  status = pjmedia_endpt_create(&cp_tone.factory, NULL, 1, &med_endpt);
    //J_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /* Create memory pool for our file player */
  pool_tone = pj_pool_create( &cp_tone.factory,     /* pool factory     */
      "app",     /* pool name.     */
      1000,      /* init size      */
      1000,      /* increment size     */
      NULL       /* callback on error    */
  );

  status = pjmedia_tonegen_create(pool_tone, 8000, 1, SAMPLES_PER_FRAME, 16, PJMEDIA_TONEGEN_LOOP, &port);
  //if (status != PJ_SUCCESS)
  //return 1;

  pjmedia_tone_desc tones[1];

  tones[0].freq1 = 425;
  tones[0].freq2 = 0;
  tones[0].on_msec = ON_DURATION;
  tones[0].off_msec = OFF_DURATION;
  
  status = pjmedia_tonegen_play(port, 1, tones, 0);
  
  delay.sec = 5;
  delay.msec = 0;


  /*add port for tonegen*/
  status = pjsua_conf_add_port(pool_tone, port, &conf_slot);
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
  
  /*delete tonegen port*/
  pjmedia_port_destroy(port);

  /*release app pool*/
  pj_pool_release(pool);
  pj_pool_release(pool_tone);

  /*delete endpoint*/
  pjmedia_endpt_destroy(med_endpt);

  /* delete pool */
  //pj_caching_pool_destroy(&cp);
  pj_caching_pool_destroy(&cp_tone);


  pjsua_destroy();
  pj_shutdown();

  return 0;
}