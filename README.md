# Chip-86

Chip-86 is a Chip-8 emulator using dynamic translation (or dynamic recompilation) to x86. Written in C++ at the University of Gavle, Sweden, 2009. Dynamic translation is definitely overkill for Chip-8. It is more of a learning experience and proof of concept.

[Scroll down to see how it works](#how-it-works)

![chip86](chip86.png?raw=true)

##License

GPLv3

##Building

Requires SDL and OpenGL support

1. Install libsdl-dev

    ```
    apt-get install libsdl-dev
    ```

2. Build
    ```
    make
    ```

##Usage

The emulator is run from CLI. Run it without arguments to display help.

```
chip86 <file> <speed> [tune]
```

Argument | - | Description
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

Test applications are found in the test directory. Testing an emulator is hard. These applications test different parts of the emulator.

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

        Expected output: A count from 0 to 9, then restarts at 0

- bsort

        Sorts 3060 integers, 0-255, using Bubblesort. This application does not draw to screen.
        Upon completion it will enter an infinite idle loop at address 230h.
        To validate the output a memory dump is needed, starting at address 23Eh.

        Expected output: A sorted sequence of numbers (in memory)

##Games

Use your prefered search engine ;)

##How it works

### Generate and execute machine code

To generate code dynamically we must know how GNU GCC (the compiler) handle function calls and return values.

It is enough to know that:
- Arguments are pushed on the stack in reverse order.
- All primitive types are returned in the EAX register.

Here is an simplified example of how code can be generated dynamically and executed.

```c
//the code to execute
unsigned char code[] = {
   0xB0,0x04, //mov al 4 (al = 4)
   0xB4,0x02, //mov ah 2 (ah = 2)
   0x02,0xC4, //add al ah (al = al + ah)
   0xC3       //ret (return al)
};

//function pointer to execute the code
char (*dynfunc)();

//allocate memory for the code
dynfunc = (char(*)()) mmap(NULL, sizeof(code), PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

//copy code to allocated memory
memcpy((void*)dynfunc, code, sizeof(code));

//execute the code
int ret = (*dynfunc)();
```

###Concepts

![concepts](concepts.png?raw=true)

####Dispatcher

The purpose of the dispatcher is to control the main flow of the emulator. It will check if code is translated or not. If the code is translated it will be executed. Otherwise, it will be translated.

####Code cache
The translated code is stored in the code cache. 

####Translator

The translator will translate code from the emulated system to native code.

####Code blocks

Translated code is divided into blocks. The size of blocks is important for emulation speed. If the blocks are to small to much time will be spent in the dispatcher.

####Basic blocks

Basic blocks is a common concept in code compilation. A basic block has only one entry point and only one exit point.

###Chip-8 system

This is a brief summary of the Chip-8 system

- Chip-8 has 35 instructions. Every instruction is 2 byte in size.
- Chip-8 has 16 8bit registers and one 16bit address register.
- The stack is limited to 16 levels and is used to store the return address of subrutine calls
- The memory is 3584 byte in size and addressed from 200h to FFFh
- Chip-8 has 2 timers. A sound timer and a delay timer.
- Input is done on 16 keys.
- The display is monochrome and has a resolution of 64 x 32 pixels.
- The sound is very simple, only a beep.

###Code blocks and code generation

In this implementation (Chip-86) code is translated to basic blocks to keep the implementation simple.

Chip-8 has 16 8bit registers and one 16bit address register. The 8bit registers are mapped to the 8bit registers in IA-32 (the native cpu), but because there is only 8 of those and Chip-8 has 16 the implementation uses a simple dynamic register allocation algorithm. If all registers happens to be allocated the least used register will be deallocated. When a register needs to be allocated code is generated to load the value from the cpu context structure. The cpu context structure keeps track of the native cpu state between blocks of code. If a register needs to be deallocated it is saved to this structure. At the end of a code block all registers that are still in use will be saved to this structure.

If the address register is used it will always be allocated to the ESI register. The EDI register is intentionally left free to be used as a temporary register by the generated code.

The code in a block is generated in such a way that it can be called as a regular function. The registers used by the code block is first pushed on the stack and popped back at the end. Each block returns the (Chip-8) address to the next block to be executed. This is a simple solution and it will be left to the dispatcher to execute the next block.

Chip-8 has a register for flags, VF. It will indicate carry on addition and borrow on subtraction. On shift operations VF will contain the lost bit. In this implementation all these flags a computed natively on the cpu, although we will copy the flag to the register where VF is allocated.

Chip-8 has a stack with a maxdepth of 16 to store return addresses. In this implementation the stack is represented by an array and code will be generated to push and pop to this array on Chip-8 Call and Return instructions.

Chip-8 has conditional instructions like:

```
IF something THEN skip next instruction
```

This kind of instruction will always end a block and result in two new blocks being generated. One that we will jump to if the condition is true and one that we go to if the condition is false.

### Code cache

The code cache is where all generated code are stored.

Because Chip-8 has very little memory by todays standards (3584 byte), we will allocate an array to hold all of this memory. This array will hold 4096 elements instead of 3584, the reason for this is that Chip-8 applications is allocated from 0x200 to 0xFFF. Each position in the array can point to a block of translated code.

It is theoretically possible for Chip-8 applications to contain self modifying code. There is one instruction that could be used in this way, FX55. Although, i have not found any Chip-8 applications that does this. So, for this reason this implementation does not handle self modifying code.

###Handling of timers, input and graphics

Chip-8 has 2 timers, one for sound and another for delays. These will decrement towards 0 everytime they are set to a value greater than 0. In the implementation this is done everytime a code block returns to the dispatcher. All graphics and input is also handled by the dispatcher.

Because the implementation is much to fast for Chip-8 applications it has to be slowed down. A simple solution is a delay loop. This loop is placed in the dispatcher and will delay execution of the next block. To get a smooth emulation speed the emulator will do its best to always execute the same number of instructions before returning to the dispatcher.


###Implementation

The implementation defines these classes:
- CodeBlock
- CodeGenerator
- TranslationCache
- Translator
- RegTracker

![uml](uml.png?raw=true)

####CodeBlock class

Every generated code block is contained in a CodeBlock object. This class has a function pointer to call the code and a destructor to deallocate the code.

####CodeGenerator class

This class has an assemply-like interface. Its purpose is to generate native machine code. The Code generator can handle labels and jumps to these labels. Code is generated when the assembly-like functions are called, but this is not true for jumps. Those are generated last.

#### Translator class

The Translator accepts one Chip-8 instruction at a time. The instructions are decoded and added to a linked list, where each element is a Chip-8 instruction. When the Translator has found a basic block the translation to machine code begins. The translator has a function for each Chip-8 instruction that generates the corresponding machine code. During this process the register allocation is done using a RegTracker object.

#### TranslationCache class

The Translation cache maps the whole memory area of Chip-8 using an array. The array stores pointers to CodeBlock objects. The Translation cache accepts a Chip-8 address and simply executes the code block on that address by calling its function pointer. The Translation cache returns true if the block is found or false otherwise.

#### RegTracker class

This class keeps track of the register mapping between the native cpu and the Chip-8 cpu. When a register needs to be allocated the Translator asks the RegTracker for a register. The RegTracker will handle the code generation that is needed for this. The RegTracker will also generate the code necessary to store registers to the cpu context structure at the end of a block. It will also keep track of the registers used within a block of code and generate code for these registers to be pushed on the stack before use. At the end of a block it will add code to pop these values back.
