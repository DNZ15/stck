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
#include <hwwatchdog.h>
#include "lorawan_stack.h"
#include "scheduler.h"
#include "timeServer.h"
#include "timer.h"
#include "string.h"
#include "platform.h"
#include "button.h"
#include "hwleds.h"
#include "hwsystem.h"
#include "inc/lora_gps.h"

void led_off_callback()
{
	led_off(status_led);
}

void led_on_callback()
{
  led_toggle(status_led);
  timer_post_task_delay(&led_off_callback, TIMER_TICKS_PER_SEC*0.050);
	hw_watchdog_feed();
}

void send_message(uint8_t length){
    lorawan_stack_status_t e = lorawan_stack_send(&transmit_buffer, length, 2, false);
    if(e == LORAWAN_STACK_ERROR_OK) {
      log_print_string("TX ok");
    } 
    else {
      log_print_string("TX failed");
      status_led = 2;
    }
    #ifdef USE_LEDS
    led_on_callback();
    #endif
    hw_watchdog_feed();
}
void send_test_message(){
  log_print_string("sending test message");
  uint8_t test_message[] = {0x01, 0x02, 0x03, 0x04};
  uint8_t* ptr = transmit_buffer;
  uint8_t len;
  status_led = 0;
  memcpy(ptr, &test_message, 4); ptr += 4;
  len = ptr - transmit_buffer;
  send_message(len);
}



void read_sensor_task() {
  hw_watchdog_feed();
  uint8_t* ptr = transmit_buffer;
  uint8_t len;

  #ifdef PERIODIC_MSGS
    timer_post_task_prio_delay(&read_sensor_task, tx_delay_sec  * TIMER_TICKS_PER_SEC, DEFAULT_PRIORITY);
  #endif
}

void on_join_completed(bool success,uint8_t app_port,bool request_ack) {
  hw_watchdog_feed();
  if(!success) {
    log_print_string("join failed");
  } 
  else {
    log_print_string("join succeeded");

    #ifdef PERIODIC_MSGS
      timer_post_task_prio_delay(&read_sensor_task, tx_delay_sec  * TIMER_TICKS_PER_SEC, DEFAULT_PRIORITY);
    #endif
  }
}
void lorwan_rx(lorawan_AppData_t *AppData)
{
   log_print_string("RECEIVED DATA");
}
void lorwan_tx(bool error){
  hw_watchdog_feed();
   log_print_string("data transmitted");
}

void feed_watchdog(){
    hw_watchdog_feed();
    timer_post_task_delay(&feed_watchdog, TIMER_TICKS_PER_SEC * 5);
}

void button_msg(){
  hw_watchdog_feed();
  uint8_t* ptr = transmit_buffer;
  uint8_t len;
}

void userbutton_callback(button_id_t button_id){
	log_print_string("Button PB%u pressed.", button_id);
  button_msg();
  hw_watchdog_feed();
  //send_test_message();
}

void bootstrap(){
  reboot_reason = hw_system_reboot_reason();
  log_print_string("Device booted\n");
  ubutton_register_callback(0, &userbutton_callback);
  sched_register_task(&led_on_callback);
  sched_register_task(&led_off_callback);
  led_off(0);
  led_off(1);
  led_off(2);

    log_print_string("init LoRaWAN ABP");
    lorawan_register_cbs(lorwan_rx,lorwan_tx,on_join_completed);
    lorawan_stack_init_abp(&lorawan_session_config_abp);
  

  sched_register_task(&read_sensor_task);


  sched_register_task(&feed_watchdog);
  __watchdog_init();
  feed_watchdog();
}