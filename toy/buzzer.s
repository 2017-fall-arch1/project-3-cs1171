	.arch msp430
	.p2align 1,0

	.file "buzzer.c"

	.text
	.word MIN_PERIOD 1000
	.word MAX_PERIOD 4000
	.word period 1000
	.word rate 200

	.global buzz0
buzz0:	call #0,buzzer_set_period

	.global buzz
buzz:	call buzzer_advance_frequency

	.global buzzer_set_period

buzzer_set_period:
	mv r12, r2
	RRA r2
	mv r2, r12
	pop r0
	
buzzer_advance_frequency:
	add &rate, &period

if1:	cmp #0, &rate
	jle buzzer_set_period
if2:	cmp &MAX_PERIOD, &period
	jle buzzer_set_period
if3:	cmp #0, &rate
	jge buzzer_set_period
if4:	cmp &MIN_PERIOD, &period
	jge buzzer_set_period

	xor &rate, &rate
	add &rate, &rate
	add &rate, &period

	call #0,buzzer_set_period
	
	.global buzzer_init()

buzzer_init:
	call timerAUpmode
	mv &BIT7, r13
	bis &BIT6, r13
	xor r13, r13
	and r13, &P2SEL2
	mv &BIT7, r13
	xor r13, r13
	and r13, &P2SEL
	bis &BIT6, &P2SEL
	mv &BIT6, &P2DIR
	mv &P2DIR, r12
	pop 0
