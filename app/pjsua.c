#include "pjsua.h" 



void create_tonegen_port(u_int8_t port_num) {

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

void create_wav_port() {
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

void acc_add(pj_str_t acc_uri, pjsua_acc_id *acc_id) {
  pj_status_t status;

  pjsua_acc_config cfg;
  
  pjsua_acc_config_default(&cfg);
  cfg.register_on_acc_add = PJ_FALSE;
  //cfg.id = pj_str("sip:" acc_name "@" SIP_DOMAIN);
  cfg.id = acc_uri;
  cfg.reg_uri = pj_str("sip:" SIP_DOMAIN);
 
  status = pjsua_acc_add(&cfg, PJ_TRUE, acc_id);
  if (status != PJ_SUCCESS) error_exit("Error adding account", status);
  
}

void call_treatment(int table_slot) {
  pjsua_call_answer(call_info[table_slot].call_id, 180, NULL, NULL);
  /*accept(200) timer*/
  pjsua_schedule_timer2(&timer_callback2, (void *)&call_info[table_slot].call_id, RINGING_DURATION);
  /*hangup timer*/
  pj_time_val delay;

  delay.sec = 0;
  delay.msec = RINGING_DURATION + ONCALL_DURATION;

  call_info[table_slot].timer_entry.cb = &timer_hangup_callback;
  call_info[table_slot].timer_entry.user_data = (void *)&call_info[table_slot].call_id;

  pjsua_schedule_timer(&call_info[table_slot].timer_entry, &delay);
  //pjsua_schedule_timer2(&timer_hangup_callback, (void *)&call_info[table_slot].call_id, RINGING_DURATION + ONCALL_DURATION);
}
 
/* Display error and exit application */
void error_exit(const char *title, pj_status_t status) {
  
  pjsua_perror(THIS_FILE, title, status);
  pjsua_destroy();
  exit(1);
}

void read_config_file(char *argv[], int argc, pj_ssize_t *size, char *config_str) {
  
  pj_oshandle_t file;

  pj_status_t status;

  if (argc > 1) {
    status = pj_file_open(pool, argv[1], PJ_O_RDONLY, &file);
  }
  else {
    status = pj_file_open(pool, CONFIG_FILE, PJ_O_RDONLY, &file);
  }
  if (status != PJ_SUCCESS) error_exit("Error in pj_file_open()", status);

  status = pj_file_read(file, (void *) config_str, size);
  pj_file_close(file);
}



void init_cdr_xml_tree()
{
  pj_xml_node *node;

  /*node names*/
  call_info_str = pj_str("call_info");
  calling_str = pj_str("calling");
  called_str = pj_str("called");
  duration_str = pj_str("duration");
  /* cdr xml tree structure
  <call_info>
    <calling></calling>
    <called></called>
    <duration></duration>
  </call_info>
  */  
  for (int i = 0; i < MAX_ONCALL; i++) {
    call_info[i].root = pj_xml_node_new(pool, &call_info_str);
    node = pj_xml_node_new(pool, &calling_str);
    pj_xml_add_node(call_info[i].root, node);
    node = pj_xml_node_new(pool, &called_str);
    pj_xml_add_node(call_info[i].root, node);
    node = pj_xml_node_new(pool, &duration_str);
    pj_xml_add_node(call_info[i].root, node);
  }
}
/*
 * main()
 *
 * argv[1] may contain URL to call.
 */
int main(int argc, char *argv[]) {

  pj_status_t status;
  pjsua_acc_id acc_id[USER_LIMIT];

  /*init call table*/
  {
    for (int i = 0; i < MAX_ONCALL; i++) {
      call_info[i].call_id = FREE;
    }
  }

  /* Create pjsua first! */
  status = pjsua_create();

  pj_caching_pool_init(&cp, &pj_pool_factory_default_policy, 0);

  pool = pj_pool_create( &cp.factory,     /* pool factory     */
       "pool",     /* pool name.     */
       4000,      /* init size      */
       4000,      /* increment size     */
       NULL       /* callback on error    */
  );

  
  if (status != PJ_SUCCESS) error_exit("Error in pjsua_create()", status);

  pj_ssize_t size = sizeof(char) * 5000;

  char *config_str = pj_pool_alloc(pool, size);
  read_config_file(argv, argc, &size, config_str);

  pj_xml_node *root, *node;
  pj_xml_attr *attr;

  root = pj_xml_parse(pool, config_str, size);
  if(root == NULL) error_exit("Bad xml config file", -1);
  pj_str_t node_s = pj_str("general");
  pj_str_t name = pj_str("log-level");

  node = pj_xml_find_node_rec(root, &node_s);
  if(node == NULL) error_exit("config node not found", -1);
  attr = pj_xml_find_attr(node, &name, NULL);
  if(attr == NULL) error_exit("config attribute not found", -1);


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
    log_cfg.console_level = pj_strtol(&attr->value);

    pjsua_media_config_default(&med_cfg);

    status = pjsua_init(&cfg, &log_cfg, &med_cfg);
    if (status != PJ_SUCCESS) error_exit("Error in pjsua_init()", status);
  }

  pj_str_t cdr = pj_str("collect-cdr");
  attr = pj_xml_find_attr(node, &cdr, NULL);
  if (attr != NULL) {
    if(pj_strcmp2(&attr->value, "yes") == 0) {
      collect_cdr = 1;
    }
    else { collect_cdr = 0; }
  }
  else { collect_cdr = 0; }

  if(collect_cdr == 1) {
    init_cdr_xml_tree();
    mkdir(CDR_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
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


    /*register defined users*/
  node_s = pj_str("acc");
  name = pj_str("username");
  pj_str_t action = pj_str("action");

  if(node == NULL) error_exit("config node not found", -1);

  for(user_cnt = 0; user_cnt < USER_LIMIT; user_cnt++) {
    /*search for 1st acc in xml tree*/
    if(user_cnt == 0) { node = pj_xml_find_node_rec(root, &node_s); }
    else { 
      //if(node->next == NULL) {
      if(pj_strcmp(&node->next->name, &node->name) != 0) {
        /*no more accs in list*/
        break;
      }
      acc[user_cnt].id = user_cnt;
      node = node->next;
    }
    attr = pj_xml_find_attr(node, &name, NULL);
      
    if(attr == NULL) { 
      
       error_exit("acc name attribute not set", -1);
    }
    
    /*duplicate caller name from xml tree*/
    pj_strdup(pool, &acc[user_cnt].uri, &attr->value);

    /*parse action for user*/
    attr = pj_xml_find_attr(node, &action, NULL);
    if(attr == NULL || pj_strcmp2(&attr->value, "play_tone") == 0) {
      acc[user_cnt].action = TONE;
    }
    else {
      if(pj_strcmp2(&attr->value, "play_file") == 0) {
        acc[user_cnt].action = WAV;   
      }
      else if(pj_strcmp2(&attr->value, "play_both") == 0) {
        acc[user_cnt].action = BOTH; 
      }
      else {
        acc[user_cnt].action = TONE;
      }
    }
    
    acc_add(acc[user_cnt].uri, &acc_id[user_cnt]);
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
    
  
  long int time = PJ_TIME_VAL_MSEC(call_info[0].end);
  /* Wait until user press "q" to quit. */
  for (;;) {
    char option[10];
    puts("Press 'h' to hangup all calls, 'q' to quit");
    
    if (fgets(option, sizeof(option), stdin) == NULL) {
      puts("EOF while reading stdin, will quit now..");
      break;
    }

    if (option[0] == 'q') {
      pjsua_call_hangup_all();
      break;
    }

    if (option[0] == 'h')
      pjsua_call_hangup_all();

    time = PJ_TIME_VAL_MSEC(call_info[0].end);
    
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
  pjmedia_endpt_destroy(med_endpt_wav);
  pj_pool_release(pool_wav);
  pj_caching_pool_destroy(&cp_wav);


  /*release app pool*/
  pj_pool_release(pool);
  pj_caching_pool_destroy(&cp);

  pjsua_destroy();
  
  return 0;
}