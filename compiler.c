#include "compiler.h"
#include<stdarg.h>
#include<stdlib.h>

struct lex_process_functions compiler_lex_functions = {
    .next_char = compile_process_next_char,
    .peek_char = compile_process_peek_char,
    .push_char = compile_process_push_char
};


/**
 * @brief 报错函数 
 * 
 * @param compiler 
 * @param msg 指示可变参数对应的类型，对照fprintf函数
 * @param ... 
 */
void compiler_error(struct compile_process* compiler, const char* msg, ...)
{
    //va_list处理可变参数
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);

    fprintf(stderr, " on line %i, col %i in file %s\n", compiler->pos.line, compiler->pos.col, compiler->pos.filename);
    exit(-1);
}

/**
 * @brief 
 * 
 * @param compiler 
 * @param msg 
 * @param ... 
 */
void compiler_warning(struct compile_process* compiler, const char* msg, ...)
{
    //va_list处理可变参数
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);

    fprintf(stderr, " on line %i, col %i in file %s\n", compiler->pos.line, compiler->pos.col, compiler->pos.filename);
}

int compile_file(const char* filename, const char* out_filename, int flags)
{
    struct compile_process* process = compile_process_create(filename, out_filename, flags);
    if(!process){
        return COMPILER_FAILED_WITH_ERRORS;
    }

    //preform lexical analysis  词法分析
    struct lex_process* lex_process = lex_process_create(process, &compiler_lex_functions, NULL);
    if(!lex_process){
        return COMPILER_FAILED_WITH_ERRORS;
    }

    //具体词法分析lex
    if(lex(lex_process) != LEXICAL_ANALYSISI_ALL_OK){
        return COMPILER_FAILED_WITH_ERRORS;
    }

    process->token_vec = lex_process->token_vec;
    
    //preform parsing   语法分析

    //preform code generation   代码生成

    return COMPILER_FILE_COMPILED_OK;
}