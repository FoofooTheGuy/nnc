
SOURCES  := source/stream.c source/exefs.c source/internal.c source/crypto.c source/sigcert.c source/tmd.c source/u128.c source/utf.c source/smdh.c source/romfs.c source/ncch.c source/exheader.c source/cia.c source/ticket.c
CFLAGS   ?= -ggdb3 -Wall -Wextra -pedantic
TARGET   := libnnc.a
BUILD    ?= build
LIBS     ?= -lmbedcrypto

TEST_SOURCES  := test/main.c test/extract-exefs.c test/tmd.c test/u128.c test/smdh.c test/romfs.c test/ncch.c test/exheader.c test/cia.c test/tik.c
TEST_TARGET   := nnc-test
LDFLAGS       ?=

DESTDIR ?= /usr/local
LIBDIR  ?= lib64

# ====================================================================== #

TEST_OBJECTS := $(foreach source,$(TEST_SOURCES),$(BUILD)/$(source:.c=.o))
OBJECTS      := $(foreach source,$(SOURCES),$(BUILD)/$(source:.c=.o))
SO_TARGET    := $(TARGET:.a=.so)
DEPS         := $(OBJECTS:.o=.d)
CFLAGS       += -Iinclude -std=c99


.PHONY: all clean test shared run-test docs examples install uninstall
all: static
run-test: test
	./$(TEST_TARGET)
docs:
	doxygen
test: $(TARGET) $(TEST_TARGET)
shared:
	$(MAKE) CFLAGS="$(CFLAGS) -fPIC" BUILD="$(BUILD)/PIC" $(SO_TARGET)
static: $(TARGET)
examples: bin/ bin/gm9_filename bin/determine_legitimacy
clean:
	rm -rf $(BUILD) $(TARGET) $(SO_TARGET)
install: static shared
	mkdir -p $(DESTDIR)/$(LIBDIR)
	install $(TARGET) $(SO_TARGET) $(DESTDIR)/$(LIBDIR)
	mkdir -p $(DESTDIR)/include/nnc
	install include/nnc/* $(DESTDIR)/include/nnc
uninstall:
	rm -rf $(DESTDIR)/lib/libnnc.so $(DESTDIR)/lib/libnnc.a $(DESTDIR)/include/nnc

-include $(DEPS)

$(TEST_TARGET): $(TEST_OBJECTS) $(TARGET)
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

$(SO_TARGET): $(OBJECTS)
	$(CC) -shared $^ -o $@ $(LDFLAGS) $(LIBS)

$(TARGET): $(OBJECTS)
	$(AR) -rcs $@ $^

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) -MMD -MF $(@:.o=.d)

bin/:
	@mkdir -p bin

bin/gm9_filename: examples/gm9_filename.c $(TARGET)
	$(CC) $^ -o $@ $(LDFLAGS) $(CFLAGS) $(LIBS)
bin/determine_legitimacy: examples/determine_legitimacy.c $(TARGET)
	$(CC) $^ -o $@ $(LDFLAGS) $(CFLAGS) $(LIBS)
