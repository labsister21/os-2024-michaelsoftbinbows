# MICHAELSOFT BINBOWS

## TUGAS BESAR SISTEM OPERASI - IF2230

Template dasar untuk IF2230 - Sistem Operasi 2024

> Chapter 0: Toolchain, Kernel, GDT

> Chapter 1: Interrupt, Driver, File System

> Chapter 2: Paging, User Mode, Shell

> Chapter 3: Process, Scheduler, Multitaskings

## Contributors

## Anggota Kelompok

<table>
    <tr>
        <td colspan="3", align = "center"><center>Nama Kelompok: MichaelsoftBinbows</center></td>
    </tr>
    <tr>
        <td>No.</td>
        <td>Nama</td>
        <td>NIM</td>
    </tr>
    <tr>
        <td>1.</td>
        <td>Ariel Herfrison</td>
        <td>13522002</td>
    </tr>
    <tr>
        <td>2.</td>
        <td>Kristo Anugrah</td>
        <td>13522024</td>
    </tr>
    <tr>
        <td>3.</td>
        <td>Venantius Sean Ardi Nugroho</td>
        <td>13522078</td>
    </tr>
        <tr>
        <td>4.</td>
        <td>M. Hanief Fatkhan Nashrullah</td>
        <td>13522100</td>
    </tr>
</table>

## Preface

![](Binbows.png)

MichaelSoft Binbows adalah sebuah OS berbasis linux yang dibuat sebagai tugas besar IF 2230, Sistem Operasi.
Selain sebagai pelengkap nilai pada mata kuliah tersebut, tugas ini juga dibuat untuk mengenang MichaelSoft Binbow yang ada di dunia nyata (pic related), yang sayangnya sudah bangkrut. Dengan dibuatnya sistem operasi ini, diharapkan namanya akan tetap dikenang sepanjang Linus Trovalds masih idup.

## Features

- Multi-level console commands
- Multi tasking
- Pembuatnya ganteng
- Bottom text

## File Structure

```bash
📦os-2024-michaelsoftbinbows
 ┣ 📂bin
 ┃ ┗ 📜.gitignore
 ┣ 📂other
 ┃ ┗ 📜grub1
 ┣ 📂src
 ┃ ┣ 📂cmos
 ┃ ┃ ┗ 📜cmos.c
 ┃ ┣ 📂external
 ┃ ┃ ┗ 📜external-inserter.c
 ┃ ┣ 📂filesystem
 ┃ ┃ ┣ 📜disk.c
 ┃ ┃ ┗ 📜fat32.c
 ┃ ┣ 📂Framebuffer
 ┃ ┃ ┣ 📜framebuffer.c
 ┃ ┃ ┗ 📜portio.c
 ┃ ┣ 📂header
 ┃ ┃ ┣ 📂cmos
 ┃ ┃ ┣ 📂cpu
 ┃ ┃ ┣ 📂driver
 ┃ ┃ ┣ 📂filesystem
 ┃ ┃ ┣ 📂paging
 ┃ ┃ ┣ 📂process
 ┃ ┃ ┣ 📂scheduler
 ┃ ┃ ┣ 📂stdlib
 ┃ ┃ ┣ 📂text
 ┃ ┃ ┗ 📜kernel-entrypoint.h
 ┃ ┣ 📂interrupt
 ┃ ┃ ┣ 📜idt.c
 ┃ ┃ ┗ 📜interrupt.c
 ┃ ┣ 📂Keyboard
 ┃ ┃ ┗ 📜keyboard.c
 ┃ ┣ 📂Paging
 ┃ ┃ ┗ 📜paging.c
 ┃ ┣ 📂process
 ┃ ┃ ┗ 📜process.c
 ┃ ┣ 📂scheduler
 ┃ ┃ ┗ 📜scheduler.c
 ┃ ┣ 📂stdlib
 ┃ ┃ ┗ 📜string.c
 ┃ ┣ 📜clock.c
 ┃ ┣ 📜clock.h
 ┃ ┣ 📜clockTet
 ┃ ┣ 📜crt0.s
 ┃ ┣ 📜gdt.c
 ┃ ┣ 📜kernel.c
 ┃ ┣ 📜kernel-entrypoint.s
 ┃ ┣ 📜linker.ld
 ┃ ┣ 📜kernel.c
 ┃ ┣ 📜menu.lst
 ┃ ┣ 📜testing.c
 ┃ ┣ 📜testing.h
 ┃ ┣ 📜user-linker.ld
 ┃ ┣ 📜user-shell.c
 ┃ ┗ 📜user-shell.h
 ┣ 📜README.md
 ┣ 📜.gitignore
 ┣ 📜Binbows.png
 ┗ 📜makefile

```

## Requirements

refer to chapter 0 of [this documentation](https://docs.google.com/document/d/1EafdqpKWpYpU08w8AmKrEDCedrh8PvnGJ3bJWZEeFPU/edit#heading=h.q5k5xxmbhitz)

## How to Run The Program

1. Clone repository ini.
2. Pastikan Anda berada pada sistem Linux (WSL bisa juga).
3. Change directory ke root repository ini.
4. Jalankan kode command berikut :

```
make disk
make insert-shell
make kernel
make run
```

5. Seharusnya jalan, selamat menikmati :D. (kalau ketemu bug jangan kasih tau lab sister ya)

## Included Commands

1. cd
2. ls
3. mkdir
4. cat
5. cp
6. rm
7. mv
8. find
9. exec

## Acknowledgements

1. Tuhan Yang Maha Esa
2. Kakak-Tachi di Lab Sister yang sudah membantu
3. Teman - teman yang Bersemangat Mengerjakan

## Epilogue

... ga ada - apa di sini, here have a best girl

![](byebye.gif)
