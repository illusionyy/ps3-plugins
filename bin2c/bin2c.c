// https://wiki.wxwidgets.org/Embedding_PNG_Images-Bin2c_In_C
// bin2c.c
//
// convert a binary file into a C source vector
//
// THE "BEER-WARE LICENSE" (Revision 3.1415):
// sandro AT sigala DOT it wrote this file. As long as you retain this notice you can do
// whatever you want with this stuff.  If we meet some day, and you think this stuff is
// worth it, you can buy me a beer in return.  Sandro Sigala
//
// syntax:  bin2c [-c] [-z] <input_file> <output_file>
//
//          -c    add the "const" keyword to definition
//          -z    terminate the array with a zero (useful for embedded C strings)
//
// examples:
//     bin2c -c myimage.png myimage_png.cpp
//     bin2c -z sometext.txt sometext_txt.cpp

#define _CRT_SECURE_NO_WARNINGS

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(PATH_MAX)
#define PATH_MAX 260
#endif

int useconst = 0;
int zeroterminated = 0;

static int myfgetc(FILE* f)
{
    int c = fgetc(f);
    if (c == EOF && zeroterminated)
    {
        zeroterminated = 0;
        return 0;
    }
    return c;
}

static void process(const char* ifname, const char* ofname, const char* char_name)
{
    FILE *ifile, *ofile;
    ifile = fopen(ifname, "rb");
    if (ifile == NULL)
    {
        fprintf(stderr, "cannot open %s for reading\n", ifname);
        exit(1);
    }
    ofile = fopen(ofname, "wb");
    if (ofile == NULL)
    {
        fprintf(stderr, "cannot open %s for writing\n", ofname);
        exit(1);
    }
#if 0
    char buf[PATH_MAX], *p;
    const char *cp;
    if ((cp = strrchr(ifname, '/')) != NULL)
    {
        ++cp;
    }
    else
    {
        if ((cp = strrchr(ifname, '\\')) != NULL)
            ++cp;
        else
            cp = ifname;
    }
    strcpy(buf, cp);
    for (p = buf; *p != '\0'; ++p)
    {
        if (!isalnum(*p))
            *p = '_';
    }
#endif
    fprintf(ofile, "// clang-format off\n"
        "static %sunsigned char %s[] = {\n", useconst ? "const " : "", char_name);
    int c, col = 1;
    while ((c = myfgetc(ifile)) != EOF)
    {
        if (col >= 78 - 6)
        {
            fputc('\n', ofile);
            col = 1;
        }
        fprintf(ofile, "0x%.2x, ", c);
        col += 6;
    }
    fprintf(ofile, "\n"
        "};\n"
        "// clang-format on\n");

    fclose(ifile);
    fclose(ofile);
}

static void usage(void)
{
    fprintf(stderr, "usage: bin2c [-cz] <input_file> <output_file> <symbol_name>\n");
    exit(1);
}

int main(int argc, char** argv)
{
    const int max_args = 4;
    while (argc > max_args)
    {
        if (!strcmp(argv[1], "-c"))
        {
            useconst = 1;
            --argc;
            ++argv;
        }
        else if (!strcmp(argv[1], "-z"))
        {
            zeroterminated = 1;
            --argc;
            ++argv;
        }
        else if (!strcmp(argv[1], "-cz"))
        {
            useconst = 1;
            zeroterminated = 1;
            --argc;
            ++argv;
        }
        else
        {
            usage();
        }
    }
    if (argc != max_args)
    {
        usage();
    }
    process(argv[1], argv[2], argv[3]);
    return 0;
}
