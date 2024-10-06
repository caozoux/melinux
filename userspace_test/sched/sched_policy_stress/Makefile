
LFLAGS := -lstdc++ -lpthread
TAGRET := sched_test

OUT_PATH := OUT
TAGRET := $(OUT_PATH)/$(TAGRET)

SRC_OBJS1 :=$(wildcard src/*.c)

SRC_OBJS :=$(notdir $(SRC_OBJS1))
OBJS :=$(patsubst %.c, $(OUT_PATH)/%.o,$(SRC_OBJS))

$(TAGRET) : $(OBJS)
	cc -o $(TAGRET) $(OUT_PATH)/*.o  -lc $(LFLAGS)
$(OBJS): $(OUT_PATH)/%.o: src/%.c
	cc -c $< -o  $@ 

clean:
	@rm $(OUT_PATH)/*.o $(TAGRET)
