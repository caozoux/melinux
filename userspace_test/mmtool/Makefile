LFLAGS := -lstdc++ -lcurses -lprocps
TARGET := pageCompact

OBJS :=  PageMange.o  TaskScan.o
OBJH := mmpagecomman.h  PageMange.h  TaskScan.h

$(TARGET) : $(OBJS) main.o
	cc -o $(TARGET) main.o $(OBJS)  -lc $(LFLAGS)

main.o: $(OBJH)
	cc -O0 -c main.cpp -o  main.o

$(OBJS): %.o: %.cpp
	cc -O0 -c $< -o  $@


clean:
	@rm *.o $(TAGRET)

