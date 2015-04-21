/*
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 */

	.text
	.code 16
	.thumb_func
	.syntax unified

	.globl _start
	.type _start, %function
_start:
	.fnstart
	ldr r0, =0x2000BFF0
    ldr r1, =0xBABEFEED
    ldr r2, [r0, #0]
    // Note: we need to allow two extra resets (for write unprotect / read unprotect), then
    //       boot normally.
    cmp r2, r1
    bne _cont
	// Increase reset counter, if = 3, then don't go to loader
    ldrb r2, [r0, #4]
    add r2, 0x01
    strb r2, [r0, #4]
    ldrb r1, =0x03
    cmp r2,r1
    bne _loader
    str r0, [r0, #0] // Invalidate the magic word
 _cont:
	ldr	r1,=__cs3_stack
	mov	sp,r1
	ldr	r1,=__cs3_start_c
	bx	r1
	.pool
	.cantunwind
	.fnend

_loader:
	ldr r0, =0x1FFFF000
    ldr sp, [r0, #0]
    ldr r0, [r0, #4]
    bx r0
