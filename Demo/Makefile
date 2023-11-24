all: Demo Demo.S Demo.bin

clean:
	rm -f Demo Demo.o Demo.S Demo.bin crt0.o

Demo.o: Demo.cpp
	riscv64-unknown-elf-gcc -g -ffreestanding -O0 -nostartfiles -nostdlib -nodefaultlibs -march=rv32i -mabi=ilp32 -c Demo.cpp -o Demo.o

Demo.S: Demo
	riscv64-unknown-elf-objdump -d Demo >Demo.S

crt0.o: crt0.s
	riscv64-unknown-elf-gcc -g -ffreestanding -O0 -nostartfiles -nostdlib -nodefaultlibs -march=rv32i -mabi=ilp32 -c crt0.s -o crt0.o

Demo: Demo.o crt0.o
	riscv64-unknown-elf-ld -m elf32lriscv --gc-sections -T riscv32.ld crt0.o Demo.o -o Demo

Demo.bin: Demo
	riscv64-unknown-elf-objcopy -O binary Demo Demo.bin



# riscv64-unknown-elf-gcc -g -ffreestanding -O0 -Wl,--gc-sections -nostartfiles -nostdlib -nodefaultlibs -Wl,-T,riscv64.ld test.cpp -o test
