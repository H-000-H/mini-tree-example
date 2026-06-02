	.cpu cortex-m4
	.arch armv7e-m
	.fpu fpv4-sp-d16
	.eabi_attribute 27, 1
	.eabi_attribute 28, 1
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 1
	.eabi_attribute 30, 6
	.eabi_attribute 34, 1
	.eabi_attribute 18, 4
	.file	"startup.c"
	.text
	.align	1
	.weak	Default_Handler
	.syntax unified
	.thumb
	.thumb_func
	.type	Default_Handler, %function
Default_Handler:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	@ link register save eliminated.
	push	{r7}
	add	r7, sp, #0
.L2:
	b	.L2
	.size	Default_Handler, .-Default_Handler
	.weak	DebugMon_Handler
	.thumb_set DebugMon_Handler,Default_Handler
	.weak	UsageFault_Handler
	.thumb_set UsageFault_Handler,Default_Handler
	.weak	BusFault_Handler
	.thumb_set BusFault_Handler,Default_Handler
	.weak	MemManage_Handler
	.thumb_set MemManage_Handler,Default_Handler
	.weak	HardFault_Handler
	.thumb_set HardFault_Handler,Default_Handler
	.weak	NMI_Handler
	.thumb_set NMI_Handler,Default_Handler
	.align	1
	.global	SysTick_Handler
	.syntax unified
	.thumb
	.thumb_func
	.type	SysTick_Handler, %function
SysTick_Handler:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	push	{r7, lr}
	add	r7, sp, #0
	bl	osal_null_systick_handler
	nop
	pop	{r7, pc}
	.size	SysTick_Handler, .-SysTick_Handler
	.align	1
	.global	Reset_Handler
	.syntax unified
	.thumb
	.thumb_func
	.type	Reset_Handler, %function
Reset_Handler:
	@ Naked Function: prologue and epilogue provided by programmer.
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	.syntax unified
@ 62 "startup.c" 1
	ldr sp, =_estack
@ 0 "" 2
@ 65 "startup.c" 1
	ldr r0, =0xE000ED88
@ 0 "" 2
@ 66 "startup.c" 1
	ldr r1, [r0]
@ 0 "" 2
@ 67 "startup.c" 1
	orr r1, r1, #0x00F00000
@ 0 "" 2
@ 68 "startup.c" 1
	str r1, [r0]
@ 0 "" 2
@ 69 "startup.c" 1
	dsb
@ 0 "" 2
@ 70 "startup.c" 1
	isb
@ 0 "" 2
@ 73 "startup.c" 1
	ldr r0, =0xE000ED0C
@ 0 "" 2
@ 74 "startup.c" 1
	ldr r1, [r0]
@ 0 "" 2
@ 75 "startup.c" 1
	bic r1, r1, #0x0700
@ 0 "" 2
@ 76 "startup.c" 1
	orr r1, r1, #0x05FA0000
@ 0 "" 2
@ 77 "startup.c" 1
	str r1, [r0]
@ 0 "" 2
@ 78 "startup.c" 1
	dsb
@ 0 "" 2
@ 81 "startup.c" 1
	ldr r0, =0xE000ED20
@ 0 "" 2
@ 82 "startup.c" 1
	ldr r1, =0xFFFF0000
@ 0 "" 2
@ 83 "startup.c" 1
	str r1, [r0]
@ 0 "" 2
@ 84 "startup.c" 1
	dsb
@ 0 "" 2
	.thumb
	.syntax unified
	ldr	r5, .L10
	ldr	r4, .L10+4
	b	.L5
.L6:
	mov	r2, r5
	adds	r5, r2, #4
	mov	r3, r4
	adds	r4, r3, #4
	ldr	r2, [r2]
	str	r2, [r3]
.L5:
	ldr	r3, .L10+8
	cmp	r4, r3
	bcc	.L6
	ldr	r4, .L10+12
	b	.L7
.L8:
	mov	r3, r4
	adds	r4, r3, #4
	movs	r2, #0
	str	r2, [r3]
.L7:
	ldr	r3, .L10+16
	cmp	r4, r3
	bcc	.L8
	bl	main
.L9:
	.syntax unified
@ 95 "startup.c" 1
	wfi
@ 0 "" 2
	.thumb
	.syntax unified
	b	.L9
.L11:
	.align	2
.L10:
	.word	_sidata
	.word	_sdata
	.word	_edata
	.word	_sbss
	.word	_ebss
	.size	Reset_Handler, .-Reset_Handler
	.global	g_pfnVectors
	.section	.isr_vector,"a"
	.align	2
	.type	g_pfnVectors, %object
	.size	g_pfnVectors, 64
g_pfnVectors:
	.word	_estack
	.word	Reset_Handler
	.word	NMI_Handler
	.word	HardFault_Handler
	.word	MemManage_Handler
	.word	BusFault_Handler
	.word	UsageFault_Handler
	.word	0
	.word	0
	.word	0
	.word	0
	.word	Default_Handler
	.word	DebugMon_Handler
	.word	0
	.word	Default_Handler
	.word	SysTick_Handler
	.ident	"GCC: (GNU Tools for STM32 13.3.rel1.20250523-0900) 13.3.1 20240614"
