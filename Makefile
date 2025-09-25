CXX      := clang++
CXXOPT   := -O1 -g
CXXFLAGS := -std=c++23 -Wall -Wextra -Iinclude 

BOLD     := \033[1m
BLUE     := \033[34m
GREEN    := \033[32m
RED      := \033[31m
RESET    := \033[0m

APPS      := $(wildcard apps/*/main.cpp)
APP_BINS  := $(patsubst apps/%/main.cpp,bin/%,$(APPS))

TESTS     := $(wildcard tests/*.cpp)
TEST_BINS := $(patsubst tests/%.cpp,bin/tests/%,$(TESTS))

.PHONY: all apps tests clean compile-commands run-tests

all: apps tests

apps: $(APP_BINS)

tests: $(TEST_BINS)

bin/%: apps/%/main.cpp
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(CXXOPT) $< -o $@

bin/tests/%: tests/%.cpp
	mkdir -p bin/tests
	$(CXX) $(CXXFLAGS) $(CXXOPT) $< -o $@

clean:
	rm -rf bin
	rm -f compile_commands.json

compile-commands: clean
	bear -- make

run-tests: tests
	@set -e; \
	for t in $(TEST_BINS); do \
	  printf "$(BOLD)$(BLUE)Running %s$(RESET)\n" "$$t"; \
	  if "$$t"; then \
	    printf "$(GREEN)PASS$(RESET)\n"; \
	  else \
	    printf "$(RED)FAIL: %s$(RESET)\n" "$$t"; \
	    exit 1; \
	  fi \
	done
