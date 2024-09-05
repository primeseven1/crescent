#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <elf.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

static void perror_fmt(const char* fmt, ...) {
    int err = errno;

    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);

    fprintf(stderr, ": %s\n", strerror(err));
}

static Elf64_Ehdr* open_elf_file(const char* file_name, int* fd, size_t* size) {
    *fd = open(file_name, O_RDONLY);
    if (*fd == -1) {
        perror_fmt("Failed to open file %s", file_name);
        return NULL;
    }

    /* Get the size of the file and then just map the whole file to memory */
    struct stat sb;
    if (fstat(*fd, &sb) == -1) {
        perror_fmt("Failed to get the file size of %s", file_name);
        goto err;
    }
    if (sb.st_size == 0) {
        fprintf(stderr, "%s has a size of 0\n", file_name);
        goto err;
    }

    Elf64_Ehdr* ehdr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, *fd, 0);
    if (ehdr == MAP_FAILED) {
        perror_fmt("Failed to map file %s to memory", file_name);
        goto err;
    }


    *size = sb.st_size;
    return ehdr;

err:
    if (close(*fd)) {
        perror_fmt("Failed to close %s in the function that successfully opened it. How did this happen?", 
                file_name);
    }

    return NULL;
}

static void close_elf_file(Elf64_Ehdr* ehdr, int fd, size_t size) {
    int err = munmap(ehdr, size);
    if (err)
        perror_fmt("Unmapping the ELF failed.");
    err = close(fd);
    if (err)
        perror_fmt("Failed to close ELF file");
}

static int validate_elf_ehdr(const Elf64_Ehdr* ehdr) {
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 || 
            ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3)
        return -EINVAL;
    if (ehdr->e_version != EV_CURRENT)
        return -EINVAL;

    return 0;
}

static void print_gdb_command(const char* file_name, Elf64_Ehdr* ehdr) {
    uintptr_t kernel_min_offset = 0xffffffff80000000;

    const Elf64_Shdr* shdr = (Elf64_Shdr*)((char*)ehdr + ehdr->e_shoff);
    const char* shstrtab = (char*)ehdr + shdr[ehdr->e_shstrndx].sh_offset;

    printf("add-symbol-file %s", file_name);
    for (unsigned long i = 0; i < ehdr->e_shnum; i++) {
        if ((shdr[i].sh_flags & SHF_WRITE) || (shdr[i].sh_flags & SHF_ALLOC) || 
                (shdr[i].sh_flags & SHF_EXECINSTR)) {
            printf(" -s %s 0x%lx", shstrtab + shdr[i].sh_name, 
                    shdr[i].sh_addr + kernel_min_offset);
        }
    }
    printf("\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <file>\n", argv[0]);
        return 1;
    }

    int fd;
    size_t file_size;
    Elf64_Ehdr* ehdr = open_elf_file(argv[1], &fd, &file_size);
    if (!ehdr)
        return 1;
    if (validate_elf_ehdr(ehdr)) {
        close_elf_file(ehdr, fd, file_size);
        fprintf(stderr, "File %s is not a valid elf file\n", argv[1]);
        return 1;
    }

    print_gdb_command(argv[1], ehdr);
    return 0;
}
