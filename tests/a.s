.equ term_in, 0xFFFFFF04
.equ term_out, 0xFFFFFF00

.section text

main:
    ld $handler, %r1
    csrwr %r1, %handler

loop:
    jmp loop

handler:
    ld term_in, %r1
    st %r1, term_out
    iret

.end