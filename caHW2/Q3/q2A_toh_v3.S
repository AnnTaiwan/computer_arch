# Perform the tower of Hanoi.
#   Disks: 0,1,2 (0 is the smallest one)
#   Peg: A,B,C (Try to move all disks from peg A to C)
# Here is the adapted code from quiz 2 - A.
# Defined constants first
.equ STDOUT, 1
.equ WRITE, 64
.equ EXIT, 93

.text
.globl play_toh_v3
.type play_toh_v3,%function
.align 2
play_toh_v3:
    addi sp, sp, -24
    sw x0, 0(sp) # disk 0's peg number
    sw x0, 4(sp) # disk 1's peg number
    sw x0, 8(sp) # disk 2's peg number
    sw ra, 12(sp) # ra
    sw s0, 16(sp)
    sw s1, 20(sp)
    # s0: n
    # s1: store previous calculated gray(n-1)
    li s1, 0 # gray(0)
    li t5, 8 # gameloop end situation
    li t6, 3 # constant
    addi s0, x0, 1 # for loop counter, and it is also n in gray(n)
game_loop:
    beq t5, s0, finish_game
    # Gray code formula: gray(n) = n XOR (n >> 1)
    srli t0, s0, 1
    xor a1, t0, s0 # gray(n)
    
    li a5, 0 # try to move which disk 0,1,2
    xor a3, a1, s1 # diff_bit_pos

    mv s1, a1 # update gray(n-1) for next round

    andi t0, a3, 1 # try to move smallest disk
    beqz t0, handle_large
    # handle_smallest
    lw a6, 0(sp) # disk 0's current peg num
    # find target disk: current disk idx +2 -3
    addi a4, a6, 2
    blt a4, t6, display_action
    sub a4, a4, t6 # -3
    jal x0, display_action
handle_large:
    addi a5, a5, 4 # middle disk 1, a5 now increases by 4, in order to align with sp[0], sp[4], sp[8]
    andi t0, a3, 0x2 # bit flip 010: this is bit 1 flip
    bnez t0, found_disk
    addi a5, a5, 4 # largest disk 2
found_disk:
    add t1, sp, a5
    lw a6, 0(t1) # disk idx's current peg num
    # find target disk: 3-(self peg + smallest disk's peg)
    lw t2, 0(sp) # smallest disk's peg
    sub a4, t6, a6
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
    addi t0, a5, 4              
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
    add t0, sp, a5
    sw a4, 0(t0)

    # Increment counter and loop
    addi    s0, s0, 1
    jal     x0, game_loop
finish_game:
    lw ra, 12(sp) # ra
    lw s0, 16(sp)
    lw s1, 20(sp)
    addi sp, sp, 24

    li a0, 0 # return 0 if successful exit
    jr ra

    .data
peg:        .byte   65, 66, 67 # ascii code: 'A' 'B' 'C'
disk:       .byte   48, 0, 0, 0, 49, 0, 0, 0, 50, 0, 0, 0, 51 # ascii code: '0' '1' '2' '3', 0 is buffer, not used
str1:       .ascii  "Move Disk "
str2:       .ascii  " from "
str3:       .ascii  " to "
nextline:   .ascii  "\n"
