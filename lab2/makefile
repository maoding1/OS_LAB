all:
	nasm -f elf32 my_print.asm
	g++ -o main -m32 -w main.cpp my_print.o
run: 
	./main
clean:
	rm -f my_print.o main.o main 
