TARGET :=misctest_lib.a

OBJS := page.o


$(TARGET): $(OBJS)
	 ar rcs $(TARGET) $(OBJS)

%.o:%.c
	gcc -c $< -o $@
