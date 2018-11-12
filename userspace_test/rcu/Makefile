.PHONY: all

#test : sudo ./namespace_execute_child -p -m  ./namespace_init

all: rcu_read_targ rcu_write_targ rcu_test

rcu_read_targ: rcu_read_targ.o 
rcu_write_targ: rcu_write_targ.o 
rcu_test:rcu_test.o
	gcc -o $@ $< -lpthread


%.o:%.c
	@gcc -c $< -o $@

clean:
	@rm *.o
	@rm multiple_map   simple_map
