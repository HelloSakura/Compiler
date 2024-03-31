#ifndef PEACHCOMPILER_H
#define PEACHCOMPILER_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// micro function
// 比较字串是否一致
#define S_EQ(str, str2) \
    (str && str2 && (0 == strcmp(str, str2)))

// 词素位置信息
struct pos
{
    int line;
    int col;
    const char *filename;
};

#define NUMERIC_CASE \
    case '0':        \
    case '1':        \
    case '2':        \
    case '3':        \
    case '4':        \
    case '5':        \
    case '6':        \
    case '7':        \
    case '8':        \
    case '9'

#define OPERATOR_CASE_EXCLUDING_DIVISION \
    case '+':                            \
    case '-':                            \
    case '*':                            \
    case '>':                            \
    case '<':                            \
    case '^':                            \
    case '%':                            \
    case '!':                            \
    case '=':                            \
    case '~':                            \
    case '|':                            \
    case '&':                            \
    case '(':                            \
    case '[':                            \
    case ',':                            \
    case '.':                            \
    case '?'

#define SYMBOL_CASE \
    case '{':       \
    case '}':       \
    case ':':       \
    case ';':       \
    case '#':       \
    case '\\':      \
    case ')':       \
    case ']'

// 词法分析结果状态
enum
{
    LEXICAL_ANALYSISI_ALL_OK,
    LEXICAL_ANALYSISI_INPUT_ERROR
};

// 词素类别
enum
{
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_SYMBOL,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_NEWLINE
};

enum
{
    NUMBER_TYPE_NORMAL,
    NUMBER_TYPE_LONG,
    NUMBER_TYPE_FLOAT,
    NUMBER_TYPE_DOUBLE
};

// 词法单元
struct token
{
    int type;
    int flag;
    struct pos pos;

    // 根据需要确定union共用体成员
    union
    {
        char cval;          // 字符
        const char *sval;   // 字符串
        unsigned int inum;  // 普通整型
        unsigned long lnum; //
        unsigned long long llnum;
        void *any;
    };

    struct token_number
    {
        int type;
    } num;
    // True：当两个token之间存在空白符
    bool whitespace;

    // 括号内字串
    // 便于调试
    const char *between_brackets;
};

struct lex_process;
// LEX_PROCESS_NEXT_CHAR:函数声明表达式  char (*pfun)(struct lex_process* process)
// ？模拟java继承
// 对应具体情况可以重写对应方法：implemente function and decide how it works
typedef char (*LEX_PROCESS_NEXT_CHAR)(struct lex_process *process);
typedef char (*LEX_PROCESS_PEEK_CHAR)(struct lex_process *process);
typedef void (*LEX_PROCESS_PUSH_CHAR)(struct lex_process *process, char c);

struct lex_process_functions
{
    LEX_PROCESS_NEXT_CHAR next_char;
    LEX_PROCESS_PEEK_CHAR peek_char;
    LEX_PROCESS_PUSH_CHAR push_char;
};

struct lex_process
{
    struct pos pos;
    struct vector *token_vec;
    struct compile_process *compiler;

    // 临时信息用于辅助词法分析
    /**
     * 记录括号brackets数目？深度
     *
     */
    int current_expression_count;
    struct buffer *parentheses_buffer;
    struct lex_process_functions *functions;

    //
    void *private;
};

enum
{
    COMPILER_FILE_COMPILED_OK,
    COMPILER_FAILED_WITH_ERRORS
};

struct compile_process
{
    // flags:文件编译选项，指定文件按照何种方式进行编译
    int flags;

    struct pos pos;
    struct compile_process_input_file
    {
        FILE *fp;
        const char *abs_path;
    } cfile;

    //a vector of tokens from lexical analysis.
    struct vector* token_vec;
    FILE *ofile;
};

/*---cprocess.c---*/
struct compile_process *compile_process_create(const char *filename, const char *filename_out, int flags);
// 读取文件流字符
char compile_process_next_char(struct lex_process *lex_process);
char compile_process_peek_char(struct lex_process *lex_process);
void compile_process_push_char(struct lex_process *lex_process, char c);

/*---compile.c---*/
int compile_file(const char *filename, const char *out_filename, int flags);

// 编译帮助
void compiler_error(struct compile_process *compiler, const char *msg, ...);
void compiler_warning(struct compile_process *compiler, const char *msg, ...);

/*---lex_process.c---*/
// 词法分析
struct lex_process *lex_process_create(struct compile_process *compiler, struct lex_process_functions *functions, void *private);
void lex_process_free(struct lex_process *process);
void *lex_process_private(struct lex_process *process);
struct vector *lex_process_vector(struct lex_process *process);

/*---lexer.c----*/
int lex(struct lex_process *process);
struct lex_process *tokens_build_for_string(struct compile_process *compiler, const char *str);

/*---token.c---*/
bool token_is_keyword(struct token *token, const char *value);

#endif