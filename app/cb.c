#include "pjsua.h"

int cnt_calls = 0;

void connect_conf_cb(void *user_data) {
  pjsua_conf_port_id *call_slot = (pjsua_call_id *) user_data;
  if(*call_slot != FREE) {
    pjsua_conf_connect(conf_slot[1], *call_slot);
    pjsua_schedule_timer2(&disconnect_conf_cb, user_data, 1800);
  }
}

void disconnect_conf_cb(void *user_data) {
  pjsua_conf_port_id *call_slot = (pjsua_call_id *) user_data;
  if(*call_slot != FREE) {
    pjsua_conf_disconnect(conf_slot[1], *call_slot);
    pjsua_schedule_timer2(&connect_conf_cb, user_data, 5000);
  }
}

void timer_hangup_callback(pj_timer_heap_t *ht, pj_timer_entry *e)
{
    pjsua_call_id *call_id = (pjsua_call_id *) e->user_data; 
    if (*call_id != FREE) {
      pjsua_call_hangup(*call_id, 200, NULL, NULL);
    }
}

void timer_callback2(void *user_data)
{
    pjsua_call_id *call_id = (pjsua_call_id *) user_data;
    pjsua_call_answer(*call_id, 200, NULL, NULL);
}


/* Callback called by the library upon receiving incoming call */
void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id,
pjsip_rx_data *rdata) {
  
  pjsua_call_info ci;

  PJ_UNUSED_ARG(acc_id);
  PJ_UNUSED_ARG(rdata);

  pjsua_call_get_info(call_id, &ci);

  PJ_LOG(3,(THIS_FILE, "Incoming call from %.*s!!",
  (int)ci.remote_info.slen,
  ci.remote_info.ptr));


  
  u_int8_t is_404 = 1;
  for (int i = 0; i < user_cnt; i++) {
    
    //if(pj_strcmp(&acc[i].uri, &ci.local_info) == 0) {
    /*dont cmp 1st & last chars of local_info (< >)*/
    if(pj_strncmp2(&acc[i].uri, ci.local_info.ptr + 1, acc[i].uri.slen - 2) == 0) {
      if(cnt_calls < MAX_ONCALL) {
        cnt_calls++;
        /*search for free slot in call table*/
        u_int8_t table_slot;
        for (table_slot = 0; table_slot < MAX_ONCALL; table_slot++) {
            if(call_info[table_slot].call_id == FREE) {
              call_info[table_slot].call_id = call_id;
              call_info[table_slot].user_id = i;
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
void on_call_state(pjsua_call_id call_id, pjsip_event *e) {

  pjsua_call_info ci;

  PJ_UNUSED_ARG(e);
  
  pjsua_call_get_info(call_id, &ci);
  if(ci.state == PJSIP_INV_STATE_DISCONNECTED) {
    for(u_int8_t table_slot = 0; table_slot < MAX_ONCALL; table_slot++) {
      if(call_info[table_slot].call_id == call_id) {
        /*free slot in call_info table*/
        pjsua_cancel_timer(&call_info[table_slot].timer_entry);
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
void on_call_media_state(pjsua_call_id call_id) {
    
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
      //if(pj_strcmp(&ci.local_info, &cmp_name[0]) == 0) {
      if(acc[call_info[table_slot].user_id].action == TONE) {
        /*tonegen connect*/
        pjsua_conf_connect(conf_slot[0], ci.conf_slot);
      }
      if(acc[call_info[table_slot].user_id].action == WAV) {  
        /*wav connect*/
        pjsua_conf_connect(wav_slot, ci.conf_slot);
      }
      if(acc[call_info[table_slot].user_id].action == BOTH) {
       /*tonegen and wav connect*/
        pjsua_conf_connect(wav_slot, ci.conf_slot);
        pjsua_conf_connect(conf_slot[0], ci.conf_slot);
      }
    }
}


/*static void timer_hangup_callback(void *user_data)
{
    pjsua_call_id *call_id = (pjsua_call_id *) user_data;
      
    if (*call_id != FREE) {
      pjsua_call_hangup(*call_id, 200, NULL, NULL);
    }

}*/


/*template callback pjsua_schedule_timer*/
/*static void timer_callback(pj_timer_heap_t *ht, pj_timer_entry *e)
{
    pjsua_call_id *call_id = (pjsua_call_id *) e->user_data; 
    pjsua_call_answer(*call_id, 200, NULL, NULL);
}*/