.PHONY: all

CC=g++
#test : sudo ./namespace_execute_child -p -m  ./namespace_init

all: blocksig_test

blocksig_test: blocksig_test.o 

#simple_map: simple_map.o


#namespace just can see it create child process

%.o:%.c
	@g++ -c $< -o $@

clean:
	@rm *.o
	@rm multiple_map   simple_map
