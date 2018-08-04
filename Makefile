include toolchain.mk

.PHONY: clean run debug

SOURCES	=	kmain.o boot.o screen.o utils.o descriptor_tables.o gdt.o idt.o \
					interrupts.o debug.o paging.o icxxabi.o kheap.o assert.o timer.o \
					reflection.o synchro.o synchro_lowlevel.o tasking.o

%.o: %.asm
	$(NASM) $(NASMFLAGS) $<

kernel: $(SOURCES)
	$(LD) $(LDFLAGS) -o $@ $^

kernel.iso: kernel
	-rm -f image/boot/kernel
	cp kernel image/boot/kernel
	grub-mkrescue -o $@ image 

clean:
	-rm -f *.o kernel *.iso *.a

run: kernel.iso
	qemu-system-i386 -cdrom $^ -m 32

debug: kernel.iso
	qemu-system-i386 -cdrom $^ -m 32 -s -S -no-shutdown -no-reboot