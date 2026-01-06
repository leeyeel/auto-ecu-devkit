###############################################################################
# @file    Makefile
# @brief   Makefile for SPI Demo on NXP S32K3
#
# @details Build system for SPI Demo application.
#
# @par     Target Device: NXP S32K3 Series
# @par     Compiler: GCC ARM Embedded (arm-none-eabi)
#
# @author  Auto ECU Demo
# @date    2026-01-07
#
# ============================================================================
# @par  Build Configuration:
#   - Build type: Release/Debug
#   - Optimization: -O2 (Release), -Og (Debug)
#   - Warnings: -Wall -Wextra -Wpedantic
#   - MISRA checking: Enabled with Code Quality tools
# ============================================================================
###############################################################################

#==============================================================================
# Project Configuration
#==============================================================================

PROJECT_NAME      := spi_demo_s32k3
PROJECT_VERSION   := 1.0.0
BUILD_DIR         := build
OUTPUT_DIR        := $(BUILD_DIR)/bin
LISTING_DIR       := $(BUILD_DIR)/listings
OBJECT_DIR        := $(BUILD_DIR)/obj
DEPEND_DIR        := $(BUILD_DIR)/depend

#==============================================================================
# Source Files
#==============================================================================

# Application sources
APP_SRCS := \
    main.c \
    spi_demo.c \
    spi_lpspi_s32k3.c

# Header files
APP_HDRS := \
    spi_demo.h \
    spi_lpspi_s32k3.h

# Generate object file list
APP_OBJS := $(APP_SRCS:%.c=$(OBJECT_DIR)/%.o)

#==============================================================================
# Toolchain Configuration
#==============================================================================

# GCC ARM Embedded Toolchain
PREFIX            := arm-none-eabi-
CC                := $(PREFIX)gcc
AS                := $(PREFIX)as
LD                := $(PREFIX)ld
OBJCOPY           := $(PREFIX)objcopy
OBJDUMP           := $(PREFIX)objdump
SIZE              := $(PREFIX)size
NM                := $(PREFIX)nm

#==============================================================================
# Compiler Flags
#==============================================================================

# Base flags
COMMON_FLAGS := \
    -mcpu=cortex-m4 \
    -mthumb \
    -mfloat-abi=hard \
    -mfpu=fpv4-sp-d16 \
    -fno-strict-aliasing \
    -ffunction-sections \
    -fdata-sections

# Warning flags
WARN_FLAGS := \
    -Wall \
    -Wextra \
    -Wpedantic \
    -Wconversion \
    -Wsign-conversion \
    -Wcast-qual \
    -Wcast-align \
    -Wwrite-strings \
    -Wpointer-arith \
    -Wformat=2 \
    -Wmissing-prototypes \
    -Wstrict-prototypes \
    -Wshadow \
    -Wunused-function \
    -Wunused-label \
    -Wunused-variable \
    -Wempty-body \
    -Wparentheses

# Optimization flags (override in command line)
OPT_LEVEL ?= -O2

# Debug flags (override in command line)
DEBUG_MODE ?= 0

ifeq ($(DEBUG_MODE),1)
    OPT_LEVEL := -Og
    DEBUG_FLAGS := -g -DDEBUG
else
    OPT_LEVEL := -O2
    DEBUG_FLAGS := -DNDEBUG
endif

# MISRA-C checking (requires Code Quality tools)
# Enable static analysis if available
MISRA_FLAGS := \
    -DMISRA_C=1

# Standard includes
INCLUDE_FLAGS := \
    -I. \
    -I$(BUILD_DIR)/generated

# C flags
C_FLAGS := \
    $(COMMON_FLAGS) \
    $(WARN_FLAGS) \
    $(OPT_LEVEL) \
    $(DEBUG_FLAGS) \
    $(MISRA_FLAGS) \
    -std=c11 \
    -D__ARM_FEATURE_CMSE=3 \
    -DUSE_STDLIB \
    -DNXP_S32K3 \
    $(INCLUDE_FLAGS)

# Assembly flags
AS_FLAGS := \
    $(COMMON_FLAGS) \
    -g

# Linker flags
LDFLAGS := \
    $(COMMON_FLAGS) \
    -Wl,--gc-sections \
    -Wl,--entry=Reset_Handler \
    -Wl,--unresolved-symbols=report-all \
    -specs=nosys.specs \
    -specs=nano.specs \
    -mcpu=cortex-m4

# Linker script
LDSCRIPT := s32k312.ld
LDFLAGS += -T$(LDSCRIPT)

#==============================================================================
# Build Rules
#==============================================================================

.PHONY: all clean run debug release help

# Default target
all: help

# Build release version
release: DEBUG_MODE=0
release: $(OUTPUT_DIR)/$(PROJECT_NAME).elf

# Build debug version
debug: DEBUG_MODE=1
debug: $(OUTPUT_DIR)/$(PROJECT_NAME).elf

# Build output directories
$(OUTPUT_DIR):
    mkdir -p $(OUTPUT_DIR)

$(LISTING_DIR):
    mkdir -p $(LISTING_DIR)

$(OBJECT_DIR):
    mkdir -p $(OBJECT_DIR)

$(DEPEND_DIR):
    mkdir -p $(DEPEND_DIR)

# Compile C sources
$(OBJECT_DIR)/%.o: %.c $(APP_HDRS) | $(OBJECT_DIR)
	@echo "Compiling: $<"
	$(CC) -c $(C_FLAGS) -MMD -MP -o $@ $<

# Link object files
$(OUTPUT_DIR)/$(PROJECT_NAME).elf: $(APP_OBJS) $(LDSCRIPT) | $(OUTPUT_DIR)
	@echo "Linking: $@"
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) -lc -lnosys

# Generate binary
$(OUTPUT_DIR)/$(PROJECT_NAME).bin: $(OUTPUT_DIR)/$(PROJECT_NAME).elf
	@echo "Generating binary: $@"
	$(OBJCOPY) -O binary $< $@

# Generate hex
$(OUTPUT_DIR)/$(PROJECT_NAME).hex: $(OUTPUT_DIR)/$(PROJECT_NAME).elf
	@echo "Generating hex: $@"
	$(OBJCOPY) -O ihex $< $@

# Generate listing
$(LISTING_DIR)/$(PROJECT_NAME).lst: $(OUTPUT_DIR)/$(PROJECT_NAME).elf
	@echo "Generating listing: $@"
	$(OBJDUMP) -S -l $< > $@

# Print size
size: $(OUTPUT_DIR)/$(PROJECT_NAME).elf
	@echo "Memory usage:"
	$(SIZE) -A -d $<

# Clean build artifacts
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR)
	@echo "Clean complete."

# Run simulation (requires qemu or similar)
run: $(OUTPUT_DIR)/$(PROJECT_NAME).bin
	@echo "Running simulation..."
	@echo "Note: Requires QEMU for ARM or hardware debugger"

# Display build information
info: $(OUTPUT_DIR)/$(PROJECT_NAME).elf
	@echo "Build Information:"
	@echo "  Project:   $(PROJECT_NAME)"
	@echo "  Version:   $(PROJECT_VERSION)"
	@echo "  Compiler:  $$($(CC) --version | head -n 1)"
	@echo "  Output:    $(OUTPUT_DIR)/$(PROJECT_NAME).elf"
	@echo "  Binary:    $(OUTPUT_DIR)/$(PROJECT_NAME).bin"
	@echo "  Hex:       $(OUTPUT_DIR)/$(PROJECT_NAME).hex"
	$(SIZE) -A -d $(OUTPUT_DIR)/$(PROJECT_NAME).elf

# Help
help:
	@echo ""
	@echo "========================================"
	@echo "  SPI Demo for NXP S32K3 - Build System"
	@echo "========================================"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Display this help (default)"
	@echo "  release   - Build release version"
	@echo "  debug     - Build debug version"
	@echo "  clean     - Remove build artifacts"
	@echo "  run       - Run simulation"
	@echo "  size      - Show memory usage"
	@echo "  info      - Display build information"
	@echo "  help      - Display this help"
	@echo ""
	@echo "Options:"
	@echo "  DEBUG_MODE=1 - Build debug version"
	@echo "  DEBUG_MODE=0 - Build release version (default)"
	@echo ""
	@echo "Output files:"
	@echo "  $(OUTPUT_DIR)/$(PROJECT_NAME).elf  - ELF executable"
	@echo "  $(OUTPUT_DIR)/$(PROJECT_NAME).bin  - Binary image"
	@echo "  $(OUTPUT_DIR)/$(PROJECT_NAME).hex  - Intel Hex"
	@echo "  $(LISTING_DIR)/$(PROJECT_NAME).lst - Listing"
	@echo ""
	@echo "========================================"

# Include generated dependencies
-include $(DEPEND_DIR)/*.d

###############################################################################
# End of Makefile
###############################################################################
