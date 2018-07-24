include toolchain.mk

.PHONY: clean run

SOURCES	= kmain.o boot.o ports.o screen.o utils.o descriptor_tables.o gdt.o idt.o
SOURCES	+= kmalloc.o interrupts.o debug.o paging.o icxxabi.o

%.c: %.h

%.cpp: %.h

%.o: %.asm
	$(NASM) $(NASMFLAGS) $<

kernel: $(SOURCES)
	$(LD) $(LDFLAGS) -o $@ $^

kernel.iso: kernel
	-rm -f image/boot/kernel
	cp kernel image/boot/kernel
	grub-mkrescue -o $@ image 

clean:
	-rm -f *.o kernel *.iso

run:
	make clean
	make kernel.iso
	/home/frabert/opt/bin/bochs -f bochs.txt -q