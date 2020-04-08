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
 *  Created on: Mar 24, 2015
 *  Authors:
 *  	glenn.ergeerts@uantwerpen.be
 */


#include <string.h>
#include <stdio.h>

#include <hwleds.h>
#include <hwradio.h>
#include <log.h>
#include <hwwatchdog.h>
#include "crc.h"
#include "timer.h"





#define PHY_CLASS
#define PACKET_LENGTH 10

hw_tx_cfg_t tx_cfg = {
    .channel_id = {
        .channel_header.ch_coding = PHY_CODING_PN9,
        .channel_header.ch_class = PHY_CLASS_LO_RATE,
        .channel_header.ch_freq_band = PHY_BAND_868,
        .center_freq_index = 80
    },
    .syncword_class = PHY_SYNCWORD_CLASS1,
    .eirp = 10
};

static uint8_t tx_buffer[sizeof(hw_radio_packet_t) + 255] = { 0 };
hw_radio_packet_t* tx_packet = (hw_radio_packet_t*)tx_buffer;
static uint8_t data[256];

static uint8_t counter = 0;
static void packet_transmitted(hw_radio_packet_t* packet);


void transmit_packet()
{
    log_print_string("%d tx %d bytes\n", counter, PACKET_LENGTH);
    memcpy(tx_packet->data, data, PACKET_LENGTH);
    memcpy(tx_packet->data + 1, &counter, 1);
    uint16_t crc = __builtin_bswap16(crc_calculate(tx_packet->data, tx_packet->length + 1 - 2));
    memcpy(tx_packet->data + tx_packet->length + 1 - 2, &crc, 2);

    hw_radio_send_packet(tx_packet, &packet_transmitted, 0, NULL);
	
    counter++;

    if(counter >= 20)
      while(1);
}

static void packet_transmitted(hw_radio_packet_t* packet)
{
    log_print_string("%d tx ok\n", counter);

    if(counter == 10) {
      // switch to LoRa and introduce a gap
      tx_packet->tx_meta.tx_cfg.channel_id.channel_header.ch_class = PHY_CLASS_LORA;
      timer_post_task_delay(&transmit_packet, TIMER_TICKS_PER_SEC);
    } else {
      sched_post_task(&transmit_packet);
    }

    hw_watchdog_feed();
}

void bootstrap()
{
    data[0] = PACKET_LENGTH-1;
    int i = 1;
    for (;i<PACKET_LENGTH;i++)
    	data[i] = i;

    hw_radio_init(NULL, NULL);

    tx_packet->tx_meta.tx_cfg = tx_cfg;
    sched_register_task(&transmit_packet);
    sched_post_task(&transmit_packet);
}