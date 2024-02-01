all : flash

TARGET:=synth
ADDITIONAL_C_FILES+=YM3812.c spiflash.c
CH32V003FUN=../ch32v003fun/ch32v003fun
MINICHLINK=../ch32v003fun/minichlink

include ../ch32v003fun/ch32v003fun/ch32v003fun.mk

flash : cv_flash
clean : cv_clean


