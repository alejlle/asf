/*
 * \file
 *
 * \brief SAM ADC Sleepwalking Example
 *
 * Copyright (C) 2013-2014 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

/**
 * \mainpage SAM ADC Sleepwalking Example
 * See \ref appdoc_main "here" for project documentation.
 * \copydetails appdoc_preface
 *
 *
 * \page appdoc_preface Overview
 * This application demonstrates how to configure the ADC in a sleepwalking
 * operation, where the input voltage is measured at a fixed interval,
 * but the device is not woken from sleep until the measured value is below
 * a given threshold; in this application 0.5 Volts.
 */

/**
 * \page appdoc_main SAM ADC Sleepwalking Example
 *
 * Overview:
 * - \ref appdoc_sam0_adc_sleepwalking_intro
 * - \ref appdoc_sam0_adc_sleepwalking_setup
 * - \ref appdoc_sam0_adc_sleepwalking_imp
 * - \ref appdoc_sam0_adc_sleepwalking_compinfo
 * - \ref appdoc_sam0_adc_sleepwalking_contactinfo
 *
 * \section appdoc_sam0_adc_sleepwalking_intro Introduction
 * This application demonstrates how to configure the ADC in a sleepwalking
 * operation, where the input voltage is measured at a fixed interval,
 * but the device is not woken from sleep  until the measured value is below
 * a given threshold; in this application 0.5 Volts.
 *
 * This application has been tested on following boards:
 * - SAM D20/D21/R21/D11 Xplained Pro
 *
 * \section appdoc_sam0_adc_sleepwalking_setup Hardware Setup
 * This application use AIN0 as ADC input channel.
 * When the measured voltage is lower than 0.5V, then the device will start
 * toggling the led pin to signal that the low voltage condition has happened.
 * Connect the PA02(EXT3 pin3) to GND in SAM D20/D21 Xplained Pro to trigger it.
 * Connect the PA06(EXT1 pin3) to GND in SAM R21 Xplained Pro to trigger it.
 * Connect the PA02(EXT1 pin3) to GND in SAM D10/D11 Xplained Pro to trigger it.
 *
 * If debugging it is also possible to start a debug session and place a
 * breakpoint in the window callback that will trigger whenever the voltage
 * has gone below the defined threshold.
 *
 * The application can easily be modified to monitor other voltages by changing
 * the input source and threshold values in the \c adc_setup function.
 *
 * \section appdoc_sam0_adc_sleepwalking_imp Implementation Details
 *
 * \subsection appdoc_sam0_adc_sleepwalking_adc ADC Configuration
 * The ADC is configured to perform hardware averaging on the measured value,
 * where the ADC will sample the signal 16 times and find the average value in
 * hardware. This smooths out high frequency noise.
 *
 * \subsection appdoc_sam0_adc_sleepwalking_rtc Sample timing
 * To trigger the ADC conversions, the RTC is setup in 32-bit counter mode,
 * clocked by the internal 32kHz oscillator divided by 32, giving a 1kHz input
 * clock. A compare value of 1000 is used to trigger the ADC conversion every
 * second.
 *
 * \subsection appdoc_sam0_adc_sleepwalking_events Event Routing
 * The Event system is setup to route the compare match interrupt from the RTC
 * to the ADC, and the ADC is configured to start a new conversion on an event
 * input. The event channel is configured as an asynchronous channel, so no
 * clock is needed for the event channel during sleep.
 *
 * \section appdoc_sam0_adc_sleepwalking_compinfo Compilation Info
 * This software was written for the GNU GCC and IAR for ARM.
 * Other compilers may or may not work.
 *
 * \section appdoc_sam0_adc_sleepwalking_contactinfo Contact Information
 * For further information, visit
 * <a href="http://www.atmel.com">http://www.atmel.com</a>.
 */

#include <asf.h>

/* ADC module instance */
struct adc_module module_inst;

/* RTC module instance */
struct rtc_module rtc_instance;

/* Flag to indicate if a low-voltage situation has occurred */
volatile bool low_voltage = false;

/* ADC Window callback; called whenever the supply voltage drops
 * below ~0.5 Volts */
static void adc_window_callback(
	const struct adc_module *const module)
{
	/* Signal the application that the voltage has gone below the
	threshold */
	low_voltage = true;
}

/* Setup RTC as ADC sample timer */
static void rtc_setup(void)
{
	struct rtc_count_config conf;

	rtc_count_get_config_defaults(&conf);

	conf.prescaler         = RTC_COUNT_PRESCALER_DIV_1;
	conf.mode              = RTC_COUNT_MODE_32BIT;
	conf.clear_on_match    = true;
	conf.compare_values[0] = 1000;

	struct rtc_count_events evconfig;
	evconfig.generate_event_on_compare[0] = true;

	rtc_count_init(&rtc_instance, RTC, &conf);
	rtc_count_enable_events(&rtc_instance, &evconfig);
	rtc_count_enable(&rtc_instance);
}

/* Setup the event system to route RTC events to the ADC */
static void event_setup(struct events_resource *event)
{
	struct events_config config;

	events_get_config_defaults(&config);

	/* Setup a event channel with RTC compare 0 as input */
	config.generator    = EVSYS_ID_GEN_RTC_CMP_0;
	config.path         = EVENTS_PATH_ASYNCHRONOUS;
	events_allocate(event, &config);

	/* Setup ADC to listen to the event channel */
	events_attach_user(event, EVSYS_ID_USER_ADC_START);
}

/* Setup the ADC to sample the internal scaled VCC */
static void adc_setup(void)
{
	struct adc_config config;
	adc_get_config_defaults(&config);

	config.gain_factor        = ADC_GAIN_FACTOR_1X;
	/* Use GCLK generator 4 as clock source */
	config.clock_source       = GCLK_GENERATOR_4;
	/* Divide input clock by 4 (8MHz / 4 = 2MHz) */
	config.clock_prescaler    = ADC_CLOCK_PRESCALER_DIV4;
	/* Use internal 1V band-gap reference */
	config.reference          = ADC_REFERENCE_INT1V;
	/* Start new conversion on event */
	config.event_action       = ADC_EVENT_ACTION_START_CONV;

	/* Enable the ADC to run in standby sleep mode */
	config.run_in_standby     = true;

	/* Average 16 samples in hardware (accumulate 16 samples and divide by 16) */
	config.resolution         = ADC_RESOLUTION_CUSTOM;
	config.accumulate_samples = ADC_ACCUMULATE_SAMPLES_16;
	config.divide_result      = ADC_DIVIDE_RESULT_16;

	/* Configure window */
	config.window.window_mode        = ADC_WINDOW_MODE_BELOW_UPPER;
	/* (1.0V / 4095 *2048) = 0.5V */
	config.window.window_upper_value = 2048;

	/* Apply configuration to ADC module */
	adc_init(&module_inst, ADC, &config);

	/* Enable ADC */
	adc_enable(&module_inst);

	/* Register and enable the window callback */
	adc_register_callback(&module_inst, adc_window_callback,
			ADC_CALLBACK_WINDOW);
	adc_enable_callback(&module_inst, ADC_CALLBACK_WINDOW);
}

int main(void)
{
	struct events_resource event;

	/* Initialize clock system */
	system_init();

	/* Initialize delay service */
	delay_init();

	rtc_setup();
	event_setup(&event);
	adc_setup();

	system_interrupt_enable_global();

	/* Set sleep mode to STANDBY */
	system_set_sleepmode(SYSTEM_SLEEPMODE_STANDBY);

	/* Stay in STANDBY sleep until low voltage is detected */
	system_sleep();

	while (true) {
		if (low_voltage == true) {
			while (true) {
				/* Toggle pin ad-infinitum */
				port_pin_toggle_output_level(LED0_PIN);
				delay_ms(500);
			}
		}
	}
}
