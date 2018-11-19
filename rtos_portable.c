/** ************************************************************************
 * @file portable.c
 * @brief Portable functions for ARM Cortex M0+
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
#include <rtos_module.h>
#include <linker/magic_variables.h>
#include "rtos_portable.h"

__attribute__((packed))
struct stack_frame_s
{
	/* Managed by context switcher */
	uint32_t r8; uint32_t r9; uint32_t r10; uint32_t r11;
	uint32_t r4; uint32_t r5; uint32_t r6; uint32_t r7;

	/* Managed by hardware */
	uint32_t r0; uint32_t r1; uint32_t r2; uint32_t r3;
	uint32_t r12; uint32_t lr; uint32_t pc; uint32_t psr;
};

typedef struct stack_frame_s stack_frame_t;

#define INITIAL_PSR	(0x01000000UL)							/* initial program status value */
#define SYSTICK_REG	(*((volatile uint32_t*)0xE000E010)) 	/* SysTick control register     */
#define SYSTICK_SET (1<<0) 									/* SysTick counter enable bit   */

/*
 * Starts the operating system
 */
void osport_start( void )
{
	/* enable the SysTick */
	SYSTICK_REG |= SYSTICK_SET;

	/* call SVC to initialize CPU */
	__asm volatile("SVC 0\n\t");
}

/*
 * Initializes a thread stack
 */
void* osport_init_stack( void *p_stack, OSPORT_UINT_T size,
		void (*p_start_from)(void), void (*p_return_to)(void) )
{
	stack_frame_t* p_frame = (stack_frame_t*)
			((OSPORT_BYTE_T*)p_stack + size - sizeof(stack_frame_t));

#if OSPORT_ENABLE_DEBUG
	/*
	 * Fill the stack will dummy values
	 * to see if the frame is loaded correctly
	 * onto the CPU.
	 */
	p_frame->r0 = 0x00001000;
	p_frame->r1 = 0x00001001;
	p_frame->r2 = 0x00001002;
	p_frame->r3 = 0x00001003;
	p_frame->r4 = 0x00001004;
	p_frame->r5 = 0x00001005;
	p_frame->r6 = 0x00001006;
	p_frame->r7 = 0x00001007;
	p_frame->r8 = 0x00001008;
	p_frame->r9 = 0x00001009;
	p_frame->r10 = 0x0000100A;
	p_frame->r11 = 0x0000100B;
	p_frame->r12 = 0x0000100C;
#endif

	p_frame->psr = INITIAL_PSR;

	/*
	 * PC will control where the thread starts
	 * executing when it is first loaded onto the CPU
	 */
	p_frame->pc = (uint32_t)p_start_from;

	/*
	 * LR will control where the thread jumps to
	 * when it returns. The operating system will
	 * install a routine here to make the thread kill
	 * itself and release the resources.
	 */
	p_frame->lr = (uint32_t)p_return_to;

	/* return the initial stack pointer */
	return p_frame;
}

/*
 * The do-nothing routine, gets called when
 * no other threads are able to run
 */
void osport_idle( void )
{
	for( ; ; )
		/*
		 * Wait for interrupt
		 *
		 * This will pause the CPU until
		 * an interrupt happens, where an
		 * interrupt entry will occur
		 * immediately upon wakeup.
		 */
		__asm volatile("WFI\n\t" );
}

/*
 * Systick handler on CPU0
 */
__attribute__((interrupt))
void isr_systick( void )
{
	os_handle_heartbeat();
}

/*
 * SVCall handler on CPU0
 */
__attribute__((naked, interrupt))
void isr_svc( void )
{
	__asm volatile(
		/* flush caches, lock interrupt */
		"DSB 							\n\t"
		"ISB 							\n\t"
		"CPSID I						\n\t"

		/* clear main stack */
		"LDR R0, =%[main_stack_bottom]	\n\t"
		"MSR MSP, R0					\n\t"

		/* load first thread stack pointer */
		"LDR R0, =%[pp_first_thread] 	\n\t"
		"LDR R0, [R0]					\n\t"
		"LDR R0, [R0]					\n\t"

		/* context restore, stage1 (R8-R11) */
		"LDMIA R0!, {R4-R7}				\n\t"
		"MOV R11, R7					\n\t"
		"MOV R10, R6					\n\t"
		"MOV R9, R5						\n\t"
		"MOV R8, R4						\n\t"

		/* context restore, stage2 (R4-R7) */
		"LDMIA R0!, {R4-R7}				\n\t"

		/* set stack register */
		"MSR PSP, R0					\n\t"

		/* set EXC_RETURN to use PSP */
		"LDR R0, =0xFFFFFFFD			\n\t"

		/* enable interrupts, jump to thread*/
		"CPSIE I						\n\t"
		"BX R0							\n\t"

		: /* outputs */
		: /* inputs */
		  [main_stack_bottom] "i" (&_l_main_stack_bottom),
		  [pp_first_thread] "i" (&g_sch.p_current)
	);
}

/*
 * PendSV handler on CPU0
 */
__attribute__((naked, interrupt))
void isr_pendsv( void )
{
	__asm volatile(
		/* flush caches, lock interrupt */
		"DSB 							\n\t"
		"ISB 							\n\t"
		"CPSID I						\n\t"

		/* context save, stage 1 (R4-R7) */
		"MRS R2, PSP					\n\t"
		"SUB R2, #16					\n\t"
		"STMIA R2!, {R4-R7}				\n\t"

		/* context save, stage 2 (R8-R11) */
		"MOV R7, R11					\n\t"
		"MOV R6, R10					\n\t"
		"MOV R5, R9						\n\t"
		"MOV R4, R8						\n\t"
		"SUB R2, #32					\n\t"
		"STMIA R2!, {R4-R7}				\n\t"
		"SUB R2, #16					\n\t"

		/* save current stack pointer */
		"LDR R0, =%[pp_current_thread]	\n\t"
		"LDR R1, [R0]					\n\t"
		"STR R2, [R1]					\n\t"

		/* load next thread control block */
		"LDR R1, =%[pp_next_thread]		\n\t"
		"LDR R1, [R1]					\n\t"
		"LDR R2, [R1]					\n\t"

		/* context load, stage 1 (R8-R11) */
		"LDMIA R2!, {R4-R7}				\n\t"
		"MOV R11, R7					\n\t"
		"MOV R10, R6					\n\t"
		"MOV R9, R5						\n\t"
		"MOV R8, R4						\n\t"

		/* context restore, stage2 (R4-R7) */
		"LDMIA R2!, {R4-R7}				\n\t"

		/* set stack register */
		"MSR PSP, R2					\n\t"

		/* update current thread to next thread */
		"STR R1, [R0]					\n\t"

		/* enable interrupts, jump to thread */
		"CPSIE I						\n\t"
		"BX LR							\n\t"

		: /* outputs */
		: /* inputs */
		  [pp_current_thread] "i" (&g_sch.p_current),
		  [pp_next_thread] "i" (&g_sch.p_next)
	);
}

