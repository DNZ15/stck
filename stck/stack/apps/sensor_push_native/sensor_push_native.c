/* * OSS-7 - An opensource implementation of the DASH7 Alliance Protocol for ultra
 * lowpower wireless sensor communication
 *
 * Copyright 2017 University of Antwerp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "log.h"

#include "lorawan_stack.h"
#include "scheduler.h"
#include "timeServer.h"
#include "timer.h"
#include "string.h"

//uint8_t ADR_ENABLED =  0;   // Enable/disable adaptive datarate
#define ADR_ENABLED    0 
#define DATA_RATE      0   // LoRa datarate if ADR is disabled (0 - 5) SF 12 - 7

// ABP config, no interference
#define LORAWAN_DEVICE_ADDR           0x2F01F1FF
#define LORAWAN_NETW_SESSION_KEY      { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
#define LORAWAN_APP_SESSION_KEY       { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }

static lorawan_session_config_abp_t lorawan_session_config_abp = {
    .appSKey = LORAWAN_APP_SESSION_KEY,
    .nwkSKey = LORAWAN_NETW_SESSION_KEY,
    .devAddr = LORAWAN_DEVICE_ADDR,
    .request_ack = false,
    .network_id = 0x00,
    .adr_enabled = ADR_ENABLED,
    .data_rate = 5,
};

void read_sensor_task() {
  static uint8_t msg_counter = 0;

  for (i=0; i<20; i++) {
    lorawan_stack_status_t e = lorawan_stack_send(&msg_counter, 1, 2, false);
    msg_counter++;
    //timer_post_task_delay(&led_toggle_callback, SENSOR_INTERVAL_SEC/10);
    timer_post_task_delay(&read_sensor_task, TIMER_TICKS_PER_SEC);
  }

 
  
}

void lorawan_status_cb(lorawan_stack_status_t status, uint8_t attempt) {
  if(status == LORAWAN_STACK_JOIN_FAILED) {
    log_print_string("join failed");
    // ...
  } else if(status == LORAWAN_STACK_JOINED){
    log_print_string("join succeeded");
  }
}
void lorawan_rx(lorawan_AppData_t *AppData)
{
   log_print_string("RECEIVED DATA"); //TODO
}
void lorawan_tx(bool error)
{
   log_print_string("RECEIVED DATA"); //TODO
}

void bootstrap()
{
    log_print_string("Device booted\n");
    memcpy(&lorawan_session_config.devEUI,devEui ,8);
    memcpy(&lorawan_session_config.appEUI, appEui,8);
    memcpy(&lorawan_session_config.appKey,appKey,16);
    lorawan_register_cbs(lorawan_rx, lorawan_tx, lorawan_status_cb);
    lorawan_stack_init_otaa(&lorawan_session_config_abp);

    sched_register_task(&read_sensor_task);
    timer_post_task_delay(&read_sensor_task, TIMER_TICKS_PER_SEC);
}