#include "pjsua.h" 
 
pjsua_conf_port_id conf_slot;

pj_time_val delay;

//pj_timer_entry *entry;
pj_pool_t *pool;
pj_timer_heap_t *timer;

int cnt_calls = 0;

call_info_table call_info[MAX_CALLS];

void call_treatment(int table_slot) {
  pjsua_call_answer(call_info[table_slot].call_id, 180, NULL, NULL);
  pjsua_schedule_timer2(&timer_callback2, (void *)&call_info[table_slot].call_id, 2000);
  pjsua_schedule_timer2(&timer_hangup_callback, (void *)&call_info[table_slot].call_id, 4000);
}

static void timer_hangup_callback(void *user_data)
{
    pjsua_call_id *call_id = (pjsua_call_id *) user_data;
    printf("!!!!!!!!!!!!*****************hangup_call_id = %d*****************!!!!!!!!!!!!!\n\n\n", *call_id);
    /*if(*call_id > MAX_CALLS) {
      printf("!!!!!!!!!!!!*****************CANCEL*****************!!!!!!!!!!!!!");
      return;
    }*/
    
    //printf("*****************hangup_call_id = %d = %d = %d*****************", *call_id, &call_id, call_id);
    
    
      
    pjsua_call_hangup(*call_id, 200, NULL, NULL);
    *call_id = FREE;
    cnt_calls--;
    //pjsua_call_hangup_all();
    //pjsua_cancel_timer(e);
    //cnt_entries--;
}

/*static void timer_callback(pj_timer_heap_t *ht, pj_timer_entry *e)
{
    pjsua_call_id *call_id = (pjsua_call_id *) e->user_data;
    printf("*****************TCB_call_id = %d*****************\n\n\n", *call_id);
    if(*call_id > MAX_CALLS) {
      printf("!!!!!!!!!!!!*****************CANCEL*****************!!!!!!!!!!!!!\n\n\n");
      return;
    }
    pjsua_call_answer(*call_id, 200, NULL, NULL);
    //pjsua_cancel_timer(e);
    //cnt_entries--;
}*/

static void timer_callback2(void *user_data)
{
    pjsua_call_id *call_id = (pjsua_call_id *) user_data;
    //printf("*****************TCB_call_id = %d*****************", *call_id);
    pjsua_call_answer(*call_id, 200, NULL, NULL);
    //pjsua_schedule_timer2(timer_hangup_callback, call_id, 2000);
    //printf("*****************hangup_call_id = %d*****************", *call_id);
    //pjsua_call_hangup(*call_id, 200, NULL, NULL);
    //pjsua_call_hangup_all();
    //pjsua_cancel_timer(e);
    //cnt_entries--;
}



/* Callback called by the library upon receiving incoming call */
static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id,
pjsip_rx_data *rdata) {
  if(cnt_calls < PJSUA_MAX_CALLS){
  pjsua_call_info ci;

  PJ_UNUSED_ARG(acc_id);
  PJ_UNUSED_ARG(rdata);

  if(cnt_calls >= PJSUA_MAX_CALLS - 1) {
    pjsua_call_hangup(call_id, 200, NULL, NULL);
    return;
  }
  
  /*поиск свободного слота в таблице звонков*/
  int table_slot;
  for (table_slot = 0; table_slot < PJSUA_MAX_CALLS; table_slot++) {
      if(call_info[table_slot].call_id == FREE) {
        call_info[table_slot].call_id = call_id;
        cnt_calls++;
        break;
      }
  }
  

  pjsua_call_get_info(call_id, &ci);

  PJ_LOG(3,(THIS_FILE, "Incoming call from %.*s!!",
  (int)ci.remote_info.slen,
  ci.remote_info.ptr));

  printf("*****************OIC_call_id = %d*****************\n", call_id);
  call_treatment(table_slot);
  /* Automatically answer incoming calls with 200/OK */
  //pjsua_call_answer(call_id, 180, NULL, NULL);
  //pjsua_call_answer(call_id, 200, NULL, NULL);
    
  /*if (cnt_entries > 0) {
    delay.sec = 20;
  }*/
  //pjsua_call_answer(call_id, 200, NULL, NULL);
  //pjsua_schedule_timer2(&timer_callback2, (void *)&call_id, 2000);
  //pjsua_call_answer(call_id, 200, NULL, NULL);
  //pjsua_schedule_timer2(&timer_hangup_callback, &call_id, 4000);
  //pjsua_schedule_timer(&entry, &delay);
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
  pjsua_acc_id acc_id;
  pj_status_t status;

  /*инициализация таблицы информации о звонках*/
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

    /*20 calls at the same time*/
    cfg.max_calls = MAX_CALLS;
    //cfg.thread_cnt = 20;

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
  {
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
  }

  /**********************************************************************/
  /*create timer*/
  
    
    //pj_status_t status;
    /*pj_pool_factory *mem;
    pj_size_t size;
    pj_caching_pool cp;

    pj_caching_pool_init(&cp, NULL, 0);  
    mem = &cp.factory;

    size = pj_timer_heap_mem_size(MAX_ENTRIES) + MAX_ENTRIES * sizeof(pj_timer_entry);
    pool = pj_pool_create( mem, "timer", size, 4000, NULL);

    if (!pool) {
      PJ_LOG(3,("test", "...error: unable to create pool of %u bytes",
      size));
      return -10;
    }

    entry = (pj_timer_entry*)pj_pool_calloc(pool, MAX_ENTRIES, sizeof(*entry));
    if (!entry)
    return -20;

    int i;

    for (i = 0; i < MAX_ENTRIES ; ++i) {
      entry[i].cb = &timer_callback;
    }
    status = pj_timer_heap_create(pool, MAX_ENTRIES, &timer);
    if (status != PJ_SUCCESS) {
        printf("failed to create timer heap");
        return -30;
    }*/
  
  
  /**********************************************************************/



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