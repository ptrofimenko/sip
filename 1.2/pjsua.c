#include "pjsua.h" 
 
pjsua_conf_port_id conf_slot;

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

  /* Automatically answer incoming calls with 200/OK */
  pjsua_call_answer(call_id, 180, NULL, NULL);
  pj_thread_sleep(10000);
  pjsua_call_answer(call_id, 200, NULL, NULL);
  //sleep(5);
  //pjsua_call_hangup(call_id, 200, NULL, NULL);


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

    pjsua_config_default(&cfg);
    cfg.cb.on_incoming_call = &on_incoming_call;
    cfg.cb.on_call_media_state = &on_call_media_state;
    cfg.cb.on_call_state = &on_call_state;

    pjsua_logging_config_default(&log_cfg);
    log_cfg.console_level = 4;

    status = pjsua_init(&cfg, &log_cfg, NULL);
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
 
  pjsua_set_null_snd_dev();

  pj_caching_pool cp;
  pjmedia_endpt *med_endpt;
  pj_pool_t *pool;
  pjmedia_port *port;
    
  pj_caching_pool_init(&cp, &pj_pool_factory_default_policy, 0);


  status = pjmedia_endpt_create(&cp.factory, NULL, 1, &med_endpt);
  //J_ASSERT_RETURN(status == PJ_SUCCESS, 1);

  /* Create memory pool for our file player */
  pool = pj_pool_create( &cp.factory,     /* pool factory     */
       "wav",     /* pool name.     */
       4000,      /* init size      */
       4000,      /* increment size     */
       NULL       /* callback on error    */
  );

  status = pjmedia_wav_player_port_create(  pool, /* memory pool      */
       WAVE_FILE,  /* file to play     */
       20, /* ptime.     */
       0,  /* flags      */
       0,  /* default buffer   */
       &port/* returned port    */
  );

  status = pjsua_conf_add_port(pool, port, &conf_slot);
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

  /*delete endpoint*/
  pjmedia_endpt_destroy(med_endpt);

  /* delete pool */
  pj_caching_pool_destroy(&cp);

  pjsua_destroy();
  pj_shutdown();

  return 0;
}