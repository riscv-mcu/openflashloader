MODE ?= loader
SPI ?= nuspi
FLASH ?= w25q256fv
ARCH ?= rv32

$(info Build Flash Loader in $(MODE) mode: MODE=$(MODE) SPI=$(SPI) FLASH=$(FLASH) ARCH=$(ARCH))

ifeq ($(MODE),sdk)
include Makefile.sdk
else
include Makefile.loader
endif
