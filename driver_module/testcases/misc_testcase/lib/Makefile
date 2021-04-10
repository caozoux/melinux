TARGET :=misctest_lib.a

OBJS := page.o crc32.o

$(TARGET): $(OBJS)
	 ar rcs $(TARGET) $(OBJS)

%.o:%.c
	gcc -c $< -o $@
