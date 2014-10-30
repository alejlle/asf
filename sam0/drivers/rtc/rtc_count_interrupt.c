/**
 * \file
 *
 * \brief SAM D20 RTC Driver (Count Interrupt Mode)
 *
 * Copyright (C) 2013 Atmel Corporation. All rights reserved.
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
#include "rtc_count_interrupt.h"


/**
 * \brief Registers callback for the specified callback type
 *
 * Associates the given callback function with the
 * specified callback type.
 * To enable the callback, the \ref rtc_count_enable_callback function
 * must be used.
 *
 * \param[in]  callback      Pointer to the function desired for the specified
 *                           callback
 * \param[in]  callback_type Callback type to register
 *
 * \return                        Status of registering callback
 * \retval STATUS_OK              Registering was done successfully
 * \retval STATUS_ERR_INVALID_ARG If trying to register a callback not available
 */
enum status_code rtc_count_register_callback(
		rtc_count_callback_t callback,
		enum rtc_count_callback callback_type)
{

	enum status_code status = STATUS_OK;

	/* Overflow callback */
	if (callback_type == RTC_COUNT_CALLBACK_OVERFLOW) {
		status = STATUS_OK;
	} else {
		/* Make sure callback type can be registered */
		switch (_rtc_dev.mode) {
		case RTC_COUNT_MODE_32BIT:
			/* Check sanity for 32-bit mode. */
			if (callback_type > RTC_NUM_OF_ALARMS) {
				status = STATUS_ERR_INVALID_ARG;
			}

			break;
		case RTC_COUNT_MODE_16BIT:
			/* Check sanity for 16-bit mode. */
			if (callback_type > RTC_NUM_OF_COMP16) {
				status = STATUS_ERR_INVALID_ARG;
			}
			break;
		default:
			status = STATUS_ERR_INVALID_ARG;
		}
	}

	if (status == STATUS_OK) {
		/* Register callback */
		_rtc_dev.callbacks[callback_type] = callback;
		/* Set corresponding bit to set callback as registered */
		_rtc_dev.registered_callback |= (1 << callback_type);
	}

	return status;
}

/**
 * \brief Unregisters callback for the specified callback type
 *
 * When called, the currently registered callback for the given callback type
 * will be removed.
 *
 * \param[in]     callback_type  Specifies the callback type to unregister
 *
 * \return                        Status of unregistering callback
 * \retval STATUS_OK              Unregistering was done successfully
 * \retval STATUS_ERR_INVALID_ARG If trying to unregister a callback not available
 */
enum status_code rtc_count_unregister_callback(
		enum rtc_count_callback callback_type)
{
	enum status_code status = STATUS_OK;

	/* Overflow callback */
	if (callback_type == RTC_COUNT_CALLBACK_OVERFLOW) {
		status = STATUS_OK;
	} else {
		/* Make sure callback type can be unregistered */
		switch (_rtc_dev.mode) {
		case RTC_COUNT_MODE_32BIT:
			/* Check sanity for 32-bit mode. */
			if (callback_type > RTC_NUM_OF_ALARMS) {
				status = STATUS_ERR_INVALID_ARG;
			}
			break;
		case RTC_COUNT_MODE_16BIT:
			/* Check sanity for 16-bit mode. */
			if (callback_type > RTC_NUM_OF_COMP16) {
				status = STATUS_ERR_INVALID_ARG;
			}
			break;
		default:
			status = STATUS_ERR_INVALID_ARG;
		}
	}
	if (status == STATUS_OK) {
		/* Unregister callback */
		_rtc_dev.callbacks[callback_type] = NULL;

		/* Clear corresponding bit to set callback as unregistered */
		_rtc_dev.registered_callback &= ~(1 << callback_type);
	}
	return status;
}

/**
 * \brief Enables callback
 *
 * Enables the callback specified by the callback_type.
 *
 * \param[in]     callback_type Callback type to enable
 */
void rtc_count_enable_callback(
		enum rtc_count_callback callback_type)
{
	/* Initialize hardware module pointer */
	Rtc *const rtc_hw = RTC;

	if (callback_type == RTC_COUNT_CALLBACK_OVERFLOW) {
		rtc_hw->MODE0.INTENSET.reg = RTC_MODE0_INTFLAG_OVF;
	} else {
		rtc_hw->MODE0.INTENSET.reg = RTC_MODE1_INTFLAG_CMP(1 << callback_type);
	}
	/* Mark callback as enabled. */
	_rtc_dev.enabled_callback |= (1 << callback_type);
}

/**
 * \brief Disables callback
 *
 * Disables the callback specified by the callback_type.
 *
 * \param[in]     callback_type Callback type to disable
 */
void rtc_count_disable_callback(
		enum rtc_count_callback callback_type)
{
	/* Initialize hardware module pointer */
	Rtc *const rtc_hw = RTC;

	/* Disable interrupt */
	if (callback_type == RTC_COUNT_CALLBACK_OVERFLOW) {
		rtc_hw->MODE0.INTENCLR.reg = RTC_MODE0_INTFLAG_OVF;
	} else {
		rtc_hw->MODE0.INTENCLR.reg = RTC_MODE1_INTFLAG_CMP(1 << callback_type);
	}

	/* Mark callback as disabled. */
	_rtc_dev.enabled_callback &= ~(1 << callback_type);
}

/**
 * \internal Interrupt handler for RTC
 *
 */
void RTC_Handler(void)
{
	/* Initialize hardware module pointer */
	Rtc *const rtc_hw = RTC;

	/* Combine callback registered and enabled masks */
	uint8_t callback_mask = _rtc_dev.enabled_callback &
			_rtc_dev.registered_callback;

	/* Read and mask interrupt flag register */
	uint16_t interrupt_status = (rtc_hw->MODE0.INTFLAG.reg &
			rtc_hw->MODE0.INTENSET.reg);

	if (interrupt_status & RTC_MODE0_INTFLAG_OVF) {
		/* Overflow interrupt */
		if (callback_mask & (1 << RTC_COUNT_CALLBACK_OVERFLOW)) {
			_rtc_dev.callbacks[RTC_COUNT_CALLBACK_OVERFLOW]();
		}

		/* Clear interrupt flag */
		rtc_hw->MODE0.INTFLAG.reg = RTC_MODE0_INTFLAG_OVF;

	} else if (interrupt_status & RTC_MODE1_INTFLAG_CMP(1 << 0)) {
		/* Compare 0 interrupt */
		if (callback_mask & (1 << RTC_COUNT_CALLBACK_COMPARE_0)) {
			_rtc_dev.callbacks[RTC_COUNT_CALLBACK_COMPARE_0]();
		}
		/* Clear interrupt flag */
		rtc_hw->MODE0.INTFLAG.reg = RTC_MODE1_INTFLAG_CMP(1 << 0);

	} else if (interrupt_status & RTC_MODE1_INTFLAG_CMP(1 << 1)) {
		#if (RTC_NUM_OF_COMP16 > 1) || defined(__DOXYGEN__)
		/* Compare 1 interrupt */
		if (callback_mask & (1 << RTC_COUNT_CALLBACK_COMPARE_1)) {
			_rtc_dev.callbacks[RTC_COUNT_CALLBACK_COMPARE_1]();
		}
		/* Clear interrupt flag */
		rtc_hw->MODE0.INTFLAG.reg = RTC_MODE1_INTFLAG_CMP(1 << 1);
		#endif

	} else if (interrupt_status & RTC_MODE1_INTFLAG_CMP(1 << 2)) {
		#if (RTC_NUM_OF_COMP16 > 2)	|| defined(__DOXYGEN__)
		/* Compare 2 interrupt */
		if (callback_mask & (1 << RTC_COUNT_CALLBACK_COMPARE_2)) {
			_rtc_dev.callbacks[RTC_COUNT_CALLBACK_COMPARE_2]();
		}
		/* Clear interrupt flag */
		rtc_hw->MODE0.INTFLAG.reg = RTC_MODE1_INTFLAG_CMP(1 << 2);
		#endif

	} else if (interrupt_status & RTC_MODE1_INTFLAG_CMP(1 << 3)) {
		#if (RTC_NUM_OF_COMP16 > 3)	|| defined(__DOXYGEN__)
		/* Compare 3 interrupt */
		if (callback_mask & (1 << RTC_COUNT_CALLBACK_COMPARE_3)) {
			_rtc_dev.callbacks[RTC_COUNT_CALLBACK_COMPARE_3]();
		}
		/* Clear interrupt flag */
		rtc_hw->MODE0.INTFLAG.reg = RTC_MODE1_INTFLAG_CMP(1 << 3);
		#endif

	} else if (interrupt_status & RTC_MODE1_INTFLAG_CMP(1 << 4)) {
		#if (RTC_NUM_OF_COMP16 > 4) || defined(__DOXYGEN__)
		/* Compare 4 interrupt */
		if (callback_mask & (1 << RTC_COUNT_CALLBACK_COMPARE_4)) {
			_rtc_dev.callbacks[RTC_COUNT_CALLBACK_COMPARE_4]();
		}
		/* Clear interrupt flag */
		rtc_hw->MODE0.INTFLAG.reg = RTC_MODE1_INTFLAG_CMP(1 << 4);
		#endif

	} else if (interrupt_status & RTC_MODE1_INTFLAG_CMP(1 << 5)) {
		#if (RTC_NUM_OF_COMP16 > 5) || defined(__DOXYGEN__)
		/* Compare 5 interrupt */
		if (callback_mask & (1 << RTC_COUNT_CALLBACK_COMPARE_5)) {
			_rtc_dev.callbacks[RTC_COUNT_CALLBACK_COMPARE_5]();
		}
		/* Clear interrupt flag */
		rtc_hw->MODE0.INTFLAG.reg = RTC_MODE1_INTFLAG_CMP(1 << 5);
		#endif
	}
}