# System information
The (Name pending) is a 10Mhz single-cycle Risc-V processor that was designed as a part of the SSCS Chipathon proyect, fabricated in Skywater 130 technology.

## Comunication 

The comunication with the processor was made throught a UART Controller with the following parameters:

| Parameter | Value          |
|-----------|----------------|
| Baud Rate | 115200         |
| Parity    | No             |
| Bits      | 8              |
| Stop bit  | 1              |

The communication is handled by the following FSM.

(Image pending)

But to simplify it we'll divide it into the following sequences:

- Restarting the system.
- Getting the previous program counter.
- Sending a non-Memory instruction.
- Sending a Memory instruction.

## Restarting the system

(Image pending)
## Getting the previous program counter

(Image pending)
## Sending a non-Memory instruction
(Image pending)
## Sending a Memory instruction
(Image pending)