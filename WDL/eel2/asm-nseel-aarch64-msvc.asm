NSEEL_RAM_ITEMSPERBLOCK_LOG2 equ 16
NSEEL_RAM_BLOCKS_LOG2 equ 11
NSEEL_LOOPFUNC_SUPPORT_MAXLEN equ 1048576 

NSEEL_RAM_BLOCKS equ (1 << NSEEL_RAM_BLOCKS_LOG2)
NSEEL_RAM_ITEMSPERBLOCK equ (1<<NSEEL_RAM_ITEMSPERBLOCK_LOG2)

	AREA	|.drectve|, DRECTVE

	AREA	|.data|, DATA

	EXPORT |nseel_asm_1pdd|
	EXPORT |nseel_asm_2pdd|
	EXPORT |nseel_asm_2pdds|
	EXPORT |nseel_asm_invsqrt|
	EXPORT |nseel_asm_dbg_getstackptr|
	EXPORT |nseel_asm_sqr|
	EXPORT |nseel_asm_abs|
	EXPORT |nseel_asm_assign|
	EXPORT |nseel_asm_assign_fromfp|
	EXPORT |nseel_asm_assign_fast|
	EXPORT |nseel_asm_assign_fast_fromfp|
	EXPORT |nseel_asm_add|
	EXPORT |nseel_asm_add_op|
	EXPORT |nseel_asm_add_op_fast|
	EXPORT |nseel_asm_sub|
	EXPORT |nseel_asm_sub_op|
	EXPORT |nseel_asm_sub_op_fast|
	EXPORT |nseel_asm_mul|
	EXPORT |nseel_asm_mul_op|
	EXPORT |nseel_asm_mul_op_fast|
	EXPORT |nseel_asm_div|
	EXPORT |nseel_asm_div_op|
	EXPORT |nseel_asm_div_op_fast|
	EXPORT |nseel_asm_mod|
	EXPORT |nseel_asm_shl|
	EXPORT |nseel_asm_shr|
	EXPORT |nseel_asm_mod_op|
	EXPORT |nseel_asm_or|
	EXPORT |nseel_asm_or0|
	EXPORT |nseel_asm_or_op|
	EXPORT |nseel_asm_xor|
	EXPORT |nseel_asm_xor_op|
	EXPORT |nseel_asm_and|
	EXPORT |nseel_asm_and_op|
	EXPORT |nseel_asm_uminus|
	EXPORT |nseel_asm_sign|
	EXPORT |nseel_asm_bnot|
	EXPORT |nseel_asm_if|
	EXPORT |nseel_asm_repeat|
	EXPORT |nseel_asm_repeatwhile|
	EXPORT |nseel_asm_band|
	EXPORT |nseel_asm_bor|
	EXPORT |nseel_asm_equal|
	EXPORT |nseel_asm_equal_exact|
	EXPORT |nseel_asm_notequal_exact|
	EXPORT |nseel_asm_notequal|
	EXPORT |nseel_asm_below|
	EXPORT |nseel_asm_beloweq|
	EXPORT |nseel_asm_above|
	EXPORT |nseel_asm_aboveeq|
	EXPORT |nseel_asm_min|
	EXPORT |nseel_asm_max|
	EXPORT |nseel_asm_min_fp|
	EXPORT |nseel_asm_max_fp|
	EXPORT |_asm_generic3parm|
	EXPORT |_asm_generic3parm_retd|
	EXPORT |_asm_generic2parm|
	EXPORT |_asm_generic2parm_retd|
	EXPORT |_asm_generic2xparm_retd|
	EXPORT |_asm_generic1parm|
	EXPORT |_asm_generic1parm_retd|
	EXPORT |_asm_megabuf|
	EXPORT |_asm_gmegabuf|
	EXPORT |nseel_asm_fcall|
	EXPORT |nseel_asm_stack_push|
	EXPORT |nseel_asm_stack_pop|
	EXPORT |nseel_asm_stack_pop_fast|
	EXPORT |nseel_asm_stack_peek|
	EXPORT |nseel_asm_stack_peek_top|
	EXPORT |nseel_asm_stack_peek_int|
	EXPORT |nseel_asm_stack_exch|
	EXPORT |nseel_asm_booltofp|
	EXPORT |nseel_asm_fptobool|
	EXPORT |nseel_asm_fptobool_rev|
	EXPORT |eel_callcode64|
	EXPORT |glue_setscr|
	EXPORT |glue_getscr|

	AREA	|.text$mn|, CODE, ARM64

|nseel_asm_1pdd| PROC
	mov x1, x1
	mov x3, 0xdead
	movk x3, 0xbeef, lsl 16
	movk x3, 0xbeef, lsl 32
	stp fp, lr, [sp, #-16]!
	mov fp, sp
	blr x3
	ldp fp, lr, [sp], #16
	mov x1, x1
	ENDP

|nseel_asm_2pdd| PROC
	mov x1, x1
	mov x3, 0xdead
	movk x3, 0xbeef, lsl 16
	movk x3, 0xbeef, lsl 32
	fmov d2, d0
	fmov d0, d1
	fmov d1, d2
	stp fp, lr, [sp, #-16]!
	mov fp, sp
	blr x3
	ldp fp, lr, [sp], #16
	mov x1, x1
	ENDP;

|nseel_asm_2pdds| PROC
	mov x1, x1
	mov x3, 0xdead
	movk x3, 0xbeef, lsl 16
	movk x3, 0xbeef, lsl 32
	stp fp, lr, [sp, #-32]!
	mov fp, sp
	str x1, [sp, #16]
	fmov d1, d0
	ldr d0, [x1]
	blr x3
	ldr x0, [sp, #16]
	ldp fp, lr, [sp], #32
	str d0, [x0]
	mov x1, x1
	ENDP


|nseel_asm_invsqrt| PROC


	mov x1, x1
	mov x0, 0x59df
	movk x0, 0x5f37, lsl 16
	fcvt s2, d0
	ldr d3, [x21, #32]
	fmov w1, s2
	fmul d0, d0, d3
	asr w1, w1, #1

	sub w0, w0, w1

	fmov s4, w0
	fcvt d1, s4

	ldr d2, [x21, #40]
	fmul d0, d0, d1
	fmul d0, d0, d1
	fadd d0, d0, d2
	fmul d0, d0, d1
    
	mov x1, x1
	ENDP

|nseel_asm_dbg_getstackptr| PROC


	mov x1, x1
	mov x0, sp
	ucvtf d0, x0
	mov x1, x1

	ENDP



|nseel_asm_sqr| PROC


	mov x1, x1
	fmul d0, d0, d0
	mov x1, x1

	ENDP



|nseel_asm_abs| PROC


	mov x1, x1
	fabs d0, d0
	mov x1, x1

	ENDP


|nseel_asm_assign| PROC


	mov x1, x1
	ldr d0, [x0]
	mov x0, x1
	str d0, [x1]

	ldr x1, [x1]
	mov     x3, #0x10000000000000
	mov     x2, #0x20000000000000
	add     x1, x1, x3
	and     x1, x1, #0x7ff0000000000000
	cmp     x1, x2
	bgt     |ftz1|
	str     xzr, [x0]
|ftz1|

	mov x1, x1

	ENDP


|nseel_asm_assign_fromfp| PROC


	mov x1, x1
	mov x0, x1
	str d0, [x1]

	ldr x1, [x1]
	mov     x3, #0x10000000000000
	mov     x2, #0x20000000000000
	add     x1, x1, x3
	and     x1, x1, #0x7ff0000000000000
	cmp     x1, x2
	bgt     |ftz2|
	str     xzr, [x0]
|ftz2|

	mov x1, x1

	ENDP


|nseel_asm_assign_fast| PROC


	mov x1, x1
	ldr d0, [x0]
	mov x0, x1
	str d0, [x1]
	mov x1, x1

	ENDP


|nseel_asm_assign_fast_fromfp| PROC


	mov x1, x1
	mov x0, x1
	str d0, [x1]
	mov x1, x1

	ENDP




|nseel_asm_add| PROC


	mov x1, x1
	fadd d0, d1, d0
	mov x1, x1

	ENDP

|nseel_asm_add_op| PROC


	mov x1, x1
	ldr d1, [x1]
	fadd d0, d1, d0
	mov x0, x1
	str d0, [x1]

	ldr x1, [x1]
	mov     x3, #0x10000000000000
	mov     x2, #0x20000000000000
	add     x1, x1, x3
	and     x1, x1, #0x7ff0000000000000
	cmp     x1, x2
	bgt     |ftz3|
	str     xzr, [x0]
|ftz3|

	mov x1, x1

	ENDP

|nseel_asm_add_op_fast| PROC


	mov x1, x1
	ldr d1, [x1]
	fadd d0, d1, d0
	mov x0, x1
	str d0, [x1]
	mov x1, x1

	ENDP



|nseel_asm_sub| PROC


	mov x1, x1
	fsub d0, d1, d0
	mov x1, x1

	ENDP

|nseel_asm_sub_op| PROC


	mov x1, x1
	ldr d1, [x1]
	fsub d0, d1, d0
	mov x0, x1
	str d0, [x1]

	ldr x1, [x1]
	mov     x3, #0x10000000000000
	mov     x2, #0x20000000000000
	add     x1, x1, x3
	and     x1, x1, #0x7ff0000000000000
	cmp     x1, x2
	bgt     |ftz4|
	str     xzr, [x0]
|ftz4|

	mov x1, x1

	ENDP

|nseel_asm_sub_op_fast| PROC


	mov x1, x1
	ldr d1, [x1]
	fsub d0, d1, d0
	mov x0, x1
	str d0, [x1]
	mov x1, x1

	ENDP


|nseel_asm_mul| PROC


	mov x1, x1
	fmul d0, d1, d0
	mov x1, x1

	ENDP

|nseel_asm_mul_op| PROC


	mov x1, x1
	ldr d1, [x1]
	fmul d0, d0, d1
	mov x0, x1
	str d0, [x1]

	ldr x1, [x1]
	mov     x3, #0x10000000000000
	mov     x2, #0x20000000000000
	add     x1, x1, x3
	and     x1, x1, #0x7ff0000000000000
	cmp     x1, x2
	bgt     |ftz5|
	str     xzr, [x0]
|ftz5|

	mov x1, x1

	ENDP

|nseel_asm_mul_op_fast| PROC


	mov x1, x1
	ldr d1, [x1]
	fmul d0, d0, d1
	mov x0, x1
	str d0, [x1]
	mov x1, x1

	ENDP


|nseel_asm_div| PROC


	mov x1, x1
	fdiv d0, d1, d0
	mov x1, x1

	ENDP

|nseel_asm_div_op| PROC


	mov x1, x1
	ldr d1, [x1]
	fdiv d0, d1, d0
	mov x0, x1
	str d0, [x1]

	ldr x1, [x1]
	mov     x3, #0x10000000000000
	mov     x2, #0x20000000000000
	add     x1, x1, x3
	and     x1, x1, #0x7ff0000000000000
	cmp     x1, x2
	bgt     |ftz6|
	str     xzr, [x0]
|ftz6|

	mov x1, x1

	ENDP

|nseel_asm_div_op_fast| PROC


	mov x1, x1
	ldr d1, [x1]
	fdiv d0, d1, d0
	mov x0, x1
	str d0, [x1]
	mov x1, x1

	ENDP


|nseel_asm_mod| PROC


	mov x1, x1
	fabs d0, d0
	fabs d1, d1
	fcvtzu w1, d0
	fcvtzu w0, d1
	udiv w2, w0, w1
	msub w0, w2, w1, w0
	ucvtf d0, w0
	mov x1, x1

	ENDP

|nseel_asm_shl| PROC


	mov x1, x1
	fcvtzs w0, d1
	fcvtzs w1, d0
	lsl w0, w0, w1
	scvtf d0, w0
	mov x1, x1

	ENDP

|nseel_asm_shr| PROC


	mov x1, x1
	fcvtzs w0, d1
	fcvtzs w1, d0
	asr w0, w0, w1
	scvtf d0, w0
	mov x1, x1

	ENDP

|nseel_asm_mod_op| PROC



	mov x1, x1
	ldr d1, [x1]
	fabs d0, d0
	fabs d1, d1
	fcvtzu w3, d0
	fcvtzu w0, d1
	udiv w2, w0, w3
	msub w0, w2, w3, w0
	ucvtf d0, w0

	str d0, [x1]
	mov x0, x1
	mov x1, x1


	ENDP


|nseel_asm_or| PROC


	mov x1, x1
	fcvtzs x0, d0
	fcvtzs x1, d1
	orr x0, x0, x1
	scvtf d0, x0
	mov x1, x1

	ENDP

|nseel_asm_or0| PROC


	mov x1, x1
	fcvtzs x0, d0
	scvtf d0, x0
	mov x1, x1

	ENDP

|nseel_asm_or_op| PROC


	mov x1, x1
	ldr d1, [x1]
	fcvtzs x0, d0
	fcvtzs x3, d1
	orr x0, x0, x3
	scvtf d0, x0
	mov x0, x1
	str d0, [x1]
	mov x1, x1

	ENDP


|nseel_asm_xor| PROC


	mov x1, x1
	fcvtzs x0, d0
	fcvtzs x1, d1
	eor x0, x0, x1
	scvtf d0, x0
	mov x1, x1

	ENDP

|nseel_asm_xor_op| PROC


	mov x1, x1
	ldr d1, [x1]
	fcvtzs x0, d0
	fcvtzs x3, d1
	eor x0, x0, x3
	scvtf d0, x0
	mov x0, x1
	str d0, [x1]
	mov x1, x1

	ENDP


|nseel_asm_and| PROC


	mov x1, x1
	fcvtzs x0, d0
	fcvtzs x1, d1
	and x0, x0, x1
	scvtf d0, x0
	mov x1, x1

	ENDP

|nseel_asm_and_op| PROC


	mov x1, x1
	ldr d1, [x1]
	fcvtzs x0, d0
	fcvtzs x3, d1
	and x0, x0, x3
	scvtf d0, x0
	mov x0, x1
	str d0, [x1]
	mov x1, x1

	ENDP



|nseel_asm_uminus| PROC


	mov x1, x1
	fneg d0, d0
	mov x1, x1

	ENDP



|nseel_asm_sign| PROC


	mov x1, x1
	fmov d1, #-1.0
	fmov d2, #1.0
	fcmpe d0, #0.0
	fcsel d0, d0, d1, gt
	fcsel d0, d0, d2, lt
	mov x1, x1

	ENDP




|nseel_asm_bnot| PROC


	mov x1, x1
	cmp w0, #0
	cset w0, eq
	mov x1, x1

	ENDP


|nseel_asm_if| PROC


	mov x1, x1
	stp fp, lr, [sp, #-16]!
	mov fp, sp
	mov x1, 0xdead
	movk x1, 0xbeef, lsl 16
	movk x1, 0xbeef, lsl 32
	mov x2, 0xdead
	movk x2, 0xbeef, lsl 16
	movk x2, 0xbeef, lsl 32
	cmp w0, #0
	csel x1, x1, x2, ne
	blr x1
	ldp fp, lr, [sp], #16
	mov x1, x1
	ENDP


|nseel_asm_repeat| PROC


	mov x1, x1
	fcvtzs w3, d0
	cmp w3, #0
	ble |rep_lz2|
	[ NSEEL_LOOPFUNC_SUPPORT_MAXLEN > 0
	mov x2, (NSEEL_LOOPFUNC_SUPPORT_MAXLEN & 65535)
	movk x2, (NSEEL_LOOPFUNC_SUPPORT_MAXLEN>>16), lsl 16
	cmp w3, w2
	csel w3, w2, w3, gt
	]
	stp fp, lr, [sp, #-48]!
	mov fp, sp
	str x24, [sp, #16]

	mov x24, 0xdead
	movk x24, 0xbeef, lsl 16
	movk x24, 0xbeef, lsl 32
|rep_lz|
	stp x3, x22, [sp, #32]
	blr x24
	ldp x3, x22, [sp, #32]
	sub x3, x3, #1
	cmp x3, #0
	bgt |rep_lz|
	ldr x24, [sp, #16]
	ldp fp, lr, [sp], #48
|rep_lz2|
	mov x1, x1

	ENDP

|nseel_asm_repeatwhile| PROC


	mov x1, x1
	[ NSEEL_LOOPFUNC_SUPPORT_MAXLEN > 0
	mov x3, (NSEEL_LOOPFUNC_SUPPORT_MAXLEN & 65535)
	movk x3, (NSEEL_LOOPFUNC_SUPPORT_MAXLEN>>16), lsl 16
	]
	stp fp, lr, [sp, #-48]!
	str x24, [sp, #16]

	mov x24, 0xdead
	movk x24, 0xbeef, lsl 16
	movk x24, 0xbeef, lsl 32
|repw_lz|
	stp x3, x22, [sp, #32]
	blr x24
	ldp x3, x22, [sp, #32]
	cmp w0, #0
	[ NSEEL_LOOPFUNC_SUPPORT_MAXLEN > 0
	beq |repw_lz2|
	sub x3, x3, #1
	cmp x3, #0
	bne |repw_lz|
|repw_lz2|
	]
	[ NSEEL_LOOPFUNC_SUPPORT_MAXLEN == 0
	bne |repw_lz|
	]
	ldr x24, [sp, #16]
	ldp fp, lr, [sp], #48

	mov x1, x1

	ENDP


|nseel_asm_band| PROC


	mov x1, x1
	cmp w0, #0
	beq |band_chk|
	mov x3, 0xdead
	movk x3, 0xbeef, lsl 16
	movk x3, 0xbeef, lsl 32
	stp fp, lr, [sp, #-16]!
	mov fp, sp
	blr x3
	ldp fp, lr, [sp], #16
|band_chk|
	mov x1, x1
	ENDP

|nseel_asm_bor| PROC


	mov x1, x1
	cmp w0, #0
	bne |bor_chk|
	mov x3, 0xdead
	movk x3, 0xbeef, lsl 16
	movk x3, 0xbeef, lsl 32
	stp fp, lr, [sp, #-16]!
	mov fp, sp
	blr x3
	ldp fp, lr, [sp], #16
|bor_chk|
	mov x1, x1
	ENDP


|nseel_asm_equal| PROC


	mov x1, x1
	ldr d2, [x21]
	fsub d0, d0, d1
	fabs d0, d0
	fcmp d0, d2
	cset w0, lt
	mov x1, x1

	ENDP

|nseel_asm_equal_exact| PROC


	mov x1, x1
	fcmp d1, d0
	cset w0, eq
	mov x1, x1

	ENDP


|nseel_asm_notequal_exact| PROC


	mov x1, x1
	fcmp d1, d0
	cset w0, ne
	mov x1, x1

	ENDP




|nseel_asm_notequal| PROC


	mov x1, x1
	ldr d2, [x21]
	fsub d0, d0, d1
	fabs d0, d0
	fcmp d0, d2
	cset w0, ge
	mov x1, x1

	ENDP



|nseel_asm_below| PROC


	mov x1, x1
	fcmp d1, d0
	cset w0, lt
	mov x1, x1

	ENDP


|nseel_asm_beloweq| PROC


	mov x1, x1
	fcmp d1, d0
	cset w0, le
	mov x1, x1

	ENDP



|nseel_asm_above| PROC


	mov x1, x1
	fcmp d1, d0
	cset w0, gt
	mov x1, x1

	ENDP

|nseel_asm_aboveeq| PROC


	mov x1, x1
	fcmp d1, d0
	cset w0, ge
	mov x1, x1

	ENDP



|nseel_asm_min| PROC


	mov x1, x1
	ldr d0, [x0]
	ldr d1, [x1]
	fcmp d1, d0
	csel x0, x1, x0, lt
	mov x1, x1

	ENDP

|nseel_asm_max| PROC


	mov x1, x1
	ldr d0, [x0]
	ldr d1, [x1]
	fcmp d1, d0
	csel x0, x1, x0, gt
	mov x1, x1

	ENDP



|nseel_asm_min_fp| PROC


	mov x1, x1
	fcmp d1, d0
	fcsel d0, d1, d0, lt
	mov x1, x1

	ENDP

|nseel_asm_max_fp| PROC


	mov x1, x1
	fcmp d1, d0
	fcsel d0, d1, d0, gt
	mov x1, x1

	ENDP







|_asm_generic3parm| PROC


	mov x1, x1
	stp fp, lr, [sp, #-16]! ; input: r0 last, r1=second to last, r2=third to last
	mov fp, sp

	mov x3, x0 ; r0 (last parameter) -> r3

	mov x0, 0xdead ; r0 is first parm (context)
	movk x0, 0xbeef, lsl 16
	movk x0, 0xbeef, lsl 32

	mov x4, 0xdead
	movk x4, 0xbeef, lsl 16
	movk x4, 0xbeef, lsl 32

	mov x5, x1 ; swap x1/x2
	mov x1, x2
	mov x2, x5

	blr x4
	ldp fp, lr, [sp], #16
	mov x1, x1

	ENDP

|_asm_generic3parm_retd| PROC


	mov x1, x1
	stp fp, lr, [sp, #-16]! ; input: r0 last, r1=second to last, r2=third to last
	mov fp, sp
	mov x3, x0 ; r0 (last parameter) -> r3

	mov x0, 0xdead  ; r0 is first parm (context)
	movk x0, 0xbeef, lsl 16
	movk x0, 0xbeef, lsl 32

	mov x4, 0xdead
	movk x4, 0xbeef, lsl 16
	movk x4, 0xbeef, lsl 32

	mov x5, x1 ; // swap x1/x2
	mov x1, x2
	mov x2, x5

	blr x4
	ldp fp, lr, [sp], #16
	mov x1, x1

	ENDP


|_asm_generic2parm| PROC


	mov x1, x1
	stp fp, lr, [sp, #-16]! ;  input: r0 last, r1=second to last
	mov fp, sp
	mov x2, x0

	mov x0, 0xdead ;  r0 is first parm (context)
	movk x0, 0xbeef, lsl 16
	movk x0, 0xbeef, lsl 32

	mov x4, 0xdead
	movk x4, 0xbeef, lsl 16
	movk x4, 0xbeef, lsl 32

	blr x4
	ldp fp, lr, [sp], #16
	mov x1, x1

	ENDP


|_asm_generic2parm_retd| PROC


	mov x1, x1
	stp fp, lr, [sp, #-16]! ; input: r0 last, r1=second to last
	mov fp, sp
	mov x2, x0

	mov x0, 0xdead ; r0 is first parm (context)
	movk x0, 0xbeef, lsl 16
	movk x0, 0xbeef, lsl 32

	mov x4, 0xdead
	movk x4, 0xbeef, lsl 16
	movk x4, 0xbeef, lsl 32

	blr x4
	ldp fp, lr, [sp], #16
	mov x1, x1

	ENDP

|_asm_generic2xparm_retd| PROC


	mov x1, x1
	stp fp, lr, [sp, #-16]! ; input: r0 last, r1=second to last
	mov fp, sp
	mov x2, x1
	mov x3, x0

	mov x0, 0xdead ; r0 is first parm (context)
	movk x0, 0xbeef, lsl 16
	movk x0, 0xbeef, lsl 32

	mov x1, 0xdead ; second parm
	movk x1, 0xbeef, lsl 16
	movk x1, 0xbeef, lsl 32

	mov x4, 0xdead
	movk x4, 0xbeef, lsl 16
	movk x4, 0xbeef, lsl 32

	blr x4
	ldp fp, lr, [sp], #16
	mov x1, x1

	ENDP


|_asm_generic1parm| PROC


	mov x1, x1
	stp fp, lr, [sp, #-16]!
	mov fp, sp
	mov x1, x0

	mov x0, 0xdead ; r0 is first parm (context)
	movk x0, 0xbeef, lsl 16
	movk x0, 0xbeef, lsl 32

	mov x4, 0xdead
	movk x4, 0xbeef, lsl 16
	movk x4, 0xbeef, lsl 32

	blr x4
	ldp fp, lr, [sp], #16
	mov x1, x1

	ENDP



|_asm_generic1parm_retd| PROC


	mov x1, x1
	stp fp, lr, [sp, #-16]!
	mov fp, sp
	mov x1, x0

	mov x0, 0xdead ; // r0 is first parm (context)
	movk x0, 0xbeef, lsl 16
	movk x0, 0xbeef, lsl 32

	mov x4, 0xdead
	movk x4, 0xbeef, lsl 16
	movk x4, 0xbeef, lsl 32

	blr x4
	ldp fp, lr, [sp], #16

	mov x1, x1

	ENDP




|_asm_megabuf| PROC


	mov x1, x1
	add x0, x20, #-8
	ldr d1, [x0]
	fadd d0, d0, d1
	fcvtzu w3, d0
	asr w2, w3, (NSEEL_RAM_ITEMSPERBLOCK_LOG2 - 3)
	bic w2, w2, #7 ;  r2 is page index*8
	mov w0, (NSEEL_RAM_BLOCKS * 8)
	cmp w2, w0
	bhs |mbchk1|

	add x2, x2, x20
	ldr x2, [x2]
	cmp x2, #0
	beq |mbchk1|

	mov x0, (NSEEL_RAM_ITEMSPERBLOCK - 1)
	and x3, x3, x0 ;  r3 mask item in slot
	add x0, x2, x3, lsl #3 ;  set result
	b |mbchk2|
|mbchk1|


	mov x2, 0xdead
	movk x2, 0xbeef, lsl 16
	movk x2, 0xbeef, lsl 32
	stp fp, lr, [sp, #-16]!
	mov fp, sp
	mov x0, x20 ;  first parameter: blocks
	mov x1, x3  ;  second parameter: slot index
	blr x2
	ldp fp, lr, [sp], #16
|mbchk2|

	mov x1, x1

	ENDP


|_asm_gmegabuf| PROC


	mov x1, x1
	add x2, x20, #-8
	ldr d1, [x2]

	mov x0, 0xdead
	movk x0, 0xbeef, lsl 16
	movk x0, 0xbeef, lsl 32

	mov x2, 0xdead
	movk x2, 0xbeef, lsl 16
	movk x2, 0xbeef, lsl 32

	fadd d0, d0, d1

	fcvtzu w1, d0

	stp fp, lr, [sp, #-16]!
	mov fp, sp

	blr x2

	ldp fp, lr, [sp], #16

	mov x1, x1

	ENDP


|nseel_asm_fcall| PROC


	mov x1, x1
	mov x0, 0xdead
	movk x0, 0xbeef, lsl 16
	movk x0, 0xbeef, lsl 32
	stp fp, lr, [sp, #-16]!
	mov fp, sp
	blr x0
	ldp fp, lr, [sp], #16
	mov x1, x1

	ENDP



|nseel_asm_stack_push| PROC


	mov x1, x1
	ldr d0, [x0]

	mov x3, 0xdead ; r3 is stack
	movk x3, 0xbeef, lsl 16
	movk x3, 0xbeef, lsl 32
	ldr x0, [x3]

	add x0, x0, #8

	mov x2, 0xdead
	movk x2, 0xbeef, lsl 16
	movk x2, 0xbeef, lsl 32
	and x0, x0, x2
	mov x2, 0xdead
	movk x2, 0xbeef, lsl 16
	movk x2, 0xbeef, lsl 32
	orr x0, x0, x2

	str x0, [x3]
	str d0, [x0]

	mov x1, x1

	ENDP

|nseel_asm_stack_pop| PROC


	mov x1, x1
	mov x3, 0xdead ;  r3 is stack
	movk x3, 0xbeef, lsl 16
	movk x3, 0xbeef, lsl 32
	ldr x1, [x3]
	ldr d0, [x1]
	sub x1, x1, #8
	mov x2, 0xdead
	movk x2, 0xbeef, lsl 16
	movk x2, 0xbeef, lsl 32
	str d0, [x0]
	and x1, x1, x2
	mov x2, 0xdead
	movk x2, 0xbeef, lsl 16
	movk x2, 0xbeef, lsl 32
	orr x1, x1, x2

	str x1, [x3]
	mov x1, x1

	ENDP



|nseel_asm_stack_pop_fast| PROC


	mov x1, x1
	mov x3, 0xdead ; r3 is stack
	movk x3, 0xbeef, lsl 16
	movk x3, 0xbeef, lsl 32
	ldr x1, [x3]
	mov x0, x1
	sub x1, x1, #8
	mov x2, 0xdead
	movk x2, 0xbeef, lsl 16
	movk x2, 0xbeef, lsl 32
	and x1, x1, x2
	mov x2, 0xdead
	movk x2, 0xbeef, lsl 16
	movk x2, 0xbeef, lsl 32
	orr x1, x1, x2
	str x1, [x3]
	mov x1, x1

	ENDP

|nseel_asm_stack_peek| PROC


	mov x1, x1
	mov x3, 0xdead ; r3 is stack
	movk x3, 0xbeef, lsl 16
	movk x3, 0xbeef, lsl 32

	fcvtzs w2, d0

	ldr x1, [x3]
	sub x1, x1, x2, lsl #3
	mov x2, 0xdead
	movk x2, 0xbeef, lsl 16
	movk x2, 0xbeef, lsl 32
	and x1, x1, x2
	mov x2, 0xdead
	movk x2, 0xbeef, lsl 16
	movk x2, 0xbeef, lsl 32
	orr x0, x1, x2
	mov x1, x1

	ENDP


|nseel_asm_stack_peek_top| PROC


	mov x1, x1
	mov x3, 0xdead ; r3 is stack
	movk x3, 0xbeef, lsl 16
	movk x3, 0xbeef, lsl 32
	ldr x0, [x3]
	mov x1, x1

	ENDP


|nseel_asm_stack_peek_int| PROC


	mov x1, x1
	mov x3, 0xdead ; r3 is stack
	movk x3, 0xbeef, lsl 16
	movk x3, 0xbeef, lsl 32

	mov x2, 0xdead
	movk x2, 0xbeef, lsl 16
	movk x2, 0xbeef, lsl 32

	ldr x1, [x3]
	sub x1, x1, x2
	mov x2, 0xdead
	movk x2, 0xbeef, lsl 16
	movk x2, 0xbeef, lsl 32
	and x1, x1, x2
	mov x2, 0xdead
	movk x2, 0xbeef, lsl 16
	movk x2, 0xbeef, lsl 32
	orr x0, x1, x2
	mov x1, x1

	ENDP

|nseel_asm_stack_exch| PROC


	mov x1, x1
	mov x3, 0xdead ; r3 is stack
	movk x3, 0xbeef, lsl 16
	movk x3, 0xbeef, lsl 32
	ldr x1, [x3]
	ldr d0, [x0]
	ldr d1, [x1]
	str d0, [x1]
	str d1, [x0]
	mov x1, x1

	ENDP


|nseel_asm_booltofp| PROC


	mov x1, x1
	cmp w0, #0
	fmov d0, #1.0
	movi d1, #0
	fcsel d0, d0, d1, ne
	mov x1, x1

	ENDP

|nseel_asm_fptobool| PROC


	mov x1, x1
	ldr d1, [x21]
	fabs d0, d0
	fcmp d0, d1
	cset w0, ge
	mov x1, x1

	ENDP

|nseel_asm_fptobool_rev| PROC


	mov x1, x1
	ldr d1, [x21]
	fabs d0, d0
	fcmp d0, d1
	cset w0, lt
	mov x1, x1

	ENDP

|eel_callcode64| PROC
	stp	fp, lr, [sp, #-128]!
	mov	fp, sp

	stp	d13, d12, [sp, #16]
	stp	d11, d10, [sp, #32]
	stp	d9, d8, [sp, #48]
	stp	d15, d14, [sp, #64]
	stp	x20, x19, [sp, #80]
	stp	x18, x21, [sp, #96]
	stp	x22, x23, [sp, #112]

	mov	x19, x0
	mov	x20, x2
	mov	x21, x3
	blr	x1

	ldp	d13, d12, [sp, #16]
	ldp	d11, d10, [sp, #32]
	ldp	d9, d8, [sp, #48]
	ldp	d15, d14, [sp, #64]
	ldp	x20, x19, [sp, #80]
	ldp	x18, x21, [sp, #96]
	ldp	x22, x23, [sp, #112]
	ldp	fp, lr, [sp], #128
	ret

	ENDP

|glue_getscr| PROC
	mrs	x0, FPCR
	ret
	ENDP

|glue_setscr| PROC
	msr	FPCR, x0
	ret
	ENDP

	END
