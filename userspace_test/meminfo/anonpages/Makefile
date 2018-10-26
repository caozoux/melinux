
LFLAGS := -lstdc++
TAGRET := dirty_cach_test

OUT_PATH :=OUT
TAGRET := $(OUT_PATH)/$(TAGRET)

SRC_OBJS1 :=$(wildcard src/*.cc)

SRC_OBJS :=$(notdir $(SRC_OBJS1))
OBJS :=$(patsubst %.cc, $(OUT_PATH)/%.o,$(SRC_OBJS))

$(TAGRET) : $(OBJS)
	cc -o $(TAGRET) $(OUT_PATH)/*.o  -lc $(LFLAGS)
$(OBJS): $(OUT_PATH)/%.o: src/%.cc
	cc -c $< -o  $@ 

clean:
	@rm $(OUT_PATH)/*.o $(TAGRET)
