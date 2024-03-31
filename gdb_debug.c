#include "compiler.h"
#include "helpers/vector.h"
#include <stdio.h>

void gdb_print_lexer_token(struct token* token)
{
    switch (token->type)
    {
    case TOKEN_TYPE_NUMBER:
        printf("<type: number, value: %lld>", token->llnum);
        break;
    case TOKEN_TYPE_NEWLINE:
        printf("<type: newline, value: \\n>");
        break;
    case TOKEN_TYPE_COMMENT:
        printf("comments");
        break;
    case TOKEN_TYPE_SYMBOL:
        printf("<type: symbol, value: %c>", token->cval);
        break;
    case TOKEN_TYPE_IDENTIFIER:
        printf("<type: identifier, value: %s>", token->sval);
        break;
    case TOKEN_TYPE_KEYWORD:
        printf("<type: keyword, value: %s>", token->sval);
        break;
    case TOKEN_TYPE_STRING:
        printf("<type: string, value: %s>", token->sval);
        break;
    case TOKEN_TYPE_OPERATOR:
        printf("<type: operator, value: %s>", token->sval);
        break;
    default:
        printf("Unknow type!\n");
        break;
    }
}


void gdb_print_lexer_token_vec(struct lex_process *lex_process)
{
    struct vector *token_vec = lex_process->token_vec;
    printf("token count:%d\n", vector_count(token_vec));
    for (int i = 0; i < vector_count(token_vec); ++i)
    {
        printf("token:%d ", i+1);
        gdb_print_lexer_token((struct token*)(vector_at(token_vec,i)));
        printf("\n");
    }
}