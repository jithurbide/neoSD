# Project Name
TARGET = NeoSD

# Sources
CPP_SOURCES = NeoSD.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP

# Includes FatFS source files within project.
USE_FATFS = 1

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

