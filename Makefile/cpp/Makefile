LFLAGS := -lstdc++ -lcurses -lprocps -lz
TARGET := test

OBJS := main.o

$(TARGET) : $(OBJS) main.o
	cc -o $(TARGET) main.o $(OBJS) -lc $(LFLAGS)

main.o: main.cpp
	cc -O0 -c main.cpp -o main.o

$(OBJS): %.o: %.cpp
	cc -O0 -c $< -o $@

clean:
	@rm *.o $(TARGET) -f


