//
// Created by sammc on 2/6/26.
//

#include "lexer.h"

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>


static bool is_operator(const char c) {
    switch(c) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '^':
            return true;
        default:
            return false;
    }
}

// this should only be calles once we know it is an operator via function above
static operator_type get_operator(const char op) {
    switch(op) {
        case '+':
            return OP_ADD;
        case '-':
            return OP_SUB;
        case '*':
            return OP_MUL;
        case '/':
            return OP_DIV;
        case '^':
            return OP_POW;
        default:
            unreachable();
    }
}

// Returns the number of characters consumed from `expr + offset`.
// Writes the resulting token into `out`.
static size_t lex_number(const char* expr, const size_t offset, token* out) {
    const char* start = expr + offset;
    char* end = nullptr;

    const double value = strtod(start, &end);

    // strtod didn't consume anything — shouldn't happen if caller
    // checked isdigit() first, but be defensive.
    if(end == start) {
        *out = (token){
            .error  = ERR_INVALID_NUMBER,
            .start  = start,
            .length = 1,
        };
        return 1;
    }

    // Guard against things like "3.14.15" — if the character right after
    // what strtod consumed is still a digit or dot, the number is malformed.
    if(*end == '.' || isdigit((unsigned char)*end)) {
        const size_t bad_len = (size_t)(end - start) + 1;
        *out = (token){
            .error  = ERR_INVALID_NUMBER,
            .start  = start,
            .length = (uint32_t)bad_len,
        };
        return bad_len;
    }

    const size_t consumed = (size_t)(end - start);
    *out = (token){
        .error        = ERR_NONE,
        .type         = TOKEN_NUMBER,
        .start        = start,
        .length       = (uint32_t)consumed,
        .value.number = value,
    };
    return consumed;
}

// Returns the number of characters consumed from `expr + offset`.
// Writes the resulting token into `out`.
// `total_length` is the full length of the expression string.
static size_t lex_identifier(const char* expr, const size_t offset, const size_t total_length, token* out) {
    const char* start = expr + offset;
    size_t consumed = 0;

    // Consume alphanumeric characters and underscores
    while(offset + consumed < total_length
          && (isalnum((unsigned char)start[consumed]) || start[consumed] == '_')) {
        consumed++;
    }

    // Peek past any whitespace to see if a '(' follows
    size_t peek = offset + consumed;
    while(peek < total_length && expr[peek] == ' ') {
        peek++;
    }

    const bool is_function = peek < total_length && expr[peek] == '(';

    *out = (token){
        .error  = ERR_NONE,
        .type   = is_function ? TOKEN_FUNCTION : TOKEN_VARIABLE,
        .start  = start,
        .length = (uint32_t)consumed,
    };
    return consumed;
}

token* lex_expression(const char* expression, const size_t length) {
    // +1 for a sentinel TOKEN_EOF at the end
    token* tokens = malloc(sizeof(token) * (length + 1));
    if(tokens == nullptr) {
        return nullptr;
    }

    size_t token_index = 0;
    for(size_t i = 0; i < length; i++) {
        const char current = expression[i];
        if(current == '(') {
            tokens[token_index] = (token){
                .error  = ERR_NONE,
                .type   = TOKEN_LPAREN,
                .start  = expression + i,
                .length = 1,
            };
            token_index++;
        }
        else if(current == ')') {
            tokens[token_index] = (token){
                .error  = ERR_NONE,
                .type   = TOKEN_RPAREN,
                .start  = expression + i,
                .length = 1,
            };
            token_index++;
        }
        else if(is_operator(current)) {
            tokens[token_index] = (token){
                .error    = ERR_NONE,
                .type     = TOKEN_OPERATOR,
                .start    = expression + i,
                .length   = 1,
                .value.op = get_operator(current),
            };
            token_index++;
        }
        else if(isdigit((unsigned char)current) || current == '.') {
            const size_t consumed = lex_number(expression, i, &tokens[token_index]);
            if(tokens[token_index].error != ERR_NONE) {
                // Error token is already written; return immediately
                // so the caller can inspect it.
                tokens[token_index + 1] = (token){.type = TOKEN_EOF};
                return tokens;
            }
            token_index++;
            i += consumed - 1; // -1 because the for-loop does i++
        }
        else if(isalpha((unsigned char)current) || current == '_') {
            const size_t consumed = lex_identifier(expression, i, length, &tokens[token_index]);
            token_index++;
            i += consumed - 1; // -1 because the for-loop does i++
        }
        else if(current == ' ') {}
        else {
            // Unrecognized character
            tokens[token_index] = (token){
                .error  = ERR_UNRECOGNIZED_CHAR,
                .start  = expression + i,
                .length = 1,
            };
            tokens[token_index + 1] = (token){.type = TOKEN_EOF};
            return tokens;
        }
    }

    // Sentinel: marks the end of the token array
    tokens[token_index] = (token){.type = TOKEN_EOF};
    return tokens;
}
