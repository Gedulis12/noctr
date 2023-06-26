# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -Iinclude
LDFLAGS := -lcrypto -pthread

# Source files
SRCDIR := src
SOURCES := $(wildcard $(SRCDIR)/*.c)

# Object files
OBJDIR := obj
OBJECTS := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

# Output directory
BINDIR := bin

# Executable name
EXECUTABLE := $(BINDIR)/server

# Default target
all: $(EXECUTABLE)
	@rm -rf $(OBJDIR)  # Remove object files after linking

# Rule to compile object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to link the executable
$(EXECUTABLE): $(OBJECTS)
	@mkdir -p $(BINDIR)
	$(CC) $^ -o $@ $(LDFLAGS)

# Clean build artifacts
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Phony targets
.PHONY: all clean

