/*
 *  TCC - Tiny C Compiler
 * 
 *  Copyright (c) 2001-2004 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef ONE_SOURCE
# define ONE_SOURCE 1
#endif

#include "tcc.h"
#if ONE_SOURCE
# include "libtcc.c"
#endif
#include "tcctools.c"

static const char help[] =
    "Tiny C Compiler "TCC_VERSION" - Copyright (C) 2001-2006 Fabrice Bellard\n"
    "Usage: tcc [options...] [-o outfile] [-c] infile(s)...\n"
    "       tcc [options...] -run infile (or --) [arguments...]\n"
    "General options:\n"
    "  -c           compile only - generate an object file\n"
    "  -o outfile   set output filename\n"
    "  -run         run compiled source [with custom stdin: -rstdin FILE]\n"
    "  -fflag       set or reset (with 'no-' prefix) 'flag' (see tcc -hh)\n"
    "  -Wwarning    set or reset (with 'no-' prefix) 'warning' (see tcc -hh)\n"
    "  -w           disable all warnings\n"
    "  -v --version show version\n"
    "  -vv          show search paths or loaded files\n"
    "  -h -hh       show this, show more help\n"
    "  -bench       show compilation statistics\n"
    "  -            use stdin pipe as infile\n"
    "  @listfile    read arguments from listfile\n"
    "Preprocessor options:\n"
    "  -Idir        add include path 'dir'\n"
    "  -Dsym[=val]  define 'sym' with value 'val'\n"
    "  -Usym        undefine 'sym'\n"
    "  -E           preprocess only\n"
    "  -nostdinc    do not use standard system include paths\n"
    "Linker options:\n"
    "  -Ldir        add library path 'dir'\n"
    "  -llib        link with dynamic or static library 'lib'\n"
    "  -nostdlib    do not link with standard crt and libraries\n"
    "  -r           generate (relocatable) object file\n"
    "  -rdynamic    export all global symbols to dynamic linker\n"
    "  -shared      generate a shared library/dll\n"
    "  -soname      set name for shared library to be used at runtime\n"
    "  -Wl,-opt[=val]  set linker option (see tcc -hh)\n"
    "Debugger options:\n"
    "  -g           generate stab runtime debug info\n"
    "  -gdwarf[-x]  generate dwarf runtime debug info\n"
#ifdef TCC_TARGET_PE
    "  -g.pdb       create .pdb debug database\n"
#endif
#ifdef CONFIG_TCC_BCHECK
    "  -b           compile with built-in memory and bounds checker (implies -g)\n"
#endif
#ifdef CONFIG_TCC_BACKTRACE
    "  -bt[N]       link with backtrace (stack dump) support [show max N callers]\n"
#endif
    "Misc. options:\n"
    "  -std=version define __STDC_VERSION__ according to version (c11/gnu11)\n"
    "  -x[c|a|b|n]  specify type of the next infile (C,ASM,BIN,NONE)\n"
    "  -Bdir        set tcc's private include/library dir\n"
    "  -M[M]D       generate make dependency file [ignore system files]\n"
    "  -M[M]        as above but no other output\n"
    "  -MF file     specify dependency file name\n"
#if defined(TCC_TARGET_I386) || defined(TCC_TARGET_X86_64)
    "  -m32/64      defer to i386/x86_64 cross compiler\n"
#endif
    "Tools:\n"
    "  create library  : tcc -ar [crstvx] lib [files]\n"
#ifdef TCC_TARGET_PE
    "  create def file : tcc -impdef lib.dll [-v] [-o lib.def]\n"
#endif
    "Discussion & bug reports:\n"
    "  https://lists.nongnu.org/mailman/listinfo/tinycc-devel\n"
    ;

static const char help2[] =
    "Tiny C Compiler "TCC_VERSION" - More Options\n"
    "Special options:\n"
    "  -P -P1                        with -E: no/alternative #line output\n"
    "  -dD -dM                       with -E: output #define directives\n"
    "  -pthread                      same as -D_REENTRANT and -lpthread\n"
    "  -On                           same as -D__OPTIMIZE__ for n > 0\n"
    "  -Wp,-opt                      same as -opt\n"
    "  -include file                 include 'file' above each input file\n"
    "  -nostdlib                     do not link with standard crt/libs\n"
    "  -isystem dir                  add 'dir' to system include path\n"
    "  -static                       link to static libraries (not recommended)\n"
    "  -dumpversion                  print version\n"
    "  -print-search-dirs            print search paths\n"
    "  -dt                           with -run/-E: auto-define 'test_...' macros\n"
    "Ignored options:\n"
    "  -arch -C --param -pedantic -pipe -s -traditional\n"
    "-W[no-]... warnings:\n"
    "  all                           turn on some (*) warnings\n"
    "  error[=warning]               stop after warning (any or specified)\n"
    "  write-strings                 strings are const\n"
    "  unsupported                   warn about ignored options, pragmas, etc.\n"
    "  implicit-function-declaration warn for missing prototype (*)\n"
    "  discarded-qualifiers          warn when const is dropped (*)\n"
    "-f[no-]... flags:\n"
    "  unsigned-char                 default char is unsigned\n"
    "  signed-char                   default char is signed\n"
    "  common                        use common section instead of bss\n"
    "  leading-underscore            decorate extern symbols\n"
    "  ms-extensions                 allow anonymous struct in struct\n"
    "  dollars-in-identifiers        allow '$' in C symbols\n"
    "  reverse-funcargs              evaluate function arguments right to left\n"
    "  gnu89-inline                  'extern inline' is like 'static inline'\n"
    "  asynchronous-unwind-tables    create eh_frame section [on]\n"
    "  test-coverage                 create code coverage code\n"
    "-m... target specific options:\n"
    "  ms-bitfields                  use MSVC bitfield layout\n"
#ifdef TCC_TARGET_ARM
    "  float-abi                     hard/softfp on arm\n"
#endif
#ifdef TCC_TARGET_X86_64
    "  no-sse                        disable floats on x86_64\n"
#endif
    "-Wl,... linker options:\n"
    "  -nostdlib                     do not search standard library paths\n"
    "  -[no-]whole-archive           load lib(s) fully/only as needed\n"
    "  -export-all-symbols           same as -rdynamic\n"
    "  -export-dynamic               same as -rdynamic\n"
    "  -image-base= -Ttext=          set base address of executable\n"
    "  -section-alignment=           set section alignment in executable\n"
#ifdef TCC_TARGET_PE
    "  -file-alignment=              set PE file alignment\n"
    "  -stack=                       set PE stack reserve\n"
    "  -large-address-aware          set related PE option\n"
    "  -subsystem=[console/windows]  set PE subsystem\n"
    "  -oformat=[pe-* binary]        set executable output format\n"
    "Predefined macros:\n"
    "  tcc -E -dM - < nul\n"
#else
    "  -rpath=                       set dynamic library search path\n"
    "  -enable-new-dtags             set DT_RUNPATH instead of DT_RPATH\n"
    "  -soname=                      set DT_SONAME elf tag\n"
#if defined(TCC_TARGET_MACHO)
    "  -install_name=                set DT_SONAME elf tag (soname macOS alias)\n"
#else
    "  -Ipath, -dynamic-linker=path  set ELF interpreter to path\n"
#endif
    "  -Bsymbolic                    set DT_SYMBOLIC elf tag\n"
    "  -oformat=[elf32/64-* binary]  set executable output format\n"
    "  -init= -fini= -Map= -as-needed -O -z= (ignored)\n"
    "Predefined macros:\n"
    "  tcc -E -dM - < /dev/null\n"
#endif
    "See also the manual for more details.\n"
    ;

static const char version[] =
    "tcc version "TCC_VERSION
#ifdef TCC_GITHASH
    " "TCC_GITHASH
#endif
    " ("
#ifdef TCC_TARGET_I386
        "i386"
#elif defined TCC_TARGET_X86_64
        "x86_64"
#elif defined TCC_TARGET_C67
        "C67"
#elif defined TCC_TARGET_ARM
        "ARM"
# ifdef TCC_ARM_EABI
        " eabi"
#  ifdef TCC_ARM_HARDFLOAT
        "hf"
#  endif
# endif
#elif defined TCC_TARGET_ARM64
        "AArch64"
#elif defined TCC_TARGET_RISCV64
        "riscv64"
#endif
#ifdef TCC_TARGET_PE
        " Windows"
#elif defined(TCC_TARGET_MACHO)
        " Darwin"
#elif TARGETOS_FreeBSD || TARGETOS_FreeBSD_kernel
        " FreeBSD"
#elif TARGETOS_OpenBSD
        " OpenBSD"
#elif TARGETOS_NetBSD
        " NetBSD"
#else
        " Linux"
#endif
    ")\n"
    ;

static void print_dirs(const char *msg, char **paths, int nb_paths)
{
    int i;
    printf("%s:\n%s", msg, nb_paths ? "" : "  -\n");
    for(i = 0; i < nb_paths; i++)
        printf("  %s\n", paths[i]);
}

static void print_search_dirs(TCCState *s)
{
    printf("install: %s\n", s->tcc_lib_path);
    /* print_dirs("programs", NULL, 0); */
    print_dirs("include", s->sysinclude_paths, s->nb_sysinclude_paths);
    print_dirs("libraries", s->library_paths, s->nb_library_paths);
    printf("libtcc1:\n  %s/%s\n", s->library_paths[0], CONFIG_TCC_CROSSPREFIX TCC_LIBTCC1);
#ifdef TCC_TARGET_UNIX
    print_dirs("crt", s->crt_paths, s->nb_crt_paths);
    printf("elfinterp:\n  %s\n",  DEFAULT_ELFINTERP(s));
#endif
}

static void set_environment(TCCState *s)
{
    char * path;

    path = getenv("C_INCLUDE_PATH");
    if(path != NULL) {
        tcc_add_sysinclude_path(s, path);
    }
    path = getenv("CPATH");
    if(path != NULL) {
        tcc_add_include_path(s, path);
    }
    path = getenv("LIBRARY_PATH");
    if(path != NULL) {
        tcc_add_library_path(s, path);
    }
}

static char *default_outputfile(TCCState *s, const char *first_file)
{
    char buf[1024];
    char *ext;
    const char *name = "a";

    if (first_file && strcmp(first_file, "-"))
        name = tcc_basename(first_file);
    if (strlen(name) + 4 >= sizeof buf)
        name = "a";
    strcpy(buf, name);
    ext = tcc_fileextension(buf);
#ifdef TCC_TARGET_PE
    if (s->output_type == TCC_OUTPUT_DLL)
        strcpy(ext, ".dll");
    else
    if (s->output_type == TCC_OUTPUT_EXE)
        strcpy(ext, ".exe");
    else
#endif
    if ((s->just_deps || s->output_type == TCC_OUTPUT_OBJ) && !s->option_r && *ext)
        strcpy(ext, ".o");
    else
        strcpy(buf, "a.out");
    return tcc_strdup(buf);
}

static unsigned getclock_ms(void)
{
#ifdef _WIN32
    return GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000 + (tv.tv_usec+500)/1000;
#endif
}
//bf 转译器
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define STACK_MAX 10000

typedef struct {
    bool data[STACK_MAX];
    int top;
} BoolStack;

static void stack_init(BoolStack *s) {
    s->top = 0;
}

static bool stack_empty(const BoolStack *s) {
    return s->top == 0;
}

static void stack_push(BoolStack *s, bool val) {
    if (s->top >= STACK_MAX) {
        fprintf(stderr, "栈溢出\n");
        exit(-1); // 或返回错误码，但此处 fatal
    }
    s->data[s->top++] = val;
}

static void stack_pop(BoolStack *s) {
    if (stack_empty(s)) {
        fprintf(stderr, "栈下溢\n");
        exit(-1);
    }
    s->top--;
}

// 核心函数：将 .bf 文件转译为 .c 文件
int translate_bf_file(const char *bf_filename, const char *c_filename) {
    FILE *bf_in = fopen(bf_filename, "rb");
    if (!bf_in) {
        fprintf(stderr, "无法打开 Brainfuck 文件: %s\n", bf_filename);
        return -1;
    }

    FILE *c_out = fopen(c_filename, "w");
    if (!c_out) {
        perror("无法创建临时 C 文件");
        fclose(bf_in);
        return -1;
    }

    BoolStack st;
    stack_init(&st);

    int val_acc = 0, ptr_acc = 0; // current_ptr 不再需要，除非调试
    int ch;

    fprintf(c_out, "#include<stdio.h>\n");
    fprintf(c_out, "unsigned char a[30000] = {0};\n");
    fprintf(c_out, "int main(){\n");
    fprintf(c_out, "unsigned char *p = a;\n");

    while ((ch = fgetc(bf_in)) != EOF) {
        char c = (char)ch;

        switch (c) {
            case '+':
                if (ptr_acc != 0) {
                    fprintf(c_out, "p+=%d;\n", ptr_acc);
                    ptr_acc = 0;
                }
                val_acc++;
                break;
            case '-':
                if (ptr_acc != 0) {
                    fprintf(c_out, "p+=%d;\n", ptr_acc);
                    ptr_acc = 0;
                }
                val_acc--;
                break;
            case '>':
                if (val_acc != 0) {
                    fprintf(c_out, "*p+=%d;\n", val_acc);
                    val_acc = 0;
                }
                ptr_acc++;
                break;
            case '<':
                if (val_acc != 0) {
                    fprintf(c_out, "*p+=%d;\n", val_acc);
                    val_acc = 0;
                }
                ptr_acc--;
                break;
            case '.':
                if (val_acc != 0) { fprintf(c_out, "*p+=%d;\n", val_acc); val_acc = 0; }
                if (ptr_acc != 0) { fprintf(c_out, "p+=%d;\n", ptr_acc); ptr_acc = 0; }
                fprintf(c_out, "putchar(*p);\n");
                break;
            case ',':
                if (val_acc != 0) { fprintf(c_out, "*p+=%d;\n", val_acc); val_acc = 0; }
                if (ptr_acc != 0) { fprintf(c_out, "p+=%d;\n", ptr_acc); ptr_acc = 0; }
                fprintf(c_out, "*p=getchar();\n");
                break;
            case '[':
                stack_push(&st, true);
                if (val_acc != 0) { fprintf(c_out, "*p+=%d;\n", val_acc); val_acc = 0; }
                if (ptr_acc != 0) { fprintf(c_out, "p+=%d;\n", ptr_acc); ptr_acc = 0; }
                fprintf(c_out, "while(*p){\n");
                break;
            case ']':
                if (stack_empty(&st)) {
                    fprintf(stderr, "编译失败：无左端点的循环\n");
                    goto error;
                }
                stack_pop(&st);
                if (val_acc != 0) { fprintf(c_out, "*p+=%d;\n", val_acc); val_acc = 0; }
                if (ptr_acc != 0) { fprintf(c_out, "p+=%d;\n", ptr_acc); ptr_acc = 0; }
                fprintf(c_out, "}\n");
                break;
            default:
                // 忽略非 BF 字符（符合规范）
                break;
        }
    }

    if (val_acc != 0) fprintf(c_out, "*p+=%d;\n", val_acc);
    if (ptr_acc != 0) fprintf(c_out, "p+=%d;\n", ptr_acc);
    fprintf(c_out, "return 0;\n}\n");

    if (!stack_empty(&st)) {
        fprintf(stderr, "编译失败：未闭合的循环\n");
        goto error;
    }

    fclose(bf_in);
    fclose(c_out);
    return 0;

error:
    fclose(bf_in);
    fclose(c_out);
    remove(c_filename);
    return -1;
}
int main(int argc, char **argv)
{
    TCCState *s, *s1;
    int ret, opt, n = 0, t = 0, done;
    unsigned start_time = 0, end_time = 0;
    const char *first_file;
    int argc0 = argc;
    char **argv0 = argv;
    FILE *ppfp = stdout;

redo:
    argc = argc0, argv = argv0;
    s = s1 = tcc_new();
    opt = tcc_parse_args(s, &argc, &argv);
    if (n == 0) {
        ret = 0;
        if (opt == OPT_HELP) {
            fputs(help, stdout);
            if (s->verbose)
                goto help2;
        } else if (opt == OPT_HELP2) {
            help2: fputs(help2, stdout);
        } else if (opt == OPT_M32 || opt == OPT_M64) {
            ret = tcc_tool_cross(argv, opt);
        } else if (s->verbose)
            printf("%s", version);

        if (opt == OPT_AR)
            ret = tcc_tool_ar(argc, argv);
#ifdef TCC_TARGET_PE
        if (opt == OPT_IMPDEF)
            ret = tcc_tool_impdef(argc, argv);
#endif
        if (opt == OPT_PRINT_DIRS) {
            /* initialize search dirs */
            set_environment(s);
            tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
            print_search_dirs(s);
        }
        if (opt) {
            if (opt < 0) err:
                ret = 1;
            tcc_delete(s);
            return ret;
        }
        if (s->nb_files == 0) {
            tcc_error_noabort("no input files");
        } else if (s->output_type == TCC_OUTPUT_PREPROCESS) {
            if (s->outfile && 0!=strcmp("-",s->outfile)) {
                ppfp = fopen(s->outfile, "wb");
                if (!ppfp)
                    tcc_error_noabort("could not write '%s'", s->outfile);
            }
        } else if (s->output_type == TCC_OUTPUT_OBJ && !s->option_r) {
            if (s->nb_libraries)
                tcc_error_noabort("cannot specify libraries with -c");
            else if (s->nb_files > 1 && s->outfile)
                tcc_error_noabort("cannot specify output file with -c many files");
        }
        if (s->nb_errors)
            goto err;
        if (s->do_bench)
            start_time = getclock_ms();
    }

    set_environment(s);
    if (s->output_type == 0)
        s->output_type = TCC_OUTPUT_EXE;
    tcc_set_output_type(s, s->output_type);
    s->ppfp = ppfp;
for (int i = 0; i < s->nb_files; i++) {
    const char *fname = s->files[i]->name;
    size_t len = strlen(fname);
    if (len >= 3 && strcmp(fname + len - 3, ".bf") == 0) {
        char temp_c[1024];
        snprintf(temp_c, sizeof(temp_c), "%s.c", fname);

        if (translate_bf_file(fname, temp_c) != 0) {
            tcc_error_noabort("Brainfuck 转译失败: %s", fname);
            goto err;
        }
        size_t new_len = strlen(temp_c);
        size_t new_size = sizeof(struct filespec) - 1 + new_len + 1;
        struct filespec *new_spec = tcc_malloc(new_size);
        if (!new_spec) {
            remove(temp_c);
            tcc_error_noabort("内存不足");
            goto err;
        }
        new_spec->type = s->files[i]->type;
        memcpy(new_spec->name, temp_c, new_len + 1);
        tcc_free(s->files[i]);
        s->files[i] = new_spec;
    }
}
    if ((s->output_type == TCC_OUTPUT_MEMORY
      || s->output_type == TCC_OUTPUT_PREPROCESS)
        && (s->dflag & 16)) { /* -dt option */
        if (t)
            s->dflag |= 32;
        s->run_test = ++t;
        if (n)
            --n;
    }

    /* compile or add each files or library */
    first_file = NULL;
    do {
        struct filespec *f = s->files[n];
        s->filetype = f->type;
        if (f->type & AFF_TYPE_LIB) {
            ret = tcc_add_library(s, f->name);
        } else {
            if (1 == s->verbose)
                printf("-> %s\n", f->name);
            if (!first_file)
                first_file = f->name;
            ret = tcc_add_file(s, f->name);
        }
    } while (++n < s->nb_files
            && 0 == ret
            && (s->output_type != TCC_OUTPUT_OBJ || s->option_r));

    if (s->do_bench)
        end_time = getclock_ms();

    if (s->run_test) {
        t = 0;
    } else if (s->output_type == TCC_OUTPUT_PREPROCESS) {
        ;
    } else if (0 == ret) {
        if (s->output_type == TCC_OUTPUT_MEMORY) {
#ifdef TCC_IS_NATIVE
            ret = tcc_run(s, argc, argv);
#endif
        } else {
            if (!s->outfile)
                s->outfile = default_outputfile(s, first_file);
            if (!s->just_deps)
                ret = tcc_output_file(s, s->outfile);
            if (!ret && s->gen_deps)
                gen_makedeps(s, s->outfile, s->deps_outfile);
        }
    }

    done = 1;
    if (t)
        done = 0; /* run more tests with -dt -run */
    else if (ret) {
        if (s->nb_errors)
            ret = 1;
        /* else keep the original exit code from tcc_run() */
    } else if (n < s->nb_files)
        done = 0; /* compile more files with -c */
    else if (s->do_bench)
        tcc_print_stats(s, end_time - start_time);

    tcc_delete(s);
    if (!done)
        goto redo;
    if (ppfp && ppfp != stdout)
        fclose(ppfp);
    return ret;
}
