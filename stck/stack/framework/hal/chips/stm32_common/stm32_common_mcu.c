/* * OSS-7 - An opensource implementation of the DASH7 Alliance Protocol for ultra
 * lowpower wireless sensor communication
 *
 * Copyright 2018 University of Antwerp
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

/*! \file stm32_common_mcu.c
 *  \author glenn.ergeerts@uantwerpen.be
 */

#include "stm32_device.h"

#include "debug.h"

#include "timer.h"
#include "hwsystem.h"
#include "platform_defs.h"

static void init_clock(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  // using 32MHz clock based on HSI+PLL, use 32k LSE for timer
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Poll VOSF bit of in PWR_CSR. Wait until it is reset to 0 */
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_VOS) != RESET) {};

#if defined(STM32L0)
  // TODO not defined on STM32L1
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_MEDIUMHIGH);
#endif
  __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);

  // first switch to HSI as sysclk. Switching to HSI+PLL immediately fails sometimes when calling HAL_RCC_OscConfig()
  __HAL_RCC_HSI_ENABLE();
  while( __HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY) == 0 ) {}
  __HAL_RCC_SYSCLK_CONFIG(RCC_SYSCLKSOURCE_HSI); //
  while(__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_SYSCLKSOURCE_STATUS_HSI) {}

  // now that we are running on HSI we can disable MSI
  __HAL_RCC_MSI_DISABLE();

  RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSEState            = RCC_HSE_OFF;
  RCC_OscInitStruct.LSEState            = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PLLDIV          = RCC_PLL_DIV3;
  assert(HAL_RCC_OscConfig(&RCC_OscInitStruct) == HAL_OK);
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_VOS) != RESET) {};

  RCC_OscInitStruct.OscillatorType =  RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  assert(HAL_RCC_OscConfig(&RCC_OscInitStruct) == HAL_OK);

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
  clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  assert(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) == HAL_OK);


#ifdef FRAMEWORK_DEBUG_ENABLE_SWD
    __HAL_RCC_DBGMCU_CLK_ENABLE( );

    HAL_DBGMCU_EnableDBGSleepMode( );
    HAL_DBGMCU_EnableDBGStopMode( );
    HAL_DBGMCU_EnableDBGStandbyMode( );
#else
    __HAL_RCC_DBGMCU_CLK_ENABLE( );
    HAL_DBGMCU_DisableDBGSleepMode( );
    HAL_DBGMCU_DisableDBGStopMode( );
    HAL_DBGMCU_DisableDBGStandbyMode( );
    __HAL_RCC_DBGMCU_CLK_DISABLE( );
#endif
}

void stm32_common_mcu_init()
{
  HAL_Init();
  init_clock();
  hw_system_save_reboot_reason();
}

uint32_t HAL_GetTick(void)
{
	return timer_get_counter_value();
}

