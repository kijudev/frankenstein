CXX = clang++
BIN_DIR = bin
CORE_DIR = core
ECS_DIR = ecs
APPS_DIR = apps

APP_DIRS = $(filter-out $(APPS_DIR)/, $(wildcard $(APPS_DIR)/*))
APPS = $(notdir $(APP_DIRS))

.PHONY: all
all: $(addprefix $(BIN_DIR)/, $(APPS))

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/%: $(APPS_DIR)/%/main.cpp | $(BIN_DIR)
	@echo "Compiling $<..."
	$(CXX) -std=c++20 $< -o $@

.PHONY: clean
clean:
	@echo "Cleaning up..."
	rm -rf $(BIN_DIR)
