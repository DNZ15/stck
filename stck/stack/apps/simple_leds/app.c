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

#include "hwleds.h"
#include "scheduler.h"
#include "timer.h"
#include "log.h"
#include "debug.h"
#include "hwwatchdog.h"
#include "console.h"

#include "platform.h"


void led_off_callback()
{
	led_toggle(0);
	led_toggle(2);
    led_toggle(3);
}

void led_on_callback()
{
  led_toggle(0);
  led_toggle(2);
  led_toggle(3);
  timer_post_task_delay(&led_off_callback, TIMER_TICKS_PER_SEC);
}


void bootstrap()
{
    sched_register_task(&led_off_callback);
    sched_register_task(&led_on_callback);
    timer_post_task_delay(&led_off_callback, TIMER_TICKS_PER_SEC);
}