
SOURCES  := source/read-stream.c source/exefs.c source/internal.c source/crypto.c
CFLAGS   ?= -ggdb3 -Wall -Wextra
TARGET   := libnnc.a
BUILD    := build
LIBS     := -lmbedcrypto

TEST_SOURCES  := test/main.c
TEST_TARGET   := nnc-test
LDFLAGS       ?=

# ====================================================================== #

TEST_OBJECTS := $(foreach source,$(TEST_SOURCES),$(BUILD)/$(source:.c=.o))
OBJECTS      := $(foreach source,$(SOURCES),$(BUILD)/$(source:.c=.o))
SO_TARGET    := $(TARGET:.a=.so)
DEPS         := $(OBJECTS:.o=.d)
CFLAGS       += -Iinclude $(LIBS) -std=gnu99
LDFLAGS      += $(LIBS)

.PHONY: all clean test shared run-test docs
all: $(TARGET)
run-test: test
	./$(TEST_TARGET)
docs:
	doxygen
test: $(TARGET) $(TEST_TARGET)
shared: CFLAGS += -fPIC
shared: clean $(SO_TARGET)
clean:
	rm -rf $(BUILD) $(TARGET) $(SO_TARGET)

-include $(DEPS)

$(TEST_TARGET): $(TEST_OBJECTS) $(TARGET)
	$(CC) $^ -o $@ $(LDFLAGS)

$(SO_TARGET): $(OBJECTS)
	$(CC) -shared $^ -o $@ $(LDFLAGS)

$(TARGET): $(OBJECTS)
	$(AR) -rcs $@ $^

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) -MMD -MF $(@:.o=.d)
