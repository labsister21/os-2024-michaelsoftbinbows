# Compiler & linker
ASM           = nasm
LIN           = ld
CC            = gcc

# Directory
SOURCE_FOLDER = src
OUTPUT_FOLDER = bin
ISO_NAME      = os2024

# Flags
WARNING_CFLAG = -Wall -Wextra -Werror
DEBUG_CFLAG   = -fshort-wchar -g
STRIP_CFLAG   = -nostdlib -fno-stack-protector -nostartfiles -nodefaultlibs -ffreestanding
CFLAGS        = $(DEBUG_CFLAG) $(WARNING_CFLAG) $(STRIP_CFLAG) -m32 -c -I$(SOURCE_FOLDER)
AFLAGS        = -f elf32 -g -F dwarf
LFLAGS        = -T $(SOURCE_FOLDER)/linker.ld -melf_i386

DISK_NAME      = storage

disk:
	@qemu-img create -f raw $(OUTPUT_FOLDER)/$(DISK_NAME).bin 4M


run: all
# @qemu-system-i386 -s -S -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso
	@qemu-system-i386 -s -drive file=$(OUTPUT_FOLDER)/$(DISK_NAME).bin,format=raw,if=ide,index=0,media=disk -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso
all: build
build: iso
clean:
	rm -rf *.o *.iso $(OUTPUT_FOLDER)/*.iso $(OUTPUT_FOLDER)/*.o



kernel:
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/kernel-entrypoint.s -o $(OUTPUT_FOLDER)/kernel-entrypoint.o
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/header/cpu/intsetup.s -o $(OUTPUT_FOLDER)/intsetup.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/kernel.c -o $(OUTPUT_FOLDER)/kernel.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/filesystem/disk.c -o $(OUTPUT_FOLDER)/disk.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/filesystem/fat32.c -o $(OUTPUT_FOLDER)/fat32.o

	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/gdt.c -o $(OUTPUT_FOLDER)/gdt.o

	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/Framebuffer/framebuffer.c -o $(OUTPUT_FOLDER)/framebuffer.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/Framebuffer/portio.c -o $(OUTPUT_FOLDER)/portio.o

	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/Interrupt/interrupt.c -o $(OUTPUT_FOLDER)/interrupt.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/Interrupt/idt.c -o $(OUTPUT_FOLDER)/idt.o

	@$(LIN) $(LFLAGS) bin/*.o -o $(OUTPUT_FOLDER)/kernel
	@echo Linking object files and generate elf32...
	@rm -f *.o

iso: kernel
	@mkdir -p $(OUTPUT_FOLDER)/iso/boot/grub
	@cp $(OUTPUT_FOLDER)/kernel     $(OUTPUT_FOLDER)/iso/boot/
	@cp other/grub1                 $(OUTPUT_FOLDER)/iso/boot/grub/
	@cp $(SOURCE_FOLDER)/menu.lst   $(OUTPUT_FOLDER)/iso/boot/grub/
	genisoimage -R             \
	-b boot/grub/grub1         \
	-no-emul-boot              \
	-boot-load-size 4          \
	-A os                      \
	-input-charset utf8        \
	-quiet                     \
	-boot-info-table           \
	-o $(OUTPUT_FOLDER)/os2024.iso              \
	$(OUTPUT_FOLDER)/iso
	@rm -r $(OUTPUT_FOLDER)/iso/

