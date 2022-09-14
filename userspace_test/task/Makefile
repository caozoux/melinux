
LFLAGS := -lstdc++ -lpthread -lgflags
TAGRET := sched_test

OUT_PATH := OUT
TAGRET := $(OUT_PATH)/$(TAGRET)

SRC_OBJS1 :=$(wildcard src/*.cpp)
CC := g++

SRC_OBJS :=$(notdir $(SRC_OBJS1))
OBJS :=$(patsubst %.cpp, $(OUT_PATH)/%.o,$(SRC_OBJS))

$(TAGRET) : $(OBJS)
	$(CC) -o $(TAGRET) $(OUT_PATH)/*.o  -lc $(LFLAGS)
$(OBJS): $(OUT_PATH)/%.o: src/%.cpp
	$(CC) -c $< -o  $@ 

clean:
	@rm $(OUT_PATH)/*.o $(TAGRET)
