LFLAGS := -lstdc++ -lcurses -lprocps
TAGRET := page_buddy

COBJS :=$(wildcard *.c)

SRC_OBJS :=$(notdir $(COBJS))
OBJS :=$(patsubst %.c, %.o, $(SRC_OBJS))

$(TAGRET) : $(OBJS)
	cc -o $(TAGRET) $(OBJS)  -lc $(LFLAGS)

$(OBJS): %.o: %.c
	cc -c $< -o  $@

clean:
	@rm *.o $(TAGRET)
