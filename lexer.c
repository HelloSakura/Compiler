#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "compiler.h"
#include "helpers/buffer.h"
#include "helpers/vector.h"

#define LEX_GETC_IF(buffer, c, exp)     \
  for (c = peekc(); exp; c = peekc()) { \
    buffer_write(buffer, c);            \
    nextc();                            \
  }

struct token *read_next_token();
bool lex_is_in_expression();

static struct lex_process *lex_process;
static struct token tmp_token;

static char peekc() { return lex_process->functions->peek_char(lex_process); }

static void pushc(char c) { lex_process->functions->push_char(lex_process, c); }

/**
 * @brief 从文件中读下一个
 *
 * @return char
 */
static char nextc() {
  char c = lex_process->functions->next_char(lex_process);
  // 读一个，列+1
  if (lex_is_in_expression()) {
    buffer_write(lex_process->parentheses_buffer, c);
  }

  lex_process->pos.col += 1;
  if ('\n' == c) {
    lex_process->pos.col = 1;
    lex_process->pos.line += 1;
  }

  return c;
}

static char assert_next_char(char c) {
  char next_c = nextc();
  assert(c == next_c);
  return next_c;
}

static struct pos lex_file_position() { return lex_process->pos; }

struct token *token_create(struct token *_token) {
  memcpy(&tmp_token, _token, sizeof(struct token));
  // 记录当前符号在文件中的位置
  tmp_token.pos = lex_file_position();

  if (lex_is_in_expression()) {
    tmp_token.between_brackets = buffer_ptr(lex_process->parentheses_buffer);
  }
  return &tmp_token;
}

static struct token *lexer_last_token() {
  return vector_back_or_null(lex_process->token_vec);
}

static struct token *handle_whitespace() {
  struct token *last_token = lexer_last_token();
  if (last_token) {
    last_token->whitespace = true;
  }

  nextc();
  return read_next_token();
}

const char *read_number_str() {
  const char *num = NULL;
  struct buffer *buffer = buffer_create();
  char c = peekc();
  LEX_GETC_IF(buffer, c, (c >= '0' && c <= '9'));

  // 写入字符串结束符 \0
  buffer_write(buffer, 0x00);
  return buffer_ptr(buffer);
}

unsigned long long read_number() {
  const char *s = read_number_str();
  return atoll(s);
}

int lexer_number_type(char c) {
  int res = NUMBER_TYPE_NORMAL;
  if ('L' == c || 'l' == c) {
    res = NUMBER_TYPE_LONG;
  } else if ('f' == c || 'F' == c) {
    res = NUMBER_TYPE_FLOAT;
  } else if ('d' == c || 'D' == c) {
    res = NUMBER_TYPE_DOUBLE;
  }

  return res;
}

struct token *token_make_number_for_value(unsigned long number) {
  int number_type = lexer_number_type(peekc());
  if (NUMBER_TYPE_NORMAL != number_type) {
    // skip l, f, d...
    nextc();
  }
  return token_create(&(struct token){
      .type = TOKEN_TYPE_NUMBER, .llnum = number, .num.type = number_type});
}

struct token *token_make_number() {
  return token_make_number_for_value(read_number());
}

struct token *token_make_string(char start_delim, char end_delimi) {
  struct buffer *buf = buffer_create();
  // 处理左引号 "
  assert(nextc() == start_delim);
  char c = nextc();
  for (; c != end_delimi && c != EOF; c = nextc()) {
    // 处理转义字符
    if ('\\' == c) {
      continue;
    }
  }

  // 添加字符串结束标志0
  buffer_write(buf, 0x00);
  return token_create(
      &(struct token){.flag = TOKEN_TYPE_STRING, .sval = buffer_ptr(buf)});
}

/*----------func used for make string token-----------*/
/**
 * @brief 判断运算符是否有效（是否是C支持的）
 *
 * @param op
 * @return true
 * @return false
 */
bool op_valid(const char *op) {
  // 注意指针* 与乘法运算 *
  return S_EQ(op, "(") ||   // 点
         S_EQ(op, "[") ||   // 点
         S_EQ(op, "->") ||  // 指向结构体成员运算符
         S_EQ(op, ".") ||   // 结构体成员运算符
         S_EQ(op, "!") ||   // 单目运算符
         S_EQ(op, "~") || S_EQ(op, "++") || S_EQ(op, "--") || S_EQ(op, "-") ||
         S_EQ(op, "*") || S_EQ(op, "&") || S_EQ(op, "*") ||  // 双目运算符
         S_EQ(op, "/") || S_EQ(op, "%") || S_EQ(op, "+") || S_EQ(op, "-") ||
         S_EQ(op, "<<") ||                   // 移位运算
         S_EQ(op, ">>") || S_EQ(op, "<") ||  // 关系运算
         S_EQ(op, ">") || S_EQ(op, "<=") || S_EQ(op, ">=") || S_EQ(op, "==") ||
         S_EQ(op, "!=") || S_EQ(op, "+") ||  // 位运算
         S_EQ(op, "&") || S_EQ(op, "^") || S_EQ(op, "|") ||
         S_EQ(op, "&&") ||                   // 逻辑运算
         S_EQ(op, "||") || S_EQ(op, "?") ||  // 三目？
         S_EQ(op, "=") ||                    // 赋值运算符
         S_EQ(op, "+=") || S_EQ(op, "-=") || S_EQ(op, "*=") || S_EQ(op, "/=") ||
         S_EQ(op, "%=") || S_EQ(op, ">>=") || S_EQ(op, "<<=") ||
         S_EQ(op, "&=") || S_EQ(op, "^=") || S_EQ(op, "|=") ||
         S_EQ(op, ",") ||  // 逗号
         S_EQ(op, "...");  // 可变参数？？
}

static bool op_treated_as_one(char op) {
  return op == '(' || op == '[' || op == ',' || op == '.' || op == '*' ||
         op == '?';
}

static bool is_single_operator(char op) {
  return op == '+' || op == '-' || op == '*' || op == '/' || op == '=' ||
         op == '&' || op == '|' || op == '!' || op == '~' || op == '^' ||
         op == '>' || op == '<' || op == '%' || op == '(' || op == '[' ||
         op == ',' || op == '.' || op == '?';
}

/**
 * @brief 处理读入不正确操作符，例如+* -* *- /+
 * 将读入到buffer的内容，除第一个外，回退到文件流
 * @param buffer
 */
void read_op_flush_back_keep_first(struct buffer *buffer) {
  const char *data = buffer_ptr(buffer);
  int len = buffer->len;
  for (int i = len - 1; i >= 1; --i) {
    if (0x00 == data[i]) {
      continue;
    }
    pushc(data[i]);
  }
}

const char *read_op() {
  bool single_operator = true;
  char op = nextc();
  struct buffer *buffer = buffer_create();
  buffer_write(buffer, op);

  if (!op_treated_as_one(op)) {
    op = peekc();
    if (is_single_operator(op)) {
      buffer_write(buffer, op);
      nextc();
      single_operator = false;
    }
  }

  buffer_write(buffer, 0x00);
  char *ptr = buffer_ptr(buffer);

  // 至此已读入一个完整的operator，可能是单个字符如 +，可能是多个如 >>=
  // 需要判断读入的operator是否是有效的，或者说是否是C supported
  if (!single_operator) {
    if (!op_valid(
            ptr)) {  // 判断是否是一个有效的操作符，无效则只保留一个符号，其余写回
      read_op_flush_back_keep_first(buffer);
      // 保留第一个，写入字符串terminator
      ptr[1] = 0x00;
    }
  } else if (!op_valid(ptr)) {
    compiler_error(lex_process->compiler, "The operator %s is not valid\n",
                   ptr);
  }

  return ptr;
}

static void lex_new_expression() {
  lex_process->current_expression_count++;
  if (1 == lex_process->current_expression_count) {
    lex_process->parentheses_buffer = buffer_create();
  }
}

static void lex_finish_expression() {
  lex_process->current_expression_count--;
  if (lex_process->current_expression_count < 0) {
    compiler_error(lex_process->compiler,
                   "You closed an expression that you never opened.\n");
  }
}

bool lex_is_in_expression() {
  return lex_process->current_expression_count > 0;
}

bool is_keyword(const char *str) {
  return S_EQ(str, "void") ||  // 基本数据类型关键字  5
         S_EQ(str, "char") || S_EQ(str, "int") || S_EQ(str, "float") ||
         S_EQ(str, "double") || S_EQ(str, "short") ||  // 类型修饰关键字  4
         S_EQ(str, "long") || S_EQ(str, "signed") || S_EQ(str, "unsigned") ||
         S_EQ(str, "struct") ||  // 复杂类型关键字  5
         S_EQ(str, "union") || S_EQ(str, "enum") || S_EQ(str, "typedef") ||
         S_EQ(str, "sizeof") || S_EQ(str, "auto") ||  // 存储级别关键字  6
         S_EQ(str, "static") || S_EQ(str, "register") || S_EQ(str, "extern") ||
         S_EQ(str, "const") || S_EQ(str, "volatile") ||
         S_EQ(str, "return") ||  // 跳转结构  4
         S_EQ(str, "continue") || S_EQ(str, "break") || S_EQ(str, "goto") ||
         S_EQ(str, "if") ||  // 分支结构  5
         S_EQ(str, "else") || S_EQ(str, "switch") || S_EQ(str, "case") ||
         S_EQ(str, "default") || S_EQ(str, "for") ||  // 循环结构  3
         S_EQ(str, "do") || S_EQ(str, "while") ||
         S_EQ(str, "__ignore_typecheck") || S_EQ(str, "include") ||
         S_EQ(str, "restrict");
}

static struct token *token_make_operator_or_string() {
  char op = peekc();
  if ('<' == op) {  // 处理类似 #include<abc.h>
    struct token *last_token = lexer_last_token();
    if (token_is_keyword(last_token, "include")) {
      return token_make_string('<', '>');
    }
  }

  struct token *token = token_create(
      &(struct token){.type = TOKEN_TYPE_OPERATOR, .sval = read_op()});
  if ('(' == op) {  // 处理类似 (exp)
    lex_new_expression();
  }

  return token;
}

/*----------func used for make symbol token-----------*/
static struct token *token_make_symbol() {
  char c = nextc();
  if (')' == c) {
    lex_finish_expression();
  }

  struct token *token =
      token_create(&(struct token){.type = TOKEN_TYPE_SYMBOL, .cval = c});
  return token;
}

/*----------func used for make identifier token-----------*/
struct token *token_make_identifier_or_keyword() {
  struct buffer *buffer = buffer_create();
  char c = 0;
  LEX_GETC_IF(buffer, c,
              (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0') && (c <= '9') || (c == '_'));

  buffer_write(buffer, 0x00);

  // 检查是否是关键字
  if (is_keyword(buffer_ptr(buffer))) {
    return token_create(&(struct token){.type = TOKEN_TYPE_KEYWORD,
                                        .sval = buffer_ptr(buffer)});
  }
  return token_create(&(struct token){.type = TOKEN_TYPE_IDENTIFIER,
                                      .sval = buffer_ptr(buffer)});
}

struct token *read_special_token() {
  char c = peekc();
  if (isalpha(c) || '_' == c) {
    return token_make_identifier_or_keyword();
  }
  return NULL;
}

/*----------func used for make comment token-----------*/
struct token *token_make_one_line_comment() {
  struct buffer *buffer = buffer_create();
  char c = 0;
  LEX_GETC_IF(buffer, c, (c != '\n' && c != EOF));
  return token_create(
      &(struct token){.type = TOKEN_TYPE_COMMENT, .sval = buffer_ptr(buffer)});
}

struct token *token_make_multi_line_comment() {
  struct buffer *buffer = buffer_create();
  char c = 0;
  while (1) {
    LEX_GETC_IF(buffer, c, c != '*' && c != EOF);
    if (EOF == c) {
      compiler_error(lex_process->compiler,
                     "You did not close this multiline comment.\n");
    } else if ('*' == c) {
      // 继续读一个
      nextc();
      if ('/' == peekc()) {
        // skip '/'
        nextc();
        break;
      }
    }
  }
  return token_create(
      &(struct token){.type = TOKEN_TYPE_COMMENT, .sval = buffer_ptr(buffer)});
}

struct token *handle_comment() {
  char c = peekc();
  if ('/' == c) {
    nextc();
    if ('/' == peekc()) {
      nextc();
      return token_make_one_line_comment();
    } else if ('*' == peekc()) {
      nextc();
      return token_make_multi_line_comment();
    }

    // '/'可作为注释开头，单个出现则是除法运算符，故放在这里处理
    // 压回'/'，按除法运算符生成token
    pushc('/');
    return token_make_operator_or_string();
  }
  return NULL;
}

/*----------func used for make special number token-----------*/
/*处理：0xAB45*/

void lexer_pop_token() { vector_pop(lex_process->token_vec); }

bool is_hex_char(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

const char *read_hex_number_str() {
  struct buffer *buffer = buffer_create();
  char c = peekc();
  LEX_GETC_IF(buffer, c, is_hex_char(c));

  // 写入字符串结束符
  buffer_write(buffer, 0x00);
  return buffer_ptr(buffer);
}

struct token *token_make_special_number_hexadecimal() {
  // skip x
  nextc();

  unsigned long number = 0;
  const char *number_str = read_hex_number_str();
  number = strtol(number_str, 0, 16);

  return token_make_number_for_value(number);
}

/*处理：0b123*/
void lexer_validate_binary_string(const char *str) {
  size_t len = strlen(str);
  for (int i = 0; i < len; i++) {
    if ('0' != str[i] && '1' != str[i]) {
      compiler_error(lex_process->compiler,
                     "This is not a valid binary number.\n");
    }
  }
}

struct token *token_make_special_number_binary() {
  // skip b
  nextc();

  unsigned long number = 0;
  const char *number_str = read_number_str();
  lexer_validate_binary_string(number_str);
  number = strtol(number_str, 0, 2);

  return token_make_number_for_value(number);
}

struct token *token_make_special_number() {
  struct token *token = NULL;
  struct token *last_token = lexer_last_token();

  // pop 0xAB 中的0
  lexer_pop_token();

  char c = peekc();
  if ('x' == c) {
    token = token_make_special_number_hexadecimal();
  } else if ('b' == c) {
    token = token_make_special_number_binary();
  }

  return token;
}

/*----------func used for make quote token-----------*/
/*处理：char c = 'x'*/
char lex_get_escaped_char(char c) {
  char co = 0;
  switch (c) {
    case 'n':
      co = '\n';
      break;

    case '\\':
      co = '\\';
      break;

    case 't':
      co = '\t';
      break;

    case '\'':
      co = '\'';
      break;

    case 'a':
      co = '\a';
      break;

    case 'b':
      co = '\b';
      break;

    case 'f':
      co = '\f';
      break;

    case 'r':
      co = '\r';
      break;

    case 'v':
      co = '\v';
      break;
    default:
      break;
  }

  return co;
}

struct token *token_make_quote() {
  // 读入并判断左单引号
  assert_next_char('\'');
  // 读入''中的内容  ASCII：0-255
  char c = nextc();
  if ('\\' == c) {
    // 处理转义字符  '\\t'
    c = nextc();
    c = lex_get_escaped_char(c);
  }

  // 判断右单引号
  if ('\'' != nextc()) {
    compiler_error(
        lex_process->compiler,
        "You open a quote ' but did not close it with a ' character.\n");
  }

  return token_create(&(struct token){.type = TOKEN_TYPE_NUMBER, .cval = c});
}

/*----------func used for make newline token-----------*/
static struct token *token_make_newline() {
  nextc();
  return token_create(&(struct token){.type = TOKEN_TYPE_NEWLINE});
}

struct token *read_next_token() {
  struct token *token = NULL;
  char c = peekc();

  token = handle_comment();
  if (token) {
    return token;
  }

  switch (c) {
  NUMERIC_CASE:
    token = token_make_number();
    break;

  OPERATOR_CASE_EXCLUDING_DIVISION:
    token = token_make_operator_or_string();
    break;

  SYMBOL_CASE:
    token = token_make_symbol();
    break;

    case 'x':
    case 'b':
      token = token_make_special_number();
      break;

    case '"':
      token = token_make_string('"', '"');
      break;

    case '\'':
      token = token_make_quote();
      break;

    case ' ':
    case '\t':
      token = handle_whitespace();
      break;

    case '\n':
      token = token_make_newline();
      break;

    case EOF:
      /* 读到文件尾表示顺利完成分析 */
      break;

    default:
      token = read_special_token();
      if (!token) {
        // 读到不能识别的字符
        compiler_error(lex_process->compiler, "Unexpected token\n");
      }
  }

  return token;
}

/**
 * @brief 执行具体的词法分析任务
 *
 * @param process
 * @return int
 */
int lex(struct lex_process *process) {
  process->current_expression_count = 0;
  process->parentheses_buffer = NULL;
  lex_process = process;
  process->pos.filename = process->compiler->cfile.abs_path;

  struct token *token = read_next_token();

  // 处理文件，获得token
  while (token) {
    vector_push(process->token_vec, token);
    token = read_next_token();
  }
  gdb_print_lexer_token_vec(lex_process);
  return LEXICAL_ANALYSISI_ALL_OK;
}

/**
 * @brief 较char compile_process_peek_char(struct lex_process* lex_process)
 * 以下几个函数是从buffer中读取，算是sub_lexer，应该是用于处理lex_process.parentheses_buffer中的内容
 * lex_process.parentheses_buffer会在输入中包含（）时创建
 * 而compile_process_peek_char及相应的几个函数
 * 处理的是输入文件流，算是sub_compiler，前者buffer数据来源于文件流
 * @param process
 * @return char
 */
char lexer_string_buffer_next_char(struct lex_process *process) {
  struct buffer *buf = lex_process_private(process);
  return buffer_read(buf);
}

char lexer_string_buffer_peek_char(struct lex_process *process) {
  struct buffer *buf = lex_process_private(process);
  return buffer_peek(buf);
}

void lexer_string_buffer_push_char(struct lex_process *process, char c) {
  struct buffer *buf = lex_process_private(process);
  buffer_write(buf, c);
}

struct lex_process_functions lexer_string_buffer_functions = {
    .next_char = lexer_string_buffer_next_char,
    .peek_char = lexer_string_buffer_peek_char,
    .push_char = lexer_string_buffer_push_char};

/**
 * @brief 递归分析exp中括号内的内容
 *
 * @param compiler
 * @param str
 * @return struct lex_process*
 */
struct lex_process *tokens_build_for_string(struct compile_process *compiler,
                                            const char *str) {
  struct buffer *buffer = buffer_create();
  buffer_printf(buffer, str);
  struct lex_process *lex_process =
      lex_process_create(compiler, &lexer_string_buffer_functions, buffer);
  if (!lex_process) {
    return NULL;
  }

  if (lex(lex_process) != LEXICAL_ANALYSISI_ALL_OK) {
    return NULL;
  }

  return lex_process;
}