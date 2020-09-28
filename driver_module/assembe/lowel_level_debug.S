FUNC c_debug, :
	mov x3, #60
	mov x1, x0
c_loop:
	mov x1, x0
	lsr x1, x1, x3
	mov x7, x1
	and x7, x7, #0xf
	cmp x7, #9
	b.gt c1
	add w7, w7, #'0'
	b    c2
c1:
	sub w7, w7, #10 
	add w7, w7, #'a'
c2:
	ldr x8, =CONSOLE_UART_BASE
	str w7, [x8, #UARTDR]

	sub x3, x3, #4
	cmp x3, #0
	b.ge c_loop

	ret
END_FUNC c_debug
