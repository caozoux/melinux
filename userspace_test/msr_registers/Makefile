.PHONY: all

#test : sudo ./namespace_execute_child -p -m  ./namespace_init

all: namespace_name namespace_pid namespace_mult_pid namespace_execute_child

namespace_name: namespace_name.o 

namespace_pid: namespace_pid.o

namespace_execute_child: namespace_execute_child.o

#namespace just can see it create child process
namespace_mult_pid: namespace_mult_pid.o

%.o:%.c
	@gcc -c $< -o $@

clean:
	@rm namespace_name namespace_pid namespace_mult_pid namespace_execute_child *.o
