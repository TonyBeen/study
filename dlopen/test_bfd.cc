/*************************************************************************
    > File Name: test_bfd.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年05月11日 星期六 15时29分43秒
 ************************************************************************/

#define PACKAGE "bfd"
#define PACKAGE_VERSION "2.35"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bfd.h>

void print_symbol(asymbol *sym) {
    if (!sym) {
        printf("Symbol is NULL\n");
        return;
    }

    printf("Name: %s\n", bfd_asymbol_name(sym));
    printf("Value: 0x%lx\n", bfd_asymbol_value(sym));
    printf("Section: %s\n", bfd_section_name(bfd_asymbol_section(sym)));

    printf("\n");
}

int main(int argc, char **argv)
{
    bfd_init();
    bfd *abfd = bfd_openr("libmyclass.so", NULL);
    if (!abfd) {
        fprintf(stderr, "Failed to open binary file\n");
        exit(1);
    }

    if (!bfd_check_format(abfd, bfd_object)) {
        fprintf(stderr, "Unsupported file format\n");
        bfd_close(abfd);
        exit(1);
    }

    long storage;
    asymbol **syms;
    storage = bfd_get_symtab_upper_bound(abfd);
    if (storage < 0) {
        fprintf(stderr, "Failed to get symbol table size\n");
        bfd_close(abfd);
        exit(1);
    }

    if (storage) {
        syms = (asymbol **)malloc(storage);
        if (!syms) {
            fprintf(stderr, "Memory allocation failed\n");
            bfd_close(abfd);
            exit(1);
        }
        bfd_canonicalize_symtab(abfd, syms);

        for (int i = 0; syms[i]; i++) {
            print_symbol(syms[i]);
        }

        free(syms);
    }

    bfd_close(abfd);
    return 0;
}
