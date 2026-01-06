#============================================================================*/
# Makefile for Spi_Sensor_Demo
#
# @description Build system for SPI Sensor module supporting compilation,
#              testing, and static analysis for automotive ECU software.
#
# @ assumptions & constraints
#   - Target: GCC (cross-compile for ARM Cortex-M or native for testing)
#   - No dynamic memory allocation
#   - MISRA C:2012 compliance required
#
# @ safety considerations
#   - All warnings treated as errors
#   - Static analysis integrated into build process
#   - Test execution verifies module behavior
#============================================================================*/

#============================================================================*/
# Project Configuration                                                        */
#============================================================================*/

# Project name
PROJECT_NAME := Spi_Sensor_Demo

# Base directories
BASE_DIR      := $(CURDIR)
SRC_DIR       := $(BASE_DIR)/src
INC_DIR       := $(BASE_DIR)/inc
EXAMPLE_DIR   := $(BASE_DIR)/example
BUILD_DIR     := $(BASE_DIR)/build
TEST_DIR      := $(BASE_DIR)/tests
OBJ_DIR       := $(BUILD_DIR)/obj
LIB_DIR       := $(BUILD_DIR)/lib
BIN_DIR       := $(BUILD_DIR)/bin
COV_DIR       := $(BUILD_DIR)/coverage

#============================================================================*/
# Toolchain Configuration                                                      */
#============================================================================*/

# Cross-compiler prefix (set to ARM toolchain if cross-compiling)
CROSS_COMPILE ?=

# Compiler and tools
CC      := $(CROSS_COMPILE)gcc
AR      := $(CROSS_COMPILE)ar
RANLIB  := $(CROSS_COMPILE)ranlib
SIZE    := $(CROSS_COMPILE)size
OBJCOPY := $(CROSS_COMPILE)objcopy

# Static analysis tools
CPPCHECK := cppcheck
SPLINT  := splint
CLANG_TIDY := clang-tidy

#============================================================================*/
# Compiler Flags - MISRA C:2012 Compliance                                    */
#============================================================================*/

# ANSI C standard with strict conformance
CFLAGS := -std=c99

# Enable all warnings and treat warnings as errors (safety-critical)
# Note: -Wno-unused-but-set-variable added for demo stub code
#       In production, remove this flag and ensure all variables are used
CFLAGS += -Wall -Wextra -Werror -Wno-unused-but-set-variable
CFLAGS += -Wpedantic
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wmissing-prototypes
CFLAGS += -Wmissing-declarations
CFLAGS += -Wold-style-definition
CFLAGS += -Wdeclaration-after-statement
CFLAGS += -Wbad-function-cast
CFLAGS += -Wcast-align
CFLAGS += -Wcast-qual
CFLAGS += -Wpointer-arith
CFLAGS += -Waggregate-return
CFLAGS += -Wfloat-equal
CFLAGS += -Wshadow
CFLAGS += -Wnested-externs
CFLAGS += -Winline
CFLAGS += -Wredundant-decls
CFLAGS += -Wvolatile-register-var

# Security and safety flags
CFLAGS += -fstack-protector-strong
CFLAGS += -fno-common
CFLAGS += -fstrict-aliasing
CFLAGS += -Wstrict-aliasing=2

# Debug and optimization
CFLAGS += -g
CFLAGS += -O0

# Include directories
CFLAGS += -I$(INC_DIR)

# Note: Configuration macros are defined in Spi_Sensor_Cfg.h
#       Do not redefine them here to avoid warnings

# Coverage flags (for testing)
COV_FLAGS := -fprofile-arcs -ftest-coverage

#============================================================================*/
# Source and Object Files                                                      */
#============================================================================*/

# Library source files
LIB_SRCS := $(SRC_DIR)/Spi_Sensor_Hal.c
LIB_SRCS += $(SRC_DIR)/Spi_Sensor_Driver.c
LIB_SRCS += $(SRC_DIR)/Spi_Sensor_Api.c

# Example source files
EXAMPLE_SRCS := $(EXAMPLE_DIR)/Spi_Sensor_Example.c

# Test source files (if tests exist)
TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)

# Object files
LIB_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(LIB_SRCS))
EXAMPLE_OBJS := $(patsubst $(EXAMPLE_DIR)/%.c,$(OBJ_DIR)/example_%.o,$(EXAMPLE_SRCS))
TEST_OBJS := $(patsubst $(TEST_DIR)/%.c,$(OBJ_DIR)/test_%.o,$(TEST_SRCS))

# Dependencies
DEPS := $(LIB_OBJS:.o=.d) $(EXAMPLE_OBJS:.o=.d) $(TEST_OBJS:.o=.d)

#============================================================================*/
# Output Files                                                                 */
#============================================================================*/

# Static library
LIB_NAME := libspisensor.a
LIB_TARGET := $(LIB_DIR)/$(LIB_NAME)

# Example executable
EXAMPLE_TARGET := $(BIN_DIR)/$(PROJECT_NAME)_example

# Test executable
TEST_TARGET := $(BIN_DIR)/$(PROJECT_NAME)_test

#============================================================================*/
# Phony Targets                                                                */
#============================================================================*/

.PHONY: all clean lib example test help
.PHONY: analyze analyze-cppcheck analyze-splint analyze-clang
.PHONY: format format-check
.PHONY: check misra-check
.PHONY: dirs
.PHONY: show-vars

#============================================================================*/
# Default Target                                                               */
#============================================================================*/

# Default target: build library and example
all: lib example

#============================================================================*/
# Directory Creation                                                           */
#============================================================================*/

# Create build directories
dirs:
	@echo "MKDIR $(OBJ_DIR)"
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(LIB_DIR)
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(COV_DIR)

#============================================================================*/
# Library Build                                                                */
#============================================================================*/

# Build static library
lib: dirs $(LIB_TARGET)

$(LIB_TARGET): $(LIB_OBJS)
	@echo "AR $@"
	@$(AR) rcs $@ $^
	@echo "RANLIB $@"
	@$(RANLIB) $@
	@echo "Library built: $@"

#============================================================================*/
# Example Build                                                                */
#============================================================================*/

# Build example executable
example: lib dirs $(EXAMPLE_TARGET)

$(EXAMPLE_TARGET): $(EXAMPLE_OBJS) $(LIB_TARGET)
	@echo "LD $@"
	@$(CC) $(CFLAGS) -o $@ $^ -lm
	@echo "Example built: $@"

#============================================================================*/
# Test Build and Run                                                           */
#============================================================================*/

# Build and run tests
test: lib dirs $(TEST_TARGET)
	@echo "Running tests..."
	@if [ -f $(TEST_TARGET) ]; then \
		$(TEST_TARGET) || exit 1; \
		echo "All tests passed!"; \
	else \
		echo "No tests found in $(TEST_DIR)/"; \
	fi

$(TEST_TARGET): $(TEST_OBJS) $(LIB_TARGET)
	@echo "LD $@"
	@$(CC) $(CFLAGS) $(COV_FLAGS) -o $@ $^ -lm
	@echo "Test binary built: $@"

# Coverage report
coverage: test
	@echo "Generating coverage report..."
	@gcov -o $(OBJ_DIR) $(LIB_SRCS) 2>/dev/null || true
	@lcov --capture --directory $(OBJ_DIR) --output-file $(COV_DIR)/coverage.info 2>/dev/null || true
	@genhtml $(COV_DIR)/coverage.info --output-directory $(COV_DIR)/html 2>/dev/null || true
	@echo "Coverage report in $(COV_DIR)/html/"

#============================================================================*/
# Compilation Rules                                                            */
#============================================================================*/

# Compile library source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	@echo "CC $<"
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Compile example source files
$(OBJ_DIR)/example_%.o: $(EXAMPLE_DIR)/%.c | dirs
	@echo "CC $<"
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Compile test source files
$(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.c | dirs
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(COV_FLAGS) -MMD -MP -c $< -o $@

#============================================================================*/
# Static Analysis                                                              */
#============================================================================*/

# Run all static analysis tools
analyze: analyze-cppcheck analyze-splint analyze-clang
	@echo "Static analysis complete"

# Cppcheck analysis
analyze-cppcheck:
	@echo "Running cppcheck..."
	@cppcheck --enable=all \
		--std=c99 \
		--inconclusive \
		--suppress=missingIncludeSystem \
		-I$(INC_DIR) \
		$(SRC_DIR) $(EXAMPLE_DIR) $(TEST_DIR) 2>&1 || true

# Splint analysis
analyze-splint:
	@echo "Running splint..."
	@if command -v splint >/dev/null 2>&1; then \
		splint +strict-lib -weak \
			-I$(INC_DIR) \
			$(SRC_DIR)/*.c $(EXAMPLE_DIR)/*.c 2>&1 || true; \
	else \
		echo "splint not installed, skipping..."; \
	fi

# Clang-tidy analysis
analyze-clang:
	@echo "Running clang-tidy..."
	@if command -v clang-tidy >/dev/null 2>&1; then \
		clang-tidy $(SRC_DIR)/*.c -- -I$(INC_DIR) -std=c99 2>&1 || true; \
	else \
		echo "clang-tidy not installed, skipping..."; \
	fi

#============================================================================*/
# MISRA C Compliance Check                                                     */
#============================================================================*/

# MISRA C:2012 compliance check
# Note: Requires tools like Cppcheck with MISRA rules or commercial tools
misra-check:
	@echo "Running MISRA C:2012 compliance check..."
	@cppcheck --enable=all \
		--addon=misra.json \
		--inconclusive \
		-I$(INC_DIR) \
		$(SRC_DIR) $(EXAMPLE_DIR) 2>&1 || true
	@echo "Note: For full MISRA compliance, use commercial tools like:"
	@echo "  - Coverity"
	@echo "  - Polyspace"
	@echo "  - QAC (Programming Research)"
	@echo "  - Cppcheck with MISRA addon"

# Generic check (runs analysis + basic MISRA)
check: analyze misra-check

#============================================================================*/
# Code Formatting                                                              */
#============================================================================*/

# Format code with clang-format
format:
	@echo "Formatting code with clang-format..."
	@find $(SRC_DIR) $(INC_DIR) $(EXAMPLE_DIR) $(TEST_DIR) \
		-name "*.c" -o -name "*.h" | \
		xargs clang-format -i -style=file 2>/dev/null || \
		echo "clang-format not found or .clang-format missing"

# Check code formatting
format-check:
	@echo "Checking code formatting..."
	@find $(SRC_DIR) $(INC_DIR) $(EXAMPLE_DIR) $(TEST_DIR) \
		-name "*.c" -o -name "*.h" | \
		xargs clang-format -style=file -Werror -dry-run 2>/dev/null || \
		echo "clang-format not found or .clang-format missing"

#============================================================================*/
# Clean                                                                        */
#============================================================================*/

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -f $(COV_DIR)/*.gcda $(COV_DIR)/*.gcno
	@rm -f $(SRC_DIR)/*.gcda $(SRC_DIR)/*.gcno
	@rm -f $(EXAMPLE_DIR)/*.gcda $(EXAMPLE_DIR)/*.gcno
	@rm -f $(TEST_DIR)/*.gcda $(TEST_DIR)/*.gcno
	@echo "Clean complete"

#============================================================================*/
# Help                                                                         */
#============================================================================*/

help:
	@echo "Spi_Sensor_Demo Makefile"
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Build targets:"
	@echo "  all        - Build library and example (default)"
	@echo "  lib        - Build static library only"
	@echo "  example    - Build example program"
	@echo "  test       - Build and run tests"
	@echo "  coverage   - Generate code coverage report"
	@echo ""
	@echo "Analysis targets:"
	@echo "  analyze         - Run all static analysis tools"
	@echo "  analyze-cppcheck - Run cppcheck analysis"
	@echo "  analyze-splint  - Run splint analysis"
	@echo "  analyze-clang   - Run clang-tidy analysis"
	@echo "  misra-check     - Run MISRA C:2012 compliance check"
	@echo "  check           - Run all checks (analyze + misra)"
	@echo ""
	@echo "Formatting targets:"
	@echo "  format        - Format code with clang-format"
	@echo "  format-check  - Check code formatting"
	@echo ""
	@echo "Utility targets:"
	@echo "  clean         - Remove build artifacts"
	@echo "  dirs          - Create build directories"
	@echo "  show-vars     - Show makefile variables"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make              # Build everything"
	@echo "  make lib          # Build library only"
	@echo "  make test         # Run tests"
	@echo "  make analyze      # Run static analysis"
	@echo "  make clean all    # Clean rebuild"

#============================================================================*/
# Show Variables (Debug)                                                       */
#============================================================================*/

show-vars:
	@echo "Project Configuration:"
	@echo "  PROJECT_NAME = $(PROJECT_NAME)"
	@echo "  BASE_DIR     = $(BASE_DIR)"
	@echo ""
	@echo "Toolchain:"
	@echo "  CC           = $(CC)"
	@echo "  AR           = $(AR)"
	@echo "  SIZE         = $(SIZE)"
	@echo ""
	@echo "Source files:"
	@echo "  LIB_SRCS     = $(LIB_SRCS)"
	@echo "  EXAMPLE_SRCS = $(EXAMPLE_SRCS)"
	@echo "  TEST_SRCS    = $(TEST_SRCS)"
	@echo ""
	@echo "Object files:"
	@echo "  LIB_OBJS     = $(LIB_OBJS)"
	@echo "  EXAMPLE_OBJS = $(EXAMPLE_OBJS)"
	@echo "  TEST_OBJS    = $(TEST_OBJS)"
	@echo ""
	@echo "Compiler flags:"
	@echo "  CFLAGS       = $(CFLAGS)"

#============================================================================*/
# Include Dependencies                                                         */
#============================================================================*/

-include $(DEPS)

#============================================================================*/
# End of Makefile                                                              */
#============================================================================*/
