 
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
This code allows you to change:
- Frequency
- SF
- TX power

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
#define DATA_RATE      2  // LoRa datarate if ADR is disabled (0 - 5) SF 12 - 7

// DEFINE PLATFORM:
// #define OCTA
#define B_L072Z_LRWAN1

#define SEND_SEQ  0  // 0: enable specified times Tx, 8: enable infinite Tx        
#define SPEED    0   // 0: slow, 1: fast (+/- 10 times)

#if SEND_SEQ == 0
    static uint8_t send_x_times = 5;
#endif
/**** END CONFIG PARAMETERS ****/


#if DATA_RATE == 1
  #define SENSOR_INTERVAL_SEC TIMER_TICKS_PER_SEC // SF 12
#elif DATA_RATE == 0 || DATA_RATE == 2
  #define SENSOR_INTERVAL_SEC TIMER_TICKS_PER_SEC/2 // SF 10
#elif DATA_RATE == 3 
  #define SENSOR_INTERVAL_SEC TIMER_TICKS_PER_SEC/6 // SF 9
#else
  #define SENSOR_INTERVAL_SEC TIMER_TICKS_PER_SEC/10 // SF 7 ok
#endif





static uint8_t counter = 1;
static uint8_t msg_counter = 0;
static uint8_t send_x_times = 10;
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
    #if defined OCTA 
    led_toggle(0);
    #elif defined B_L072Z_LRWAN1	
      led_toggle(0);
	    led_toggle(2); 
    #endif
}

void read_sensor_task() {

      static uint8_t buffer[1] = {0xAB}; // payload of 1 byte
 
    #if SEND_SEQ == 0
        
         TX_leds();
          lorawan_stack_status_t e = lorawan_stack_send(&msg_counter, 1, 2, false);
          
        if(e == LORAWAN_STACK_ERROR_OK) {
          if (counter < (send_x_times)){ 
            
            lorawan_stack_status_t e2 = lorawan_stack_send(&msg_counter, 1, 2, false);
            timer_post_task_delay(&read_sensor_task, SENSOR_INTERVAL_SEC);
            msg_counter++;
            counter++; 
            TX_leds();
             
            //timer_post_task_delay(&TX_leds, SENSOR_INTERVAL_SEC); 
          }
        }    
      
    #elif SEND_SEQ == 8
         // TX_leds_init();
         // TX_leds();
          
          lorawan_stack_status_t e = lorawan_stack_send(&msg_counter, 1, 2, false);

        // lorawan_stack_status_t e = lorawan_stack_send(&msg_counter, 1, 2, false);
          if(e == LORAWAN_STACK_ERROR_OK) {
            if (counter < (send_x_times)){ 
            msg_counter++;
            counter++; 
              #if SPEED == 0
                timer_post_task_delay(&read_sensor_task, SENSOR_INTERVAL_SEC);
              #elif SPEED == 1
                timer_post_task_delay(&read_sensor_task, SENSOR_INTERVAL_SEC/10);
              #endif
              
            TX_leds();
            //  timer_post_task_delay(&TX_leds, SENSOR_INTERVAL_SEC); // bug fix (recieve only SF12)
          }}

      #elif SEND_SEQ == 1  
      lorawan_stack_status_t e = lorawan_stack_send(&buffer, 1, 2, false);
     // timer_post_task_delay(&TX_leds, SENSOR_INTERVAL_SEC*2); // bug fix (recieve only SF12)
      timer_post_task_delay(&TX_leds, SENSOR_INTERVAL_SEC*2); // bug fix (recieve only SF12)
      lorawan_stack_status_t e2 = lorawan_stack_send(&buffer2, 1, 2, false); // bug fix (rec only SF12)    
      msg_counter++;
      timer_post_task_delay(&TX_leds, SENSOR_INTERVAL_SEC/8); // DR 0; SF 12

    #endif



 //   timer_post_task_delay(&led_toggle_callback, SENSOR_INTERVAL_SEC);
   // timer_post_task_delay(&read_sensor_task, 60*SENSOR_INTERVAL_SEC;
}

void lorawan_status_cb(lorawan_stack_status_t status, uint8_t attempt) {}

void bootstrap()
{
    log_print_string("Device booted\n");
    lorawan_register_cbs(0, true, lorawan_status_cb);
    lorawan_stack_init_abp(&lorawan_session_config_abp);

    sched_register_task(&read_sensor_task);
    timer_post_task_delay(&read_sensor_task, SENSOR_INTERVAL_SEC);
    //sched_register_task(&TX_leds);
    TX_leds();
}