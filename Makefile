MODE ?= loader
SPI ?= nuspi
FLASH ?= w25q256fv
ARCH ?= rv32

$(info Build Loader in $(MODE) mode)

ifeq ($(MODE),sdk)
include Makefile.sdk
else
include Makefile.loader
endif
