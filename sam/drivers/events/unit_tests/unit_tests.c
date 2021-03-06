/**
 * \file
 *
 * \brief Unit tests for Events driver (PEVC).
 *
 * Copyright (c) 2013-2014 Atmel Corporation. All rights reserved.
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

#include <asf.h>
#include <conf_test.h>

/**
 * \mainpage
 *
 * \section intro Introduction
 * This is the unit test application for Events driver (PEVC).
 * It consists of test cases for the following functionality:
 * - Using the AST as an event source and the DACC as a triggered event.
 * - Using the software trigger as an event source and the DACC as a triggered
 *   event.
 *
 * \section files Main Files
 * - \ref unit_tests.c
 * - \ref conf_test.h
 * - \ref conf_board.h
 * - \ref conf_clock.h
 * - \ref conf_uart_serial.h
 * - \ref conf_ast.h
 * - \ref conf_sleepmgr.h
 *
 * \section device_info Device Info
 * SAM4L devices can be used.
 * This example has been tested with the following setup:
 * - sam4l4c_sam4l_ek
 * - sam4lc4c_sam4l_xplained_pro
 * - sam4lc8c_sam4l8_xplained_pro
 *
 * \section compinfo Compilation info
 * This software was written for the GNU GCC and IAR for ARM. Other compilers
 * may or may not work.
 *
 * \section contactinfo Contact Information
 * For further information, visit <a href="http://www.atmel.com/">Atmel</a>.\n
 * Support and FAQ: http://www.atmel.com/design-support/
 */

/** \name Unit test configuration */
/* @{ */
/**
 * \def CONF_TEST_USART
 * \brief USART to redirect STDIO to
 */

/**
 * \def CONF_TEST_BAUDRATE
 * \brief Baudrate of USART
 */

/**
 * \def CONF_TEST_CHARLENGTH
 * \brief Character length (bits) of USART
 */

/**
 * \def CONF_TEST_PARITY
 * \brief Parity mode of USART
 */

/**
 * \def CONF_TEST_STOPBITS
 * \brief Stopbit configuration of USART
 */
/* @} */

/**
 * Initialize the AST as event generator.
 */
static void init_ast(void)
{
	struct ast_config ast_conf;

	/* Enable osc32 oscillator */
	if (!osc_is_ready(OSC_ID_OSC32)) {
		osc_enable(OSC_ID_OSC32);
		osc_wait_ready(OSC_ID_OSC32);
	}

	/* Enable the AST */
	ast_enable(AST);

	/* Configure the AST with counter mode and set counter to 0 */
	ast_conf.mode = AST_COUNTER_MODE;
	ast_conf.osc_type = AST_OSC_32KHZ;
	ast_conf.psel = AST_PSEL_32KHZ_1HZ;
	ast_conf.counter = 0;

	ast_set_config(AST, &ast_conf);

	/* Enable period enent of AST */
	ast_write_periodic0_value(AST, AST_PSEL_32KHZ_1HZ);
	ast_enable_event(AST, AST_EVENT_PER);
}

/**
 * Initialize the DAC as event user.
 */
static void init_dacc(void)
{
	/* Enable clock for DACC */
	sysclk_enable_peripheral_clock(DACC);

	/* Reset DACC registers */
	dacc_reset(DACC);

	/* Half word transfer mode */
	dacc_set_transfer_mode(DACC, 0);

	/*
	 * Timing:
	 * startup                - 0x10 (17 clocks)
	 * internal trigger clock - 0x60 (96 clocks)
	 */
	dacc_set_timing(DACC, 0x10, 0x60);

	/* Enable event trigger (0: external trigger, 1: peripheral event) */
	dacc_set_trigger(DACC, 1);

	/* Enable DAC */
	dacc_enable(DACC);
}

/**
 * \brief Test events driver with AST trigger.
 *
 * \param test Current test case.
 */
static void run_events_ast_test(const struct test_case *test)
{
	uint32_t retry_times = 3;
	bool trigger_flag = false;
	struct events_conf    events_config;
	struct events_ch_conf ch_config;

	init_ast();
	init_dacc();

	/* Initialize event module */
	events_get_config_defaults(&events_config);
	events_init(&events_config);
	events_enable();

	/*
	 * Configure an event channel
	 * - AST periodic event 0  --- Generator
	 * - DAC                   --- User
	 */
	events_ch_get_config_defaults(&ch_config);
	ch_config.channel_id = CONF_TEST_USER_ID;
	ch_config.generator_id = CONF_TEST_GEN_ID;
	ch_config.shaper_enable = true;
	ch_config.igf_edge = EVENT_IGF_EDGE_NONE;
	events_ch_configure(&ch_config);

	/* Enable the channel */
	events_ch_enable(CONF_TEST_USER_ID);

	/* Set new DACC value */
	dacc_write_conversion_data(DACC, DACC_MAX_DATA / 2);


	/* Wait for AST event trigger */
	events_ch_clear_trigger_status(CONF_TEST_USER_ID);
	do {
		if (events_ch_is_triggered(CONF_TEST_USER_ID)) {
			trigger_flag = true;
			events_ch_clear_trigger_status(CONF_TEST_USER_ID);
			break;
		}
		delay_ms(1000);
	} while (retry_times--);

	/* Disable the AST */
	ast_disable(AST);

	test_assert_true(test, trigger_flag, "AST event not triggered!");
}

/**
 * \brief Test events driver with Software trigger.
 *
 * \param test Current test case.
 */
static void run_events_software_test(const struct test_case *test)
{
	uint32_t retry_times = 3;
	bool trigger_flag = false;

	/* Enable software trigger */
	events_ch_enable_software_trigger(CONF_TEST_USER_ID);

	/* Set new DACC value */
	dacc_write_conversion_data(DACC, DACC_MAX_DATA / 5);

	/* Software event trigger */
	events_ch_clear_trigger_status(CONF_TEST_USER_ID);
	do {
		events_ch_software_trigger(CONF_TEST_USER_ID);
		delay_ms(100);
		if (events_ch_is_triggered(CONF_TEST_USER_ID)) {
			trigger_flag = true;
			events_ch_clear_trigger_status(CONF_TEST_USER_ID);
			break;
		}
	} while (retry_times--);

	test_assert_true(test, trigger_flag, "Software event not triggered!");
}

/**
 * \brief Run Event driver unit tests.
 */
int main(void)
{
	const usart_serial_options_t usart_serial_options = {
		.baudrate = CONF_TEST_BAUDRATE,
		.charlength = CONF_TEST_CHARLENGTH,
		.paritytype = CONF_TEST_PARITY,
		.stopbits = CONF_TEST_STOPBITS
	};

	sysclk_init();
	board_init();
	stdio_serial_init(CONF_TEST_USART, &usart_serial_options);

	/* Define all the test cases */
	DEFINE_TEST_CASE(events_ast_test, NULL, run_events_ast_test, NULL,
			"Test Event driver with AST trigger.");
	DEFINE_TEST_CASE(events_software_test, NULL, run_events_software_test, NULL,
			"Test Event driver with software trigger.");

	/* Put test case addresses in an array */
	DEFINE_TEST_ARRAY(events_tests) = {
		&events_ast_test,
		&events_software_test
	};

	/* Define the test suite */
	DEFINE_TEST_SUITE(events_suite, events_tests, "Events driver test suite");

	/* Run all tests in the test suite */
	test_suite_run(&events_suite);

	while (1) {
		/* Busy-wait forever. */
	}
}
