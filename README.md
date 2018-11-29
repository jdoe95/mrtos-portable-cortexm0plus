# Microcontroller Real Time Operating System

This is the portable code of MRTOS for the ARM Cortex M0+ platform. To use this
platform, please see [the main mRTOS repository](https://github.com/jdoe95/mrtos). 

The portable code uses the following interrupts:
 * SVCall, ``isr_svc``, used for loading the first thread
 * SysTick, ``isr_systick``, used for generating heart beat
 * PendSV, ``isr_pendsv``, implemented as the context switcher

The portable code does not configure the SYSTICK timer. The configuration of 
SYSTICK must be set before os_start(), and __SYSTICK must be disabled__. When
the operating system starts, it will enable the timer automatically.

Do not use ``CPSID I`` and ``CPSIE I`` instructions in the application. 
Use the nested version provided by the operating system instead (``os_enter_critical()`` and ``os_exit_critical()``). Be sure to
replace the macros in CMSIS, too, if it's being used. Manipulating interrupts directly in the application can really mess up the system's shared variable access locks.

## Features
 * Low power mode (when idle)
  
## Settings

| Setting | Value |
|---|---|
|SYSTICK interrupt frequency| 100 Hz (recommended)|
|SYSTICK priority| highest (recommended) |
|SVC priority | highest (required) |
|PendSV priority| lowest (recommended) |
  
## Code example


```C
/*
 * These variables are defined by the linker as
 * instructed by the linker script. 
 *
 * Example linker script output section (GCC), place
 * it after all other RAM sections.
 *
 * .blank_ram : ALIGN(4) {
 *	   PROVIDE( _l_blank_ram_start = . );
 *   // automatically adjust the size of the hole
 *   // to use all remaining RAM
 * 	. = ORIGIN(INT_SRAM) + LENGTH(INT_SRAM);
 * 	PROVIDE( _l_blank_ram_end = . );
 * } >INT_SRAM
 *
 */
extern int _l_blank_ram_end;
extern int _l_blank_ram_start;

int main(void)
{
	// initialize hardware, configure SYSTICK, enable SYSTICK interrupt, 
	// but do not enable counting.
	// P.S. the CMSIS function will enable SYSTICK immediately. You can 
	// either modify it or write your own.
	
	// SYSTICK has two input clocks, the CPU clock HCLK, or an external clock
	systick_config( SYSTICK_CLOCK_SOURCE_CPU, CPU_CLOCK_FREQ/100);
	
	// set exception priorities
	scb_set_svcall_prio(IRQ_PRIO_0);
	scb_set_systick_prio(IRQ_PRIO_0);
	scb_set_pendsv_prio(IRQ_PRIO_3);
	
	// ...
	
	// Initialize the operating system

	os_config_t os_config;

	os_config.p_pool_mem = &_l_blank_ram_start;
	os_config.pool_size = (uint8_t*)&_l_blank_ram_end -  (uint8_t*)&_l_blank_ram_start;

	os_init( &os_config );
	
	// create some operating system objects here: threads, semaphores, etc.
	
	// ...
	
	
	// start the operating system, run the highest
	// priority thread
	os_start();
	
	// reaches the point of no return. The execution 
	// should never get here. If it does, something 
	// went wrong.
	
	for( ; ; )
		;

	return 0;
}

```
