.PHONY: all

#test : sudo ./namespace_execute_child -p -m  ./namespace_init

all: remote_signal

#namespace just can see it create child process
simple_map: remote_signal.o

%.o:%.c
	@gcc -c $< -o $@

clean:
	@rm *.o
	@rm multiple_map   simple_map
