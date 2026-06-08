;; If you can't make inline shared library calls with your C compiler, you can
;; use glue routines.  Just assemble this file with whatever assembler is right
;; for your linker, using small or large data as appropriate for the program
;; you're going to link with.  This assumes that the assembler is capable of
;; translating an external data symbol into a small data register-relative
;; reference.  With these glue routines, the arguments must be pushed onto
;; the stack as longwords, with the last argument pushed first.  The function
;; FSRexxQuery will be left out unless you define the word REXXQUERY, because
;; at present that function in the library only returns 1 and does nothing.


;;;;		include		"fastscan.i"		; _LVO definitions
;;;; heck, put em inline:

_LVOFSRexxQuery         equ     -30
_LVOFastExamine         equ     -36
_LVOFastExNext          equ     -42
_LVOFastExCleanup       equ     -48
_LVOFastExGet80         equ     -54


		xdef		_FastExamine
		xdef		_FastExNext
		xdef		_FastExCleanup
		xdef		_FastExGet80

		section		code

_FastExamine	movem.l		4(sp),d0/a0
		move.l		a6,-(sp)
		move.l		_FastScanBase,a6
		jsr		_LVOFastExamine(a6)
		move.l		(sp)+,a6
		rts

_FastExNext	movem.l		4(sp),d0/a0
		move.l		a6,-(sp)
		move.l		_FastScanBase,a6
		jsr		_LVOFastExNext(a6)
		move.l		(sp)+,a6
		rts

_FastExCleanup	move.l		4(sp),a0
		move.l		a6,-(sp)
		move.l		_FastScanBase,a6
		jsr		_LVOFastExCleanup(a6)
		move.l		(sp)+,a6
		rts

_FastExGet80	move.l		4(sp),a0
		move.l		a6,-(sp)
		move.l		_FastScanBase,a6
		jsr		_LVOFastExGet80(a6)
		move.l		(sp)+,a6
		rts


	IFD		REXXQUERY

		xdef		_FSRexxQuery

_FSRexxQuery	move.l		4(sp),a0
		move.l		a6,-(sp)
		move.l		_FastScanBase,a6
		jsr		_LVOFSRexxQuery(a6)
		move.l		(sp)+,a6
		rts

	ENDC


		section		data

		xdef		_FastScanBase
_FastScanBase	dc.l		0
