all : flash

#PREFIX_DEFAULT:=/home/tholin/riscv/bin/riscv32-unknown-elf
TARGET:=synth
ADDITIONAL_C_FILES+=YM3812.c spiflash.c
TARGET_MCU?=CH32V003
MINICHLINK=../ch32fun/minichlink

include ../ch32fun/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean


