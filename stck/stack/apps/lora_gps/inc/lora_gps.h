#include "lorawan_stack.h"
#include "hwsystem.h"

system_reboot_reason_t reboot_reason;

/*************************
 * LORAWAN CONFIGURATION *
 *************************/
static uint8_t OTAA_ENABLED = 0;   // enable/disable OTAA (if disabled, ABP is used)
uint8_t ADR_ENABLED =  1;   // enable/disable adaptive datarate
#define DATA_RATE       0   // LoRa datarate if ADR is disabled (0 - 5) SF 12 - 7

// OTAA config
#define LORAWAN_DEV_EUI     { 0x00, 0xB2, 0x58, 0xD6, 0x33, 0xD8, 0xFC, 0x85 }
#define LORAWAN_APP_EUI     { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x01, 0x43, 0xE8 }
#define LORAWAN_APP_KEY     { 0xF6, 0x6A, 0x40, 0x7D, 0x1B, 0x2A, 0x2E, 0x6A, 0x77, 0x12, 0x22, 0xF3, 0x8A, 0x91, 0xE5, 0x1A }

static lorawan_session_config_otaa_t lorawan_session_config_otaa = {
    .devEUI = LORAWAN_DEV_EUI,
    .appEUI = LORAWAN_APP_EUI,
    .appKey = LORAWAN_APP_KEY,
    .request_ack = false,
    .adr_enabled = 1,
    .data_rate = DATA_RATE
};

// ABP config
#define LORAWAN_DEVICE_ADDR           0x26011155
#define LORAWAN_NETW_SESSION_KEY      { 0xF1, 0x19, 0xC5, 0x8C, 0x7F, 0xB7, 0x5B, 0xE2, 0x61, 0xB0, 0x39, 0xA6, 0xAE, 0x14, 0x47, 0x63 }
#define LORAWAN_APP_SESSION_KEY       { 0x5D, 0xB3, 0xB1, 0x19, 0x5C, 0x3B, 0x01, 0xB7, 0xD3, 0x15, 0x6A, 0x13, 0x7E, 0xEE, 0x98, 0x06 }

static lorawan_session_config_abp_t lorawan_session_config_abp = {
    .appSKey = LORAWAN_APP_SESSION_KEY,
    .nwkSKey = LORAWAN_NETW_SESSION_KEY,
    .devAddr = LORAWAN_DEVICE_ADDR,
    .request_ack = false,
    .network_id = 0x00,
    .adr_enabled = 1,
    .data_rate = DATA_RATE
};

uint8_t transmit_buffer[100];

//#define PERIODIC_MSGS
#ifdef PERIODIC_MSGS
    uint8_t tx_delay_sec = 1;  // msg interval
#endif

//#define USE_GPS
#define USE_LEDS
uint8_t status_led = 0;
#define SEND_REBOOT_REASON


/**********************/