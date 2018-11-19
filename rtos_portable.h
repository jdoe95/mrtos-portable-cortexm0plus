/** ************************************************************************
 * @file portable.h
 * @brief Platform dependent functions for ARM Cortex-M0+
 * @author John Yu buyi.yu@wne.edu
 *
 * This file is part of mRTOS.
 *
 * Copyright (C) 2018 John Buyi Yu
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *****************************************************************************/
#ifndef H5AE6135E_6D79_4A6E_805E_BDA6BFF2ACC9
#define H5AE6135E_6D79_4A6E_805E_BDA6BFF2ACC9

/***********************************************
 * Operating system configurations
 ***********************************************/
#define OSPORT_BYTE_T		uint8_t
#define OSPORT_UINT_T		uint16_t
#define OSPORT_UINTPTR_T	uintptr_t
#define OSPORT_BOOL_T		bool

#define OSPORT_IDLE_STACK_SIZE	(64)
#define OSPORT_NUM_PRIOS		(8) /* 0 to 6, 7 reserved for idle */
#define OSPORT_MEM_ALIGN		(4)
#define OSPORT_MEM_SMALLEST		(12)
#define OSPORT_ENABLE_DEBUG 	(1)

#define OSPORT_IDLE_FUNC \
	osport_idle

#define OSPORT_START() \
	osport_start()

#define OSPORT_INIT_STACK(p_stack, size, p_start, p_return) \
	osport_init_stack(p_stack, size, p_start, p_return)

#define OSPORT_BREAKPOINT() \
	__asm volatile("BKPT 0\n\t")

#define OSPORT_DISABLE_INT() \
	/* exhaust data and instruction caches */ \
	__asm volatile("DSB\n\t ISB\n\t CPSID I\n\t")

#define OSPORT_ENABLE_INT() \
	__asm volatile("CPSIE I\n\t")

#define OSPORT_CONTEXTSW_REQ() \
	PENDSV_REG |= PENDSV_SET

/***********************************************
 * End of operating system configurations
 ***********************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

/* PendSV interrupt control */
#define PENDSV_REG (*((volatile uint32_t*)0xE000ED04))
#define PENDSV_SET (1<<28)

void osport_start( void );
void osport_idle( void ) __attribute__((noreturn));
void* osport_init_stack( void *p_stack, OSPORT_UINT_T size,
		void (*p_start_from)(void), void (*p_return_to)(void) );

#endif /* H5AE6135E_6D79_4A6E_805E_BDA6BFF2ACC9 */
