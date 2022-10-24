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
	.fill 32, REGBYTES, 0x8675309
stack_end:
