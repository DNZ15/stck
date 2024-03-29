/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech
Description: Ping-Pong implementation
License: Revised BSD License, see LICENSE.TXT file include in the project
Maintainer: Miguel Luis and Gregory Cristian
*/
/******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @version V1.1.2
  * @date    08-September-2017
  * @brief   this is the main!
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
//#include "hw.h"
#include "Phy\radio.h"
#include "timeServer.h"
#include "delay.h"
#include "low_power.h"
//#include "vcom.h"


#include "hwleds.h"

#include "scheduler.h"
#include "lorawan_stack.h"

#include "hwwatchdog.h"
#include "console.h"
#include "platform.h"


#define RF_FREQUENCY                                868000000 // Hz
#define TX_OUTPUT_POWER                             14        // dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                          //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         5         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false



typedef enum
{
    LOWPOWER,
    RX,
    RX_TIMEOUT,
    RX_ERROR,
    TX,
    TX_TIMEOUT,
}States_t;

#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 64 // Define the payload size here
#define LED_PERIOD_MS               200


const uint8_t PingMsg[] = "PING";
const uint8_t PongMsg[] = "PONG";

uint16_t BufferSize = BUFFER_SIZE;
uint8_t Buffer[BUFFER_SIZE];

States_t State = LOWPOWER;

int8_t RssiValue = 0;
int8_t SnrValue = 0;

 /* Led Timers objects*/
static  TimerEvent_t timerLed;

/* Private function prototypes -----------------------------------------------*/
/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

/*!
 * \brief Function to be executed on Radio Rx Done event
 */
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void OnTxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Timeout event
 */
void OnRxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Error event
 */
void OnRxError( void );

/*!
 * \brief Function executed on when led timer elapses
 */
static void OnledEvent( void );
/**
 * Main application entry point.
 */


void DelayMs( uint32_t ms )
{
  HW_RTC_DelayMs( ms );

}

int main( void )
{
  uint8_t i;

  HAL_Init( );
  
  SystemClock_Config( );
  
  DBG_Init( );

  HW_Init( );  
  
  /* Led Timers*/
  TimerInit(&timerLed, OnledEvent);   
  TimerSetValue( &timerLed, LED_PERIOD_MS);

  TimerStart(&timerLed );

  // Radio initialization
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;

  Radio.Init( &RadioEvents );

  Radio.SetChannel( RF_FREQUENCY );

#if defined( USE_MODEM_LORA )
    
  Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON, true );

#elif defined( USE_MODEM_FSK )
    
  Radio.SetRxConfig( MODEM_FSK, FSK_BANDWIDTH, FSK_DATARATE,
                                  0, FSK_AFC_BANDWIDTH, FSK_PREAMBLE_LENGTH,
                                  0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
                                  0, 0,false, true );

//#else
//    #error "Please define a frequency band in the compiler options."
#endif
                                  
  Radio.Rx( RX_TIMEOUT_VALUE );
                                  
  while( 1 )
  {
    switch( State )
    {
    case RX:
        if( BufferSize > 0 )
        {
          if( strncmp( ( const char* )Buffer, ( const char* )PongMsg, 4 ) == 0 )
          {
            TimerStop(&timerLed );
           // LED_Off( LED_BLUE);
           // LED_Off( LED_GREEN ) ; 
            //LED_Off( LED_RED1 ) ;;
            // Indicates on a LED that the received frame is a PONG
            //LED_Toggle( LED_RED2 ) ;

            led_off(2); // toggle blue LED
            led_off(0); // off green LED  
            led_off(1); 
            led_off(3);         
   
            // Send the next PING frame      
            Buffer[0] = 'P';
            Buffer[1] = 'I';
            Buffer[2] = 'N';
            Buffer[3] = 'G';
            // We fill the buffer with numbers for the payload 
            for( i = 4; i < BufferSize; i++ )
            {
              Buffer[i] = i - 4;
            }
            PRINTF("...PING\n\r");

            DelayMs( 1 ); 
            Radio.Send( Buffer, BufferSize );
            }
            else if( strncmp( ( const char* )Buffer, ( const char* )PingMsg, 4 ) == 0 )
            { // A master already exists then become a slave
              isMaster = false;
              //GpioWrite( &Led2, 1 ); // Set LED off
              Radio.Rx( RX_TIMEOUT_VALUE );
            }
            else // valid reception but neither a PING or a PONG message
            {    // Set device as master ans start again
              isMaster = true;
              Radio.Rx( RX_TIMEOUT_VALUE );
            }
          }

      State = LOWPOWER;
      break;
    case TX:
      // Indicates on a LED that we have sent a PING [Master]
      // Indicates on a LED that we have sent a PONG [Slave]
      //GpioWrite( &Led2, GpioRead( &Led2 ) ^ 1 );
      Radio.Rx( RX_TIMEOUT_VALUE );
      State = LOWPOWER;
      break;
    case RX_TIMEOUT:
    case RX_ERROR:
      if( isMaster == true )
      {
        // Send the next PING frame
        Buffer[0] = 'P';
        Buffer[1] = 'I';
        Buffer[2] = 'N';
        Buffer[3] = 'G';
        for( i = 4; i < BufferSize; i++ )
        {
          Buffer[i] = i - 4;
        }
        DelayMs( 1 ); 
        Radio.Send( Buffer, BufferSize );
      }
      else
      {
        Radio.Rx( RX_TIMEOUT_VALUE );
      }
      State = LOWPOWER;
      break;
    case TX_TIMEOUT:
      Radio.Rx( RX_TIMEOUT_VALUE );
      State = LOWPOWER;
      break;
    case LOWPOWER:
      default:
            // Set low power
      break;
    }
    
    DISABLE_IRQ( );
    /* if an interupt has occured after __disable_irq, it is kept pending 
     * and cortex will not enter low power anyway  */
    if (State == LOWPOWER)
    {
#ifndef LOW_POWER_DISABLE
      LowPower_Handler( );
#endif
    }
    ENABLE_IRQ( );
       
  }
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    Radio.Sleep( );
    BufferSize = size;
    memcpy( Buffer, payload, BufferSize );
    RssiValue = rssi;
    SnrValue = snr;
    State = RX;
  
    PRINTF("OnRxDone\n\r");
    PRINTF("RssiValue=%d dBm, SnrValue=%d\n\r", rssi, snr);
}


void OnRxTimeout( void )
{
    Radio.Sleep( );
    State = RX_TIMEOUT;
    PRINTF("OnRxTimeout\n\r");
}

void OnRxError( void )
{
    Radio.Sleep( );
    State = RX_ERROR;
    PRINTF("OnRxError\n\r");
}

static void OnledEvent( void )
{
   // LED_Toggle( LED_BLUE ) ; 
 // LED_Toggle( LED_RED1 ) ; 
  //LED_Toggle( LED_RED2 ) ; 
 // LED_Toggle( LED_GREEN ) ; 
  led_toggle(2); // toggle blue LED
  led_toggle(1); 
  led_toggle(3); 
  led_toggle(0); // toggle green LED
  	
  TimerStart(&timerLed );
}