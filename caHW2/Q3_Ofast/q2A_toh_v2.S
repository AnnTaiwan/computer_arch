# Perform the tower of Hanoi.
#   Disks: 0,1,2 (0 is the smallest one)
#   Peg: A,B,C (Try to move all disks from peg A to C)
# Here is the adapted code from quiz 2 - A.
# Defined constants first
.equ STDOUT, 1
.equ WRITE, 64
.equ EXIT, 93

.text
.globl play_toh_v2
.type play_toh_v2,%function
.align 2
play_toh_v2:
    addi sp, sp, -20
    sw x0, 0(sp) # disk 0's peg number
    sw x0, 4(sp) # disk 1's peg number
    sw x0, 8(sp) # disk 2's peg number
    sw ra, 12(sp) # ra
    sw s0, 16(sp)
    addi s0, x0, 1 # for loop counter, and it is also n in gray(n)
game_loop:
    li t0, 8
    beq t0, s0, finish_game
    # Gray code formula: gray(n) = n XOR (n >> 1)
    srli t0, s0, 1
    xor a1, t0, s0 # gray(n)

    addi t0, s0, -1 # n-1
    srli t1, t0, 1
    xor a2, t0, t1 # gray(n-1)

    li a5, 0 # try to move which disk 0,1,2
    xor a3, a1, a2 # diff_bit_pos
    andi t0, a3, 1 # try to move smallest disk
    beqz t0, handle_large
    # handle_smallest
    lw a6, 0(sp) # disk 0's current peg num
    # find target disk: current disk idx +2 -3
    addi a4, a6, 2
    li t0, 3
    blt a4, t0, display_action
    sub a4, a4, t0 # -3
    jal x0, display_action
handle_large:
    addi a5, a5, 1 # middle disk 1
    srli a3, a3, 1
    andi t0, a3, 1
    bnez t0, found_disk
    addi a5, a5, 1 # largest disk 2
found_disk:
    slli t0, a5, 2 # see idx in sp, disk idx * 4
    add t1, sp, t0
    lw a6, 0(t1) # disk idx's current peg num
    # find target disk: 3-(self peg + smallest disk's peg)
    lw t2, 0(sp) # smallest disk's peg
    li a4, 3
    sub a4, a4, a6
    sub a4, a4, t2
    jal x0, display_action
display_action:
    # Move disk a5 from a6 to a4
    li a7, WRITE                 
    li a0, STDOUT # stdout   
    la a1, str1                  
    li a2, 10 #length character 
    ecall

    li a7, WRITE                 
    li a0, STDOUT # stdout     
    la a1, disk   
    addi t0, a5, 1              
    add a1, a1, t0
    li a2, 1 #length character 
    ecall

    li a7, WRITE                 
    li a0, STDOUT # stdout 
    la a1, str2                  
    li a2, 6 #length character 
    ecall

    li a7, WRITE                 
    li a0, STDOUT # stdout
    la a1, peg                 
    add a1, a1, a6
    li a2, 1 #length character 
    ecall

    li a7, WRITE                 
    li a0, STDOUT # stdout  
    la a1, str3                  
    li a2, 4 #length character 
    ecall

    li a7, WRITE                 
    li a0, STDOUT # stdout 
    la a1, peg                 
    add a1, a1, a4
    li a2, 1 #length character 
    ecall

    li a7, WRITE                 
    li a0, STDOUT # stdout   
    la a1, nextline                  
    li a2, 1 #length character 
    ecall

    # Update disk position ( Move disk a5 from a6 to a4 )
    slli t0, a5, 2 # * 4
    add t0, sp, t0
    sw a4, 0(t0)

    # Increment counter and loop
    addi    s0, s0, 1
    jal     x0, game_loop
finish_game:
    lw ra, 12(sp) # ra
    lw s0, 16(sp)
    addi sp, sp, 20

    li a0, 0 # return 0 if successful exit
    jr ra

    .data
peg:        .byte   65, 66, 67 # ascii code: 'A' 'B' 'C'
disk:       .byte   48, 49, 50, 51 # ascii code: '0' '1' '2' '3'
str1:       .ascii  "Move Disk "
str2:       .ascii  " from "
str3:       .ascii  " to "
nextline:   .ascii  "\n"
