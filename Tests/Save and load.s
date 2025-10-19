addi x5,x0, 56
addi x6,x0, 4
add x7,x5,x6
sw x7,64(x0)
add x5, x6,x7
lw x4,64(x0)
addi x4,x4,0
addi x4,x4,1
sw x4,64(x0)