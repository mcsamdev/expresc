//
// Created by sammc on 2/6/26.
//
#pragma once
#ifndef EXPRESC_LEXER_H
#define EXPRESC_LEXER_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_POW,
} operator_type;

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_VARIABLE,
    TOKEN_NUMBER,
    TOKEN_OPERATOR,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_FUNCTION,
} token_type;

typedef enum {
    ERR_NONE = 0,
    ERR_UNRECOGNIZED_CHAR, // e.g., "5 @ 2" (@ is not an operator)
    ERR_INVALID_NUMBER, // e.g., "3.14.15" or "1.2e++3"
    ERR_NUMBER_OUT_OF_RANGE, // overflow/underflow from strtod
} tok_error_type;

typedef struct {
    tok_error_type error; // rest is only valid if this is ERR_NONE
    const char* start;
    uint32_t length;
    token_type type;

    union {
        double number;
        operator_type op;
    } value;
} token;

token* lex_expression(const char* expression, size_t length);
// this is only for debugging/testing. not intended to be used in actual use of lib.
char* sprint_tokens(const token* tokens, size_t amount);

void free_tokens(token* tokens);

#endif //EXPRESC_LEXER_H
