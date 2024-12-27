#if __riscv_xlen == 64
	# define LREG ld
	# define SREG sd
	# define REGBYTES 8
#else
	# define LREG lw
	# define SREG sw
	# define REGBYTES 4
#endif

	.section .text.entry
	.global _start
_start:
	lla sp, stack_end
	
    lla t0, early_exc_entry
    csrw mtvec, t0
	csrw mcause, x0
	csrw mtval, x0

        /* no global variables or static variables are allowed in loader_main and its called sub functions */
        /* TODO: take care of the stack size usage, if your function and subfunction are complex and use many variable */
	jal loader_main

	ebreak

/* Early boot exception entry before main */
.align 6
.global early_exc_entry
.type early_exc_entry, @function
early_exc_entry:
    wfi
    j early_exc_entry

	.section .data
	.balign REGBYTES
stack:
        /* TODO: Change 128 to bigger size if your code is complex and workarea is big enough */
        /* Otherwise it may crupt normal text section code */
	.fill 128, REGBYTES, 0x8675309
stack_end:
