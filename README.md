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

### Building the Demo

    cd /path/to/RISC-V/Demo
    make

This should create Demo.bin.

### Invoking RISC-V-Emulator with the Demo

    cd /path/to/RISC-V/
    ./build/RISC-V-Emulator ./Demo/Demo.bin

That's it.
