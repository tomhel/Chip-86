# Chip-86

Chip-86 is a Chip-8 emulator using dynamic translation to x86. Created in C++ by Tommy Hellstrom at the University of Gavle, Sweden, 2009.

##License

GPLv3

##Installation

todo

##Usage

The emulator is run from CLI. Run it without arguments to display help.

```
chip86 <file> <speed> [tune]
```

Arg | - | Description
--- | --- | ---
file | required | The Chip-8 application (rom).
speed | required | Emulation speed, lower equals higher speed. Good values are 5-20.
tune | optional | Emulation speed and smoothness control. Good values are 5-20.

If you are unsure about the speed and tune argument, 10 10 are good values to start at.

###Example

```
chip86 test/count 5
```

##Keys

Key | Description
--- | ---
PAGE DOWN | Decrease emulation speed. (Modifies speed argument).
PAGE UP | Increase emulation speed. (Modifies speed argument).
HOME | Emulation speed and smoothness control, increase. (Modifies tune argument).
END  | Emulation speed and smoothness control, decrease. (Modifies tune argument).
X | CHIP-8 key 0
1 | CHIP-8 key 1
2 | CHIP-8 key 2
3 | CHIP-8 key 3
Q | CHIP-8 key 4
W | CHIP-8 key 5
E | CHIP-8 key 6
A | CHIP-8 key 7
S | CHIP-8 key 8
D | CHIP-8 key 9
Z | CHIP-8 key A
C | CHIP-8 key B
4 | CHIP-8 key C
R | CHIP-8 key D
F | CHIP-8 key E
V | CHIP-8 key F

##Testing

The test applications found in the test directory are written by Tommy Hellstrom. They are free to use and (re)distribute under GPLv3 License.

- flag1

        Validate flag when adding two registers.
        Expected output (5 first): 0, 1, 0, 0, 0

- flag2

        Validate flag when subtracting two registers.
        Expected output (5 first): 1, 0, 1, 1, 1

- flag3

        Validate flag on right shift.
        Expected output (5 first): 0, 1, 1, 1, 1
    
- flag4

        Validate flag on left shift.
        Expected output (5 first): 0, 1, 1, 1, 1
    
- count

        Counts from 0 to 9, and starts then over, in an infinite loop.
        Expected output: A count from 0 to 9, then restarts at 0.

- bsort

        Sorts 3060 integers, 0-255, using Bubblesort. This application does not draw to screen.
        Upon completion it will enter an infinite idle loop at address 230h.
        To validate the output a memory dump is needed, starting at address 23Eh.
        Expected output: A sorted sequence of numbers (in memory).

##Games

Use your prefered search engine ;)

##How it works

todo
