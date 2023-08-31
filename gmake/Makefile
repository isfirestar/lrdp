PROGRAM := hiklrdp

# specify the program version
VERSION := 1.0.0

PROJECT_DIR := $(shell pwd)/../

# TARGET shall be the output file when success compile
TARGET := $(PROGRAM)

# add include directory, path MUST end with slash
INC_DIRS := $(PROJECT_DIR)include/	$(PROJECT_DIR)include/redis/ $(PROJECT_DIR)nshost/ $(PROJECT_DIR)redis/ $(PROJECT_DIR)redis/hiredis/ $(PROJECT_DIR)src/ $(PROJECT_DIR)/src/redis/ $(PROJECT_DIR)/src/redis/hiredis/

# add include directory, this variable allow framework force traverse and include all head files in entire directoy and it's sub directory
# path MUST end with slash
INC_ENTIRE_DIRS :=

# add source directory, compiler shall compile all files which with $(SRC_SUFFIX) in these folders, path MUST end with slash
SRC_DIRS := $(PROJECT_DIR)src/ $(PROJECT_DIR)nshost/ $(PROJECT_DIR)redis/ $(PROJECT_DIR)redis/hiredis/

# add source directory, this variable allow framework force traverse and include all source files in entire directoy and it's sub directory
# path MUST end with slash
SRC_ENTIRE_DIRS :=

# add some source file which didn't in any of $(SRC_DIRS)
SRC_ADDON :=

# exclude some source file which maybe in one of $(SRC_DIRS)
SRC_EXCLUDE := $(PROJECT_DIR)redis/ae_epoll.c $(PROJECT_DIR)redis/ae_evport.c $(PROJECT_DIR)redis/ae_kqueue.c $(PROJECT_DIR)redis/ae_select.c

# specify the extension of source file name, can be one or more of (c/cc/cpp/cxx)
SRC_SUFFIX := c

# $(TARGET_TYPE) can be one of (dll/so, exe/app, lib/archive) corresponding to (dynamic-library, executive-elf, static-archive)
TARGET_TYPE := exe

# $(BUILD) can be one of (debug, release) to change optimization pattern of this build, default to (release)
BUILD := debug

# target architecture, can be one of  (X64/X8664/IA64/X86_64, X86/I386, ARM/ARM32, ARM64)
ARCH := X64

# specify the cross compiler prefix string (arm-seev100-linux-gnueabihf-)
ifneq ($(PLAT), )
ifeq ($(PLAT), $(filter $(PLAT), h5 H5))
ARCH = arm64
CROSS_COMPILER_PREFIX = aarch64-himix100-linux-
endif
ifeq ($(PLAT), $(filter $(PLAT), h9 H9))
ARCH = arm64
CROSS_COMPILER_PREFIX = aarch64-himix210-linux-
endif
endif

# user define complie-time options
CFLAGS_ADDON :=

# application additional link-time library path
LIBRARY_PATH_ADDON :=

# application link-time library
LIBRARYS := pthread rt dl m

# user define link-time options
LDFALGS_ADDON := -rdynamic

# directory to save intermediate file and output target ( ./build/ by default), path MUST end with slash
BUILD_DIR :=

# invoke other makefile or subdirs, using this field in the top make entry
INVOKE :=
INVOKE_SETTINGS := CROSS_COMPILER_PREFIX=$(CROSS_COMPILER_PREFIX) ARCH=$(ARCH) BUILD=$(BUILD)

# both of below variable use to help you run shell script before/after gcc linker executive
PRE_LINK_ORDER =
POST_LINK_ORDER =

include pattern.mk

