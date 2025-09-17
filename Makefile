CXX = clang++
CXXFLAGS = -std=c++20 -g -Wall -Wextra

SRCDIR = apps/main
OBJDIR = obj
BINDIR = bin

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/main

.PHONY: all build-only clean compile-commands compile-commands-force
	
all: compile-commands $(TARGET)

build-only: $(TARGET)

$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CXX) $(OBJECTS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(OBJDIR) $(BINDIR) compile_commands.json

compile-commands: $(SOURCES)
	@echo "Generating compile_commands.json..."
	@bear -- $(MAKE) build-only
	@echo "compile_commands.json generated!"

compile-commands-force:
	@echo "Force regenerating compile_commands.json..."
	@bear -- $(MAKE) clean build-only
	@echo "compile_commands.json regenerated!"
