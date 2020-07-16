cflags += -I../

block_test: block_main.c
	@gcc -L/usr/lib64 -lm -lc -lpthread  $^ -o block_test

test: main.o rcu_test.o
	@gcc -L/usr/lib64 -lm -lc -lpthread  $^ -o test

%.o:%.c
	@gcc -c ${cflags} $^

clean:
	@rm *.o
	@rm test
