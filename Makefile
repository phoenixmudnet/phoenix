GITHUB_SHA ?= manual build

CFLAGS := -MMD -MP -g -Wall -Wshadow -Wnested-externs -ffreestanding -m32 -L/lib/i386-linux-gnu -D GIT_REF="$(GITHUB_SHA)"

SRCDIR := PhoenixMud/src

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c, build/%.o, $(SRCS))
DEPS := $(OBJS:.o=.d)


circle/bin/circle: circle/compile/circle | circle/bin
	cp circle/compile/circle circle/bin/circle

circle/compile/circle: build/circle | circle/compile
	cp build/circle circle/compile/circle

circle/bin:
	mkdir -p circle/bin

circle/compile:
	mkdir -p circle/compile

build/circle: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

# Compile each .c into .o
build/%.o: $(SRCDIR)/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

-include $(DEPS)

clean:
	rm -rf build
