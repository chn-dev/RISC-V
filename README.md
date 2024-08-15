RISC-V-Emulator
===============

This is an emulator of a single-core, 32bit RISC-V CPU with the I (base integer instructions), M (multiplication & division instructions) and A (atomic instructions) extensions, or for short: rv32ima. The emulator has been written for purely educational purposes. Therefore the aim for it is to be accurate, complete and simple.

The RISC-V ISA (instruction set architecture) specifications can be found here:

[Specifications - RISC-V International](https://riscv.org/technical/specifications/)

## Cloning from git and compilation

    git clone https://github.com/chn-dev/RISC-V
    cd RISC-V
    cmake . -B build
    cmake --build build

This builds the executable "RISC-V-Emulator".

RISC-V-Emulator loads the contents of the file given in the first command line parameter as a flat binary and copies it into its virtual RAM. It then starts executing that binary data as RISC-V machine code. The virtual RAM ranges from address 0x80000000 to address 0x07ffffff and is therefore 128MB in size.

## The RISC-V Demo program

In ./RISC-V/Demo/ you can find a small C++ program which can be compiled into RISC-V machine code in a flat binary file. That file can then be executed using RISC-V-Emulator.

The necessary files

 - riscv32.ld: linker script
 - crt0.s for setting up the stack and jumping to the "main" function 
 - Makefile for building
     - Demo.S: the RISC-V assembly code
     - Demo: the Linux executable in ELF format
     - Demo.bin: the flat machine language binary

are all available in ./RISC-V/Demo/. Note that you need to install the riscv64-unknown-elf tools for cross-compilation before building the Demo.

Under Ubuntu, you can use the following commands to install them:

    sudo apt-get install gcc-riscv64-unknown-elf
    sudo apt-get install binutils-riscv64-unknown-elf

### Building the Demo

    cd /path/to/RISC-V/Demo
    make

This should create Demo.bin.

### Invoking RISC-V-Emulator with the Demo program

    cd /path/to/RISC-V/
    ./build/RISC-V-Emulator ./Demo/Demo.bin

That's it. If everything works as intended, the output should look like this:

    chn@chnX200:~/Programming/RISC-V$ ./build/RISC-V-Emulator ./Demo/Demo.bin
    Hello, world!
    I'm running from within RISC-V-Emulator.
    
    -10
    -9
    -8
    -7
    -6
    -5
    -4
    -3
    -2
    -1
    0
    1
    2
    3
    4
    5
    6
    7
    8
    9
    10
    Unknown opcode found at 0x0.

In Emulator.cpp you can see that writing a byte to memory address 0x0 causes the Emulator to output that byte as an ASCII character to its stdout. That's how the Demo program can output its text without any operating system.
