
.globl start_image

.thumb_func
start_image:
	mov sp, r1
	bx r0
	b .
