 
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
/*
This code sends in an infinite loop (jammer)

Tested on B-L072Z-LRWAN1
SETTINGS:

Change TX power: line 106
LoRaMAC/Mac/region/RegionEU868.h

Change freq band: line 346
LoRaMAC/Mac/region/RegionEU868.c
*/

#include "log.h"
#include "lorawan_stack.h"
#include "scheduler.h"
#include "timeServer.h"
#include "timer.h"
#include "string.h"


/**** START CONFIG PARAMETERS ****/
#define ADR_ENABLED    0   // Enable/disable adaptive datarate
#define DATA_RATE      2 // LoRa datarate if ADR is disabled (0 - 5) SF 12 - 7

// Define platform

//#define OCTA
#define B_L072Z_LRWAN1
/**** END CONFIG PARAMETERS ****/


#if DATA_RATE == 0
  #define SENSOR_INTERVAL_SEC TIMER_TICKS_PER_SEC // SF 12
#elif DATA_RATE == 2
  #define SENSOR_INTERVAL_SEC TIMER_TICKS_PER_SEC/4 // SF 10
#elif DATA_RATE == 3 
  #define SENSOR_INTERVAL_SEC TIMER_TICKS_PER_SEC/8 // SF 9
#else
  #define SENSOR_INTERVAL_SEC TIMER_TICKS_PER_SEC/10 // SF 7 ok
#endif



// ABP config, no interference
#define LORAWAN_DEVICE_ADDR           0x260111DF
#define LORAWAN_NETW_SESSION_KEY      { 0xB0, 0xD1, 0x02, 0xE2, 0xDA, 0xA0, 0xDC, 0xD0, 0xEE, 0x5C, 0x61, 0x94, 0x7C, 0xE0, 0x08, 0xCB }
#define LORAWAN_APP_SESSION_KEY       { 0x1C, 0xFA, 0x6D, 0xEE, 0x0D, 0x76, 0xCC, 0x63, 0x4D, 0x28, 0xF0, 0x9C, 0x40, 0x63, 0x9E, 0xD0 }

static lorawan_session_config_abp_t lorawan_session_config_abp = {
    .appSKey = LORAWAN_APP_SESSION_KEY,
    .nwkSKey = LORAWAN_NETW_SESSION_KEY,
    .devAddr = LORAWAN_DEVICE_ADDR,
    .request_ack = false,
    .application_port = 1,
    .network_id = 0x00,
    .adr_enabled = ADR_ENABLED,
    .data_rate = DATA_RATE
};

void TX_leds_init()
{
    #if defined OCTA 
        led_on(0);
        led_on(1);  
        led_on(2);
        led_on(3);
    #endif
}

void TX_leds()
{ 
        led_toggle(0);
    #if defined OCTA 
    #elif defined B_L072Z_LRWAN1	
	    led_toggle(2);
    #endif
}

void read_sensor_task() {
  
      TX_leds_init(); 
     TX_leds();
//  timer_post_task_delay(&led_toggle_callback, SENSOR_INTERVAL_SEC);
//  static uint8_t msg_counter = 0;
//  uint8_t buffer[1] = {0xAB}; // payload of 4 bytes
       // timer_post_task_delay(&led_toggle_callback, SENSOR_INTERVAL_SEC);
   // lorawan_stack_status_t e = lorawan_stack_send(&buffer, 1, 2, false);
    //msg_counter++;
      
      static uint8_t msg_counter = 0;
      static uint8_t buffer[1] = {0xAB}; // payload of 1 byte
      static uint8_t buffer2[1] = {0xCD}; // payload of 1 byte

      lorawan_stack_status_t e = lorawan_stack_send(&buffer, 1, 2, false);
   
      timer_post_task_delay(&TX_leds, SENSOR_INTERVAL_SEC*2); // bug fix (recieve only SF12)
      lorawan_stack_status_t e2 = lorawan_stack_send(&buffer2, 1, 2, false); // bug fix (rec only SF12) 
    
      msg_counter++;
      
       //timer_post_task_delay(&TX_leds, SENSOR_INTERVAL_SEC*2); // DR 4, 5; SF 7, 8
      // timer_post_task_delay(&TX_leds, SENSOR_INTERVAL_SEC); // DR 3; SF 9
      //timer_post_task_delay(&TX_leds, SENSOR_INTERVAL_SEC/2); // DR 2; SF 10
      //timer_post_task_delay(&TX_leds, SENSOR_INTERVAL_SEC/4); // DR 1; SF 11
      timer_post_task_delay(&TX_leds, SENSOR_INTERVAL_SEC/8); // DR 0; SF 12
      
 //   timer_post_task_delay(&led_toggle_callback, SENSOR_INTERVAL_SEC);
   // timer_post_task_delay(&read_sensor_task, 60*SENSOR_INTERVAL_SEC;
}

void lorawan_status_cb(lorawan_stack_status_t status, uint8_t attempt) {}

void bootstrap()
{
    //lorawan_register_cbs(lorawan_rx, lorawan_tx, lorawan_status_cb);
    //lorawan_register_cbs(LORAWAN_STACK_ERROR_OK, true, lorawan_status_cb);
   
 //  lorawan_register_cbs(lorawan_rx, lorawan_tx, lorawan_status_cb);
   //lorawan_register_cbs(0, true, lorawan_status_cb);
  // lorawan_stack_init_abp(&lorawan_session_config_abp);
    
    //read_sensor_task();
  //  led_toggle_callback(); // when line is disabled, LEDs commented when NOT sending!
    
    //sched_register_task(&read_sensor_task);  
    //sched_post_task(&read_sensor_task);   
   // timer_post_task_delay(&read_sensor_task, SENSOR_INTERVAL_SEC);
    //sched_register_task(&led_toggle_callback);

    log_print_string("Device booted\n");
    lorawan_register_cbs(0, true, lorawan_status_cb);
    lorawan_stack_init_abp(&lorawan_session_config_abp);

    sched_register_task(&read_sensor_task);
    timer_post_task_delay(&read_sensor_task, SENSOR_INTERVAL_SEC);
    sched_register_task(&TX_leds);
}