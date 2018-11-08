.PHONY: all

#test : sudo ./namespace_execute_child -p -m  ./namespace_init

all: multiple_map   simple_map

multiple_map: multiple_map.o 

simple_map: simple_map.o

multiple_map: multiple_map.o

#namespace just can see it create child process
simple_map: simple_map.o

%.o:%.c
	@gcc -c $< -o $@

clean:
	@rm *.o
	@rm multiple_map   simple_map
