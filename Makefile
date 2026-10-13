# Makefile for the ls(1) midterm project.
#
#   make          build ./ls
#   make debug    build with debug symbols and sanitizers
#   make clean    remove object files and the binary

CC      ?= cc
CFLAGS  ?= -O2
CFLAGS  += -std=c99 -Wall -Wextra -pedantic
# Expose POSIX/XSI interfaces plus the BSD extensions (major(), S_ISWHT...).
CPPFLAGS += -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -D_DARWIN_C_SOURCE \
            -D_NETBSD_SOURCE

SRCDIR  = src
OBJDIR  = obj
TARGET  = ls

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all debug clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

# -MMD -MP generate header dependencies so edits to .h files rebuild.
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

debug: CFLAGS = -g -O0 -fsanitize=address,undefined
debug: clean $(TARGET)

clean:
	rm -rf $(OBJDIR) $(TARGET)

-include $(DEPS)
