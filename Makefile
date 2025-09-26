# Targets:
#  default - build project executable (optimized)
#  debug - build project executable (debug)
#  grumpy - build project executable (with all warnings on)
#  tests - TEST the project executable on tests in test director
#  clean - Remove excess files

# Project-specific settings
PROJECT := Project2

# Identify compiler to use
CXX := c++

# Flags to ALWAYs use
CFLAGS_all := -Wall -Wextra -std=c++20

# Flags based on compilation type.
#   Default flags turn on optimizations
#   Use "make debug" to turn on debugger flag
#   Use "make grumpy" to get extra warnings during compilation
CFLAGS := -O3 -DNDEBUG $(CFLAGS_all)
CFLAGS_debug := -g $(CFLAGS_all)
CFLAGS_grumpy := -pedantic -Wconversion -Weffc++ $(CFLAGS_all)

default: $(PROJECT)
all: $(PROJECT)

debug:	CFLAGS := $(CFLAGS_debug)
debug:	$(PROJECT)

grumpy:	CFLAGS := $(CFLAGS_grumpy)
grumpy:	$(PROJECT)

tests: $(PROJECT)
	@echo "Running project tests..."
	@cd tests && ./run_tests.sh
	@echo "Tests completed."

my_tests: $(PROJECT)
	@echo "Running my custom tests..."
	@cd my_tests && ./run_tests.sh
	@echo "Tests completed."

# Always run the tests, even if nothing has changed
.PHONY: tests

# List any files here that should trigger full recompilation when they change.
KEY_FILES := 

$(PROJECT):	$(PROJECT).cpp $(KEY_FILES)
	$(CXX) $(CFLAGS) $(PROJECT).cpp -o $(PROJECT)

clean:
	rm -f $(PROJECT) *.o tests/current/output-*.txt

# Debugging information
print-%: ; @echo '$(subst ','\'',$*=$($*))'
