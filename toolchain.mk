PREFIX	=	$(HOME)/opt/cross
TARGET	=	i786-elf

CC		=	$(PREFIX)/bin/$(TARGET)-gcc
CXX		=	$(PREFIX)/bin/$(TARGET)-g++
LD		=	$(PREFIX)/bin/$(TARGET)-ld
AS		=	$(PREFIX)/bin/$(TARGET)-as
NASM	=	nasm

CFLAGS			=	-ffreestanding -nostdlib -fno-builtin -fno-stack-protector -Wall -g -I include
CXXFLAGS		=	-ffreestanding -nostdlib -fno-builtin -fno-stack-protector -Wall -std=c++17 -g -I include
LDFLAGS			=	-Tlink.lds
NASMFLAGS		=	-felf