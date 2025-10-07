# Risc-V Testing app

A QT-based application for debugging a Risc-V core that uses uart as its communication method.

# Documentation 

## Reset
When the device is reset, it'll send 8'b1 throught uart to confirm its active.

## Comunication
Once the device is ready for use, it will enter the CPU_READY State, where depending on what is the next recieved byte it will:

- Reset if the recieved byte is a 1.
- It will send the previous Program Counter if the recieved byte is a 2.
- If its anything else it will await an Instruction.