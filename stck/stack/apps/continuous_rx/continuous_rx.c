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

/*! \file
 * Test application which puts the radio in continous RX mode transmitting random data.
 * Usefull for measuring center frequency offset on a spectrum analyzer
 * Note: works only on cc1101 since this is not using the public hw_radio API but depends on cc1101 internal functions
 *
 *  Created on: Feb 27, 2017
 *  Authors:
 *  	ben.bellekens@uantwerpen.be
 */

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "hwsystem.h"
#include "platform.h"

#if !defined (USE_CC1101) && !defined (USE_SI4460) && !defined (USE_SX127X)
    #error "This application only works with cc1101, SI4460 or SX1276"
#endif

#include "log.h"
#include "hwradio.h"

#if PLATFORM_NUM_BUTTONS > 1
#include "button.h"
#endif


typedef enum {
  MODULATION_CW,
  MODULATION_GFSK,
} modulation_t;

#define HI_RATE_CHANNEL_COUNT 32
#define NORMAL_RATE_CHANNEL_COUNT 32
#define LO_RATE_CHANNEL_COUNT 280

static hw_rx_cfg_t rx_cfg;
static uint16_t current_channel_indexes_index = 13;
static uint8_t current_eirp_level = 0x7f;
static modulation_t current_modulation = MODULATION_GFSK;
static phy_coding_t current_coding = PHY_CODING_RFU;
static phy_channel_band_t current_channel_band = PHY_BAND_868;
static phy_channel_class_t current_channel_class = PHY_CLASS_NORMAL_RATE;
static uint16_t channel_indexes[LO_RATE_CHANNEL_COUNT] = { 0 }; // reallocated later depending on band/class
static uint16_t channel_count = NORMAL_RATE_CHANNEL_COUNT;

void stop_radio(){
#if defined USE_SI4460
    // stop receiving signal
    ezradio_change_state(EZRADIO_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY);
#elif defined USE_CC1101
    cc1101_interface_strobe(RF_SIDLE);
#endif
}

void start_radio(){
#if defined USE_SI4460

        /* start the device as receiver  */
        ezradio_change_state(EZRADIO_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY);
        hw_radio_set_rx(&rx_cfg, NULL, NULL);
#endif
        while (true) {
            int16_t rss = hw_radio_get_rssi();
            log_print_string("rss : %d \n", rss);
            hw_busy_wait(5000);
        }
}

void configure_radio(modulation_t mod){
#if defined USE_SI4460
    // Si4460 Direct mode
    ezradio_set_property(0x20, 0x01, 0x00, mod);

    //power level EIRP
    ezradio_set_property(0x22, 0x01, 0x01, current_eirp_level);

#elif defined USE_CC1101

    hw_radio_set_rx(&rx_cfg, NULL, NULL); // we 'misuse' hw_radio_set_rx to configure the channel (using the public API)
    hw_radio_set_idle(); // go straight back to idle

    /* Configure */
    cc1101_interface_write_single_patable(current_eirp_level);
    //cc1101_interface_write_single_reg(0x08, 0x22); // PKTCTRL0 random PN9 mode + disable data whitening
    cc1101_interface_write_single_reg(0x08, 0x22); // PKTCTRL0 disable data whitening, continious preamble
    cc1101_interface_write_single_reg(0x12, mod); // MDMCFG2
    cc1101_interface_strobe(0x32); // strobe calibrate
#elif defined USE_SX127X
    hw_radio_continuous_rx(&rx_cfg, 0);
#endif
}

void start()
{
    rx_cfg.channel_id.channel_header.ch_coding = current_coding;
    rx_cfg.channel_id.channel_header.ch_class = current_channel_class;
    rx_cfg.channel_id.channel_header.ch_freq_band = current_channel_band;
    rx_cfg.channel_id.center_freq_index = channel_indexes[current_channel_indexes_index];

#ifdef HAS_LCD
    char string[10] = "";
    char rate;
    char band[3];
    switch(current_channel_class)
    {
        case PHY_CLASS_LO_RATE: rate = 'L'; break;
        case PHY_CLASS_NORMAL_RATE: rate = 'N'; break;
        case PHY_CLASS_HI_RATE: rate = 'H'; break;
    }

    switch(current_channel_band)
    {
        case PHY_BAND_433: strncpy(band, "433", sizeof(band)); break;
        case PHY_BAND_868: strncpy(band, "868", sizeof(band)); break;
        case PHY_BAND_915: strncpy(band, "915", sizeof(band)); break;
    }

    sprintf(string, "%.3s%c-%i\n", band, rate, rx_cfg.channel_id.center_freq_index),
    lcd_write_string(string);
#endif

    /* Configure */  
    configure_radio(current_modulation);

    /* start the radio */
    start_radio();

}

#if PLATFORM_NUM_BUTTONS > 1
void userbutton_callback(button_id_t button_id)
{
    switch(button_id)
    {        
        case 1:
            // change channel and restart
            if(current_channel_indexes_index < channel_count - 1)
                current_channel_indexes_index++;
            else
                current_channel_indexes_index = 0;
            sched_post_task(&start);
    }
}
#endif

void bootstrap()
{
    DPRINT("Device booted at time: %d\n", timer_get_counter_value()); // TODO not printed for some reason, debug later

uint16_t i = 0;
    switch(current_channel_class)
    {
        case PHY_CLASS_LO_RATE:
          channel_count = LO_RATE_CHANNEL_COUNT;
            realloc(channel_indexes, channel_count);
            
            for(; i < channel_count; i++)
                channel_indexes[i] = i;

            break;
        case PHY_CLASS_NORMAL_RATE:
          channel_count = NORMAL_RATE_CHANNEL_COUNT;
            realloc(channel_indexes, channel_count);
            
            for(; i < channel_count-4; i++)
                channel_indexes[i] = i*8;
            channel_indexes[i++]=229;
            channel_indexes[i++]=239;
            channel_indexes[i++]=257;
            channel_indexes[i++]=270;

            break;
        case PHY_CLASS_HI_RATE:
          channel_count = HI_RATE_CHANNEL_COUNT;
            realloc(channel_indexes, channel_count);

            for(; i < channel_count-4; i++)
                channel_indexes[i] = i*8;
            channel_indexes[i++]=229;
            channel_indexes[i++]=239;
            channel_indexes[i++]=257;
            channel_indexes[i++]=270; 
    }

#if PLATFORM_NUM_BUTTONS > 1
    ubutton_register_callback(0, &userbutton_callback);
    ubutton_register_callback(1, &userbutton_callback);
#endif

    hw_radio_init(NULL);

    sched_register_task(&start);
    timer_post_task_delay(&start, TIMER_TICKS_PER_SEC * 2);
}