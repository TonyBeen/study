/*************************************************************************
    > File Name: test_gzip.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年04月03日 星期三 13时11分17秒
 ************************************************************************/

#include "zlib.h"
#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <string>

#include <utils/elapsed_time.h>

#ifdef USE_MMAP
#  include <sys/types.h>
#  include <sys/mman.h>
#  include <sys/stat.h>
#endif

#if defined(WIN32)
#include <fcntl.h>
#include <io.h>
    #define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
    #define SET_BINARY_MODE(file)
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
    #define snprintf _snprintf
#endif

#ifndef GZ_SUFFIX
    #define GZ_SUFFIX ".gz"
#endif

#define SUFFIX_LEN (sizeof(GZ_SUFFIX)-1)

#define BUFLEN          (16 * 1024)
#define MAX_NAME_LEN    1024

static char *prog;

void error(const char *msg) {
    fprintf(stderr, "%s: %s\n", prog, msg);
    exit(1);
}

#ifdef USE_MMAP

/* Try compressing the input file at once using mmap. Return Z_OK if
 * if success, Z_ERRNO otherwise.
 */
int gz_compress_mmap(FILE *in, gzFile out) {
    int len;
    int err;
    int ifd = fileno(in);
    caddr_t buf;    /* mmap'ed buffer for the entire input file */
    off_t buf_len;  /* length of the input file */
    struct stat sb;

    /* Determine the size of the file, needed for mmap: */
    if (fstat(ifd, &sb) < 0) return Z_ERRNO;
    buf_len = sb.st_size;
    if (buf_len <= 0) return Z_ERRNO;

    /* Now do the actual mmap: */
    buf = mmap((caddr_t) 0, buf_len, PROT_READ, MAP_SHARED, ifd, (off_t)0);
    if (buf == (caddr_t)(-1)) return Z_ERRNO;

    /* Compress the whole file at once: */
    len = gzwrite(out, (char *)buf, (unsigned)buf_len);

    if (len != (int)buf_len) error(gzerror(out, &err));

    munmap(buf, buf_len);
    fclose(in);
    if (gzclose(out) != Z_OK) error("failed gzclose");
    return Z_OK;
}
#endif /* USE_MMAP */

/* ===========================================================================
 * Compress input to output then close both files.
 */

void gz_compress(FILE *in, gzFile out) {
    static char buf[BUFLEN];
    int len;
    int err;

#ifdef USE_MMAP
    if (gz_compress_mmap(in, out) == Z_OK) return;
#endif
    for (;;) {
        len = (int)fread(buf, 1, sizeof(buf), in);
        if (ferror(in)) {
            perror("fread");
            exit(1);
        }

        if (len == 0) break;

        if (gzwrite(out, buf, (unsigned)len) != len)
        {
            error(gzerror(out, &err));
        }
    }

    if (gzclose(out) != Z_OK)
    {
        error("failed gzclose");
    }
}

void gz_uncompress(gzFile in, FILE *out) {
    static char buf[BUFLEN];
    int len;
    int err;

    for (;;) {
        len = gzread(in, buf, sizeof(buf));
        if (len < 0) error (gzerror(in, &err));
        if (len == 0) break;

        if ((int)fwrite(buf, 1, (unsigned)len, out) != len) {
            error("failed fwrite");
        }
    }
    if (fclose(out)) error("failed fclose");

    if (gzclose(in) != Z_OK) error("failed gzclose");
}

void file_compress(char *file, char *mode) {
    static char outfile[MAX_NAME_LEN];
    FILE  *in;
    gzFile out;

    if (strlen(file) + strlen(GZ_SUFFIX) >= sizeof(outfile)) {
        fprintf(stderr, "%s: filename too long\n", prog);
        exit(1);
    }

#if !defined(NO_snprintf) && !defined(NO_vsnprintf)
    snprintf(outfile, sizeof(outfile), "%s%s", file, GZ_SUFFIX);
#else
    strcpy(outfile, file);
    strcat(outfile, GZ_SUFFIX);
#endif

    in = fopen(file, "rb");
    if (in == NULL) {
        perror(file);
        exit(1);
    }
    out = gzopen(outfile, mode);
    if (out == NULL) {
        fprintf(stderr, "%s: can't gzopen %s\n", prog, outfile);
        exit(1);
    }
    gz_compress(in, out);

    unlink(file);
}

void file_uncompress(char *file) {
    static char buf[MAX_NAME_LEN];
    char *infile, *outfile;
    FILE  *out;
    gzFile in;
    z_size_t len = strlen(file);

    if (len + strlen(GZ_SUFFIX) >= sizeof(buf)) {
        fprintf(stderr, "%s: filename too long\n", prog);
        exit(1);
    }

#if !defined(NO_snprintf) && !defined(NO_vsnprintf)
    snprintf(buf, sizeof(buf), "%s", file);
#else
    strcpy(buf, file);
#endif

    if (len > SUFFIX_LEN && strcmp(file+len-SUFFIX_LEN, GZ_SUFFIX) == 0) {
        infile = file;
        outfile = buf;
        outfile[len-3] = '\0';
    } else {
        outfile = file;
        infile = buf;
#if !defined(NO_snprintf) && !defined(NO_vsnprintf)
        snprintf(buf + len, sizeof(buf) - len, "%s", GZ_SUFFIX);
#else
        strcat(infile, GZ_SUFFIX);
#endif
    }
    in = gzopen(infile, "rb");
    if (in == NULL) {
        fprintf(stderr, "%s: can't gzopen %s\n", prog, infile);
        exit(1);
    }
    out = fopen(outfile, "wb");
    if (out == NULL) {
        perror(file);
        exit(1);
    }

    gz_uncompress(in, out);

    unlink(infile);
}

// 获取文件带下
uint32_t getFileSize(FILE *file)
{
    uint32_t size = 0;

    // 将文件指针移动到文件末尾
    fseek(file, 0, SEEK_END);

    // 获取文件指针的当前位置，即文件大小
    size = ftell(file);

    // 将文件指针移动回文件开头
    fseek(file, 0, SEEK_SET);

    return size;
}


void test_compress(const char *fileName, const char *mode)
{
    FILE *pFileIn = nullptr;
    FILE *pFileOut = nullptr;
    gzFile gzOut;

    std::string gzFileName(fileName);
    gzFileName.append(GZ_SUFFIX);

    pFileIn = fopen(fileName, "rb");
    if (pFileIn == nullptr) {
        perror(fileName);
        exit(1);
    }

    uint32_t fileSize = getFileSize(pFileIn);

    pFileOut = fopen(gzFileName.c_str(), "wb+");
    if (pFileIn == nullptr) {
        perror(gzFileName.c_str());
        exit(1);
    }

    gzOut = gzdopen(fileno(pFileOut), mode);
    if (gzOut == nullptr) {
        fprintf(stderr, "%s: can't gzdopen stdout\n", prog);
        exit(1);
    }

    ElapsedTime et;
    et.start();
    gz_compress(pFileIn, gzOut);
    et.stop();
    printf("compress %s(%u) consume %zu ms\n", fileName, fileSize, et.elapsedTime());

    fclose(pFileIn);
    // gzclose(gzOut) in gz_compress
}

/* ===========================================================================
 * Usage: test_gzip [-f] [-h] [-r] [-1 to -9] [files...]
 *   -f : compress with Z_FILTERED
 *   -h : compress with Z_HUFFMAN_ONLY
 *   -r : compress with Z_RLE
 *   -1 to -9 : compression level
 */

int main(int argc, char *argv[]) {
    char *bname, outmode[20];

    snprintf(outmode, sizeof(outmode), "%s", "wb6 ");

    prog = argv[0];
    bname = strrchr(argv[0], '/');
    if (bname)
      bname++;
    else
      bname = argv[0];
    argc--, argv++;

    while (argc > 0) {
      if (strcmp(*argv, "-f") == 0)
        outmode[3] = 'f';
      else if (strcmp(*argv, "-h") == 0)
        outmode[3] = 'h';
      else if (strcmp(*argv, "-r") == 0)
        outmode[3] = 'R';
      else if ((*argv)[0] == '-' && (*argv)[1] >= '1' && (*argv)[1] <= '9' && (*argv)[2] == 0)
        outmode[2] = (*argv)[1];
      else
        break;
      argc--, argv++;
    }

    if (argc == 0)
    {
        printf("No input file\n");
        return 0;
    }

    if (outmode[3] == ' ')
        outmode[3] = '\0';

    do {
        test_compress(*argv, outmode);
    } while (argv++, --argc);

    return 0;
}
