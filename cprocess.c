#include<stdio.h>
#include<stdlib.h>
#include "compiler.h"

struct compile_process* compile_process_create(const char* filename, const char* filename_out, int flags)
{
    FILE *file = fopen(filename, "r");
    if(!file){
        return NULL;
    }

    FILE *out_file = NULL;
    if(filename_out){
        out_file = fopen(filename_out, "w");
        if(!out_file){
            return NULL;
        }
    }

    struct compile_process* process = calloc(1, sizeof(struct compile_process));
    process->flags = flags;
    process->cfile.fp = file;
    process->ofile = out_file;

    return process;
}

/**
 * @brief 从文件中读入一字符
 * as default function
 * @param lex_process 
 * @return char 
 */
char compile_process_next_char(struct lex_process* lex_process)
{
    struct compile_process* compiler = lex_process->compiler;
    //每次从fileStream读入1字符
    //列+1
    compiler->pos.col +=1;
    char c = getc(compiler->cfile.fp);

    //读到换行置位col并++line
    if('\n' == c){
        compiler->pos.col = 1;
        compiler->pos.line += 1;
    }

    return c;
}

/**
 * @brief 在文件流中查看下一个字符
 * as default function
 * @param lex_process 
 * @return char 
 */
char compile_process_peek_char(struct lex_process* lex_process)
{
    struct compile_process* compiler = lex_process->compiler;
    //从文件流中读一个后放回
    char c = getc(compiler->cfile.fp);
    ungetc(c, compiler->cfile.fp);

    return c;
}

/**
 * @brief 向文件流中退回一个字符
 * as default function
 * @param lex_process 
 * @param c 
 */
void  compile_process_push_char(struct lex_process* lex_process, char c)
{
    struct compile_process* complier = lex_process->compiler;
    ungetc(c, complier->cfile.fp);
}