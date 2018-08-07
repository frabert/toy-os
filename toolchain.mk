PREFIX	=	$(HOME)/opt/cross
TARGET	=	i786-elf

CC		=	$(PREFIX)/bin/$(TARGET)-gcc
CXX		=	$(PREFIX)/bin/$(TARGET)-g++
LD		=	$(PREFIX)/bin/$(TARGET)-ld
AS		=	$(PREFIX)/bin/$(TARGET)-as
AR		=	$(PREFIX)/bin/$(TARGET)-ar
NASM	=	nasm

CWARNINGS	=	-Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
						-Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
						-Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
						-Wstrict-prototypes

CXXWARNINGS	=	-Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
							-Wwrite-strings -Wmissing-declarations -Wredundant-decls -Winline \
							-Wno-long-long

CFLAGS			=	-ffreestanding $(CWARNINGS) -std=c11 -I include -g
CXXFLAGS		=	-ffreestanding $(CXXWARNINGS) -std=c++17 -I include -g -fno-rtti
LDFLAGS			=	-Tlink.lds
NASMFLAGS		=	-felf