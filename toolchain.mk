PREFIX	=	$(HOME)/opt/cross
TARGET	=	i786-elf

CC		=	$(PREFIX)/bin/$(TARGET)-gcc
CXX		=	$(PREFIX)/bin/$(TARGET)-g++
LD		=	$(PREFIX)/bin/$(TARGET)-ld
AS		=	$(PREFIX)/bin/$(TARGET)-as
AR		=	$(PREFIX)/bin/$(TARGET)-ar
NASM	=	nasm

CFLAGS			=	-ffreestanding -Wall -std=c11 -I include -g
CXXFLAGS		=	-ffreestanding -Wall -std=c++17 -I include -g
LDFLAGS			=	-Tlink.lds
NASMFLAGS		=	-felf