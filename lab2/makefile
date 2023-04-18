all:
	nasm -f elf32 my_print.asm
	g++ -o fat -m32 main.cpp my_print.o
run: 
	./fat
