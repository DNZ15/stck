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


// This examples pushes sensor data to gateway(s) by manually constructing an ALP command with a file read result action
// (unsolicited message). The D7 session is configured to request ACKs. All received ACKs are printed.
// Temperature data is used as a sensor value, when a HTS221 is available, otherwise value 0 is used.
// You are able to send one, a sequence and even infinite messages.

/*
// BUG: TX power
Tested on B-L072Z-LRWAN1 (Murata) / OCTA (Murata) with LED
CHANGE SETTINGS:

TX power: not sure if this even works...
stack/Modules/d7ap/phy.c line 539

Change frequency:
stack/Modules/d7ap/phy.c line 450

Rate and freqband: 
stack/Modules/d7ap/phy.c line 142

Default frequency D7: 863 MHz
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hwleds.h"
#include "hwsystem.h"
#include "scheduler.h"
#include "timer.h"
#include "d7ap_fs.h"
#include "d7ap.h"
#include "alp_layer.h"
#include "dae.h"
#include "platform.h"


/**** START CONFIG PARAMETERS ****/
#define SEND_SEQ  10   // 10: enable specified times Tx, 8: enable infinite Tx        
#define SPEED   0   // 0: slow, 1: fast (+/- 10 times)

// define platform
#define B_L072Z_LRWAN1      
//#define OCTA    
/**** END CONFIG PARAMETERS ****/



#if SEND_SEQ == 10
    static uint8_t send_x_times = 10;
#endif

#if SPEED == 0
    //slow, every second, 10 seconds send time for 10 packages
    #define SENSOR_INTERVAL_SEC TIMER_TICKS_PER_SEC 
#elif SPEED == 1
    //fast, +/-2 seconds send time for 10 packages
    #define SENSOR_INTERVAL_SEC TIMER_TICKS_PER_SEC/10 
#endif
/**** CONFIG PARAMETERS ****/



#define SENSOR_FILE_ID           0x40
#define SENSOR_FILE_SIZE         2

static uint8_t counter = 1;


// Define the D7 interface configuration used for sending the ALP command on
static d7ap_session_config_t session_config = {
    .qos = {
        //.qos_resp_mode = SESSION_RESP_MODE_PREFERRED,
        .qos_resp_mode = SESSION_RESP_MODE_NO,
        .qos_retry_mode = SESSION_RETRY_MODE_NO
    },
    .dormant_timeout = 0,
    .addressee = {
        .ctrl = {
            .nls_method = AES_NONE,
            .id_type = ID_TYPE_NOID,
        },
        .access_class = 0x01,
        .id = 0
    }
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
	    led_toggle(2); // disable when OCTA 
    #endif
}

void execute_sensor_measurement()
{
  // first get the sensor reading ...
  int16_t temperature = 0; // in decicelsius. When there is no sensor, we just transmit 0 degrees
  temperature = __builtin_bswap16(temperature); // convert to big endian before transmission

  // Generate ALP command.
  // We will be sending a return file data action, without a preceding file read request.
  // This is an unsolicited message, where we push the sensor data to the gateway(s).

  // allocate a buffer and fifo to store the command
  uint8_t alp_command[128];
  fifo_t alp_command_fifo;
  fifo_init(&alp_command_fifo, alp_command, sizeof(alp_command));

  // add the return file data action
  alp_append_return_file_data_action(&alp_command_fifo, SENSOR_FILE_ID, 0, SENSOR_FILE_SIZE, (uint8_t*)&temperature);

  // and execute this
  alp_layer_execute_command_over_d7a(alp_command, fifo_get_size(&alp_command_fifo), &session_config);
  TX_leds_init();
  TX_leds();
}

void on_alp_command_completed_cb(uint8_t tag_id, bool success)
{   
    TX_leds();
    #if SEND_SEQ == 10
        if (counter < (send_x_times)){
            timer_post_task_delay(&execute_sensor_measurement, (SENSOR_INTERVAL_SEC));
            counter++;    
        }
    #elif SEND_SEQ == 8
        timer_post_task_delay(&execute_sensor_measurement, (SENSOR_INTERVAL_SEC));
    #endif
}

void on_alp_command_result_cb(d7ap_session_result_t result, uint8_t* payload, uint8_t payload_length)
{
    log_print_string("recv response @ %i dB link budget from:", result.link_budget);
    log_print_data(result.addressee.id, 8);
}

static alp_init_args_t alp_init_args;

void bootstrap()
{
      d7ap_init();

      alp_init_args.alp_command_completed_cb = &on_alp_command_completed_cb;
      alp_init_args.alp_command_result_cb = &on_alp_command_result_cb;
      alp_layer_init(&alp_init_args, false);

     sched_register_task(&execute_sensor_measurement);
     sched_post_task(&execute_sensor_measurement);
 }