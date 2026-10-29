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
	$(CC) $(CFLAGS) $(OBJS) -o $@ -lm

# Compile each .c into .o
build/%.o: $(SRCDIR)/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

-include $(DEPS)

clean:
	rm -rf build

# Unit checks for the pure prototype-vs-instance rules (obj_instance.c)
build/obj_instance_test: PhoenixMud/tests/obj_instance_test.c build/obj_instance.o | build
	$(CC) $(CFLAGS) $^ -o $@

# The class weapon's state and an object's binding through rent lines (hero_state.c)
build/hero_state_test: PhoenixMud/tests/hero_state_test.c build/hero_state.o | build
	$(CC) $(CFLAGS) $^ -o $@

# A weapon's life at the smiths, for the class weapon's forms (repair_life.c)
build/repair_life_test: PhoenixMud/tests/repair_life_test.c build/repair_life.o | build
	$(CC) $(CFLAGS) $^ -o $@

# A height or weight brought inside a race's range (size_fit.c)
build/size_fit_test: PhoenixMud/tests/size_fit_test.c build/size_fit.o | build
	$(CC) $(CFLAGS) $^ -o $@

# What a cast is worth to a class weapon (cw_math.c)
build/cw_math_test: PhoenixMud/tests/cw_math_test.c build/cw_math.o | build
	$(CC) $(CFLAGS) $^ -o $@ -lm

# The class armor's arithmetic and tables (armor_math.c)
build/armor_math_test: PhoenixMud/tests/armor_math_test.c build/armor_math.o build/hero_state.o | build
	$(CC) $(CFLAGS) $^ -o $@ -lm

# A word naming a help entry exactly (help_match.c)
build/help_match_test: PhoenixMud/tests/help_match_test.c build/help_match.o | build
	$(CC) $(CFLAGS) $^ -o $@

test: build/obj_instance_test build/hero_state_test build/repair_life_test build/size_fit_test build/help_match_test build/cw_math_test build/armor_math_test
	./build/obj_instance_test
	./build/hero_state_test
	./build/repair_life_test
	./build/size_fit_test
	./build/help_match_test
	./build/cw_math_test
	./build/armor_math_test

.PHONY: test clean
