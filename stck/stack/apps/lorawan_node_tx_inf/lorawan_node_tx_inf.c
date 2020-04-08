/* * OSS-7 - An opensource implementation of the DASH7 Alliance Protocol for ultra
 * lowpower wireless sensor communication
 *
 * Copyright 2015 University of Antwerp
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

Tested on B-L072Z-LRWAN1 (Murata) / OCTA (Murata)
CHANGE SETTINGS:

TX power: line 106
LoRaMAC/Mac/region/RegionEU868.h

Change freq band: line 346
LoRaMAC/Mac/region/RegionEU868.c

Change BW: line 297
LoRaMAC/Mac/region/RegionEU868.h

Default Frequency: 867.1 MHz 
*/

#include "hwleds.h"
#include "scheduler.h"
#include "timer.h"
#include "log.h"
#include "lorawan_stack.h"
#include "debug.h"
#include "hwwatchdog.h"
#include "console.h"
#include "platform.h"


/**** START CONFIG PARAMETERS ****/
#define ADR_ENABLED    0    // Enable/disable adaptive datarate
#define DATA_RATE     2   // LoRa datarate if ADR is disabled (0 - 5) SF 12 - 7

// DEFINE PLATFORM:

// #define OCTA
#define B_L072Z_LRWAN1
/**** END CONFIG PARAMETERS ****/



#if DATA_RATE == 0
  #define SENSOR_INTERVAL_SEC TIMER_TICKS_PER_SEC*2 
#else
  #define SENSOR_INTERVAL_SEC TIMER_TICKS_PER_SEC
#endif
#define SPEED    0   // 0: slow, 1: fast (+/- 10 times)




// ABP config, no interference
#define LORAWAN_DEVICE_ADDR           0x260111DF
#define LORAWAN_NETW_SESSION_KEY      { 0xB0, 0xD1, 0x02, 0xE2, 0xDA, 0xA0, 0xDC, 0xD0, 0xEE, 0x5C, 0x61, 0x94, 0x7C, 0xE0, 0x08, 0xCB }
#define LORAWAN_APP_SESSION_KEY       { 0x1C, 0xFA, 0x6D, 0xEE, 0x0D, 0x76, 0xCC, 0x63, 0x4D, 0x28, 0xF0, 0x9C, 0x40, 0x63, 0x9E, 0xD0 }

uint8_t msg_counter = 0;

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

void read_sensor_task()
{
  TX_leds_init();
  TX_leds();
  static uint8_t buffer[1] = {0xAB}; // payload of 1 byte
  lorawan_stack_status_t e = lorawan_stack_send(&buffer, 1, 2, false);

 // lorawan_stack_status_t e = lorawan_stack_send(&msg_counter, 1, 2, false);
  if(e == LORAWAN_STACK_ERROR_OK) {
    msg_counter++;
      #if SPEED == 0
        timer_post_task_delay(&read_sensor_task, SENSOR_INTERVAL_SEC);
      #elif SPEED == 1
        timer_post_task_delay(&read_sensor_task, SENSOR_INTERVAL_SEC/10);
      #endif
       
     TX_leds();
    //  timer_post_task_delay(&TX_leds, SENSOR_INTERVAL_SEC); // bug fix (recieve only SF12)
  }
}

void bootstrap()
{
    lorawan_stack_init_abp(&lorawan_session_config_abp);
    sched_register_task(&read_sensor_task);
    timer_post_task_delay(&read_sensor_task, SENSOR_INTERVAL_SEC);
}