# Perform uf8_encode and uf8_decode and CLZ in rv32i
.section .text
# Function: CLZ
# Description: Counts the number of leading zero bits in a 32-bit unsigned integer
#              using binary search algorithm
# 
# Input:
#   a0 - 32-bit unsigned integer value to count leading zeros
#
# Output:
#   a0 - Number of leading zero bits (0-32)
CLZ:
    li t0, 32 #n
    li t1, 16 #c
do_while:
    srl t2, a0, t1 #t2 is y
    beq t2, x0, SHIFT_TOO_MUCH
    sub t0, t0, t1
    add a0, t2, x0
SHIFT_TOO_MUCH: 
    srli t1, t1, 1
    bne t1, x0, do_while
    sub a0, t0, a0
    jr ra   # jump to ra

# Function: uf8_decode
# Description: Decodes an 8-bit UF8 (micro-float 8) encoded value into 
#              a 20-bit unsigned integers
#
# UF8 Format (8 bits):
#   [7:4] - exponent (4 bits)
#   [3:0] - mantissa (4 bits)
#
# Input:
#   a0 - 8-bit UF8 encoded value
#
# Output:
#   a0 - 20-bit decoded unsigned integers
.globl uf8_decode
.type uf8_decode,%function
.align 2   
uf8_decode:
    andi t0, a0, 0x0F # mantissa
    srli t1, a0, 4 # exponent
    li t2, 15
    sub t2, t2, t1 # 15 - exponent
    li t3, 0x7FFF
    srl t3, t3, t2
    slli t3, t3, 4 # offset
    sll t2, t0, t1
    add a0, t2, t3
    jr ra   # jump to ra
.size uf8_decode,.-uf8_decode

# Function: uf8_encode
# Description: Encodes a 20-bit unsigned integers into an 8-bit UF8 
#
# UF8 Format (8 bits):
#   [7:4] - exponent (4 bits)
#   [3:0] - mantissa (4 bits)
#
# Input:
#   a0 - 20-bit unsigned integers to encode
#
# Output:
#   a0 - 8-bit UF8 encoded value
.globl uf8_encode
.type uf8_encode,%function
.align 2
uf8_encode:
    addi sp, sp, -4
    sw ra, 0(sp) # because it will call CLZ in this function
    add t6, a0, x0 # value
    li t0, 16
    blt t6, t0, RETURN # if value < 16
    jal ra, CLZ # call clz
    li t0, 31
    sub t0, t0, a0 # msb,  a0 is lz(return value from CLZ)
    add t1, a0, x0 # lz

    li t2, 0 # exponent # Start from a good initial guess
    li t3, 0 # overflow
    li t4, 5
    blt t0, t4, Find_exact_exponent # go to Find_exact_exponent
    addi t2, t0, -4
    li t4, 15
    bge t4, t2, Cal_overflow # if 15 >= exponent
    li t2, 15 # exponent is 15
Cal_overflow:
    li t4, 0 # counter
For_1:
    slli t5, t3, 1
    addi t3, t5, 16 # overflow = (overflow << 1) + 16;
    addi t4, t4, 1
    blt t4, t2, For_1
while_1:
    blez t2, Find_exact_exponent
    bge  t6, t3, Find_exact_exponent
    addi t5, t3, -16 
    srli t3, t5, 1 # overflow = (overflow - 16) >> 1;
    addi t2, t2, -1
    j while_1
Find_exact_exponent:    
    li t5, 15
while_2: 
    bge t2, t5, PRE_RETURN
    slli t4, t3, 1
    addi t4, t4, 16 # next_overflow = (overflow << 1) + 16;
    blt t6, t4, PRE_RETURN
    add t3, t4, x0
    addi t2, t2, 1
    j while_2
PRE_RETURN:
    sub t1, t6, t3
    srl t1, t1, t2 # mantissa
    slli t0, t2, 4
    or a0, t0, t1 # prepare return value(a0)
RETURN:
    lw ra, 0(sp)
    addi sp, sp, 4
    jr ra   # jump to ra
.size uf8_encode,.-uf8_encode
