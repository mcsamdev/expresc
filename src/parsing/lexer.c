//
// Created by sammc on 2/6/26.
//

#include "lexer.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

// This should only be called once we know it is an operator via function above
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
            assert(false && "get_operator called with non-operator character");
            unreachable();
    }
}

// --- Helper: create a single-character token ---
static token make_single_char_token(const token_type type, const char* start) {
    return (token){
        .error  = ERR_NONE,
        .type   = type,
        .start  = start,
        .length = 1,
    };
}

// --- Helper: create a single-character operator token ---
static token make_operator_token(const char* start, const char op_char) {
    return (token){
        .error    = ERR_NONE,
        .type     = TOKEN_OPERATOR,
        .start    = start,
        .length   = 1,
        .value.op = get_operator(op_char),
    };
}

// --- Helper: create an error token ---
static token make_error_token(tok_error_type error, const char* start, uint32_t length) {
    return (token){
        .error  = error,
        .start  = start,
        .length = length,
    };
}

// --- Helper: finalize with an error token at `count`, EOF at `count+1` ---
static token_array finalize_with_error(token* tokens, const size_t count) {
    // The error token is already written at tokens[count].
    tokens[count + 1] = (token){.type = TOKEN_EOF};
    return (token_array){.tokens = tokens, .amount = count + 1};
}

// --- Helper: finalize the token array, append EOF, shrink, and return ---
static token_array finalize_token_array(token* tokens, const size_t count) {
    tokens[count] = (token){.type = TOKEN_EOF};
    token* shrunk = realloc(tokens, sizeof(token) * (count + 1));
    if(shrunk != nullptr) {
        tokens = shrunk;
    }
    return (token_array){.tokens = tokens, .amount = count};
}

// Returns the number of characters consumed from `expr + offset`.
// Writes the resulting token into `out`.
static size_t lex_number(const char* expr, const size_t offset, token* out) {
    const char* start = expr + offset;
    char* end = nullptr;

    errno = 0;
    const double value = strtod(start, &end);

    // strtod didn't consume anything, or it only consumed a bare dot —
    // neither is a valid number literal.
    if(end == start || (end == start + 1 && *start == '.')) {
        const uint32_t len = (end > start) ? (uint32_t)(end - start) : 1;
        *out = (token){
            .error  = ERR_INVALID_NUMBER,
            .start  = start,
            .length = len,
        };
        return len;
    }

    // Reject overflow (±HUGE_VAL) and underflow (± true zero)
    if(errno == ERANGE) {
        const size_t consumed = (size_t)(end - start);
        *out = (token){
            .error  = ERR_NUMBER_OUT_OF_RANGE,
            .start  = start,
            .length = (uint32_t)consumed,
        };
        return consumed;
    }

    // Reject strtod-accepted literals we don't want: inf, nan, hex floats
    if(isinf(value) || isnan(value)
       || (end - start >= 2 && start[0] == '0' && (start[1] == 'x' || start[1] == 'X'))) {
        const size_t consumed = (size_t)(end - start);
        *out = (token){
            .error  = ERR_INVALID_NUMBER,
            .start  = start,
            .length = (uint32_t)consumed,
        };
        return consumed;
    }

    // Guard against things like "3.14.15" or "1e2e3" — because strtod is smart, if the character right
    // after what strtod consumed is still a digit, dot, or exponent marker, the number is malformed.
    if(*end == '.' || isdigit((unsigned char)*end)
       || *end == 'e' || *end == 'E') {
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

    // Defensive: caller guarantees at least one alnum/_ char, but be safe
    if(consumed == 0) {
        *out = make_error_token(ERR_UNRECOGNIZED_CHAR, start, 1);
        return 1;
    }

    // Peek past any whitespace to see if a '(' follows
    size_t peek = offset + consumed;
    while(peek < total_length && isspace((unsigned char)expr[peek])) {
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

token_array lex_expression(const char* expression, const size_t length) {
    // +1 for a sentinel TOKEN_EOF at the end
    token* tokens = malloc(sizeof(token) * (length + 1));
    if(tokens == nullptr) {
        return (token_array){.tokens = nullptr, .amount = 0};
    }

    size_t count = 0;
    for(size_t i = 0; i < length; i++) {
        const char current = expression[i];

        if(current == '(') {
            tokens[count++] = make_single_char_token(TOKEN_LPAREN, expression + i);
        }
        else if(current == ')') {
            tokens[count++] = make_single_char_token(TOKEN_RPAREN, expression + i);
        }
        else if(is_operator(current)) {
            tokens[count++] = make_operator_token(expression + i, current);
        }
        else if(isdigit((unsigned char)current) || current == '.') {
            const size_t consumed = lex_number(expression, i, &tokens[count]);
            if(tokens[count].error != ERR_NONE) {
                return finalize_with_error(tokens, count);
            }
            count++;
            i += consumed - 1;
        }
        else if(isalpha((unsigned char)current) || current == '_') {
            const size_t consumed = lex_identifier(expression, i, length, &tokens[count]);
            if(tokens[count].error != ERR_NONE) {
                return finalize_with_error(tokens, count);
            }
            count++;
            i += consumed - 1;
        }
        else if(isspace((unsigned char)current)) {}
        else {
            tokens[count] = make_error_token(ERR_UNRECOGNIZED_CHAR, expression + i, 1);
            return finalize_with_error(tokens, count);
        }
    }

    return finalize_token_array(tokens, count);
}

void free_tokens(token* tokens) {
    free(tokens);
}

// for sprint_tokens
static const char* token_type_str(const token_type type) {
    switch(type) {
        case TOKEN_EOF:
            return "EOF";
        case TOKEN_IDENTIFIER:
            return "IDENTIFIER";
        case TOKEN_VARIABLE:
            return "VARIABLE";
        case TOKEN_NUMBER:
            return "NUMBER";
        case TOKEN_OPERATOR:
            return "OPERATOR";
        case TOKEN_LPAREN:
            return "LPAREN";
        case TOKEN_RPAREN:
            return "RPAREN";
        case TOKEN_FUNCTION:
            return "FUNCTION";
        default:
            return "UNKNOWN";
    }
}

// for sprint_tokens
static const char* operator_type_str(const operator_type op) {
    switch(op) {
        case OP_ADD:
            return "+";
        case OP_SUB:
            return "-";
        case OP_MUL:
            return "*";
        case OP_DIV:
            return "/";
        case OP_POW:
            return "^";
        default:
            return "?";
    }
}

// for sprint_tokens
static const char* error_type_str(const tok_error_type err) {
    switch(err) {
        case ERR_NONE:
            return "NONE";
        case ERR_UNRECOGNIZED_CHAR:
            return "UNRECOGNIZED_CHAR";
        case ERR_INVALID_NUMBER:
            return "INVALID_NUMBER";
        case ERR_NUMBER_OUT_OF_RANGE:
            return "NUMBER_OUT_OF_RANGE";
        default:
            return "UNKNOWN_ERROR";
    }
}

// print for debugging
char* sprint_tokens(const token* tokens, const size_t max_amount) {
    if(tokens == nullptr) {
        return nullptr;
    }
    size_t capacity = 256;
    size_t used = 0;
    char* buf = malloc(capacity);
    if(buf == nullptr) {
        return nullptr;
    }
    buf[0] = '\0';

    for(size_t i = 0; i < max_amount; i++) {
        const token* t = &tokens[i];
        char line[256];
        int written;

        const bool is_terminal = t->type == TOKEN_EOF || t->error != ERR_NONE;

        if(t->error != ERR_NONE) {
            written = snprintf(line,
                               sizeof(line),
                               "[%zu] ERROR(%s)  \"%.*s\"\n",
                               i,
                               error_type_str(t->error),
                               (int)t->length,
                               t->start);
        }
        else if(t->type == TOKEN_NUMBER) {
            written = snprintf(line,
                               sizeof(line),
                               "[%zu] %-10s  %g\n",
                               i,
                               token_type_str(t->type),
                               t->value.number);
        }
        else if(t->type == TOKEN_OPERATOR) {
            written = snprintf(line,
                               sizeof(line),
                               "[%zu] %-10s  '%s'\n",
                               i,
                               token_type_str(t->type),
                               operator_type_str(t->value.op));
        }
        else if(t->type == TOKEN_EOF) {
            written = snprintf(line,
                               sizeof(line),
                               "[%zu] EOF\n",
                               i);
        }
        else {
            // VARIABLE, FUNCTION, IDENTIFIER, LPAREN, RPAREN
            written = snprintf(line,
                               sizeof(line),
                               "[%zu] %-10s  \"%.*s\"\n",
                               i,
                               token_type_str(t->type),
                               (int)t->length,
                               t->start);
        }

        if(written < 0) {
            if(is_terminal) {
                break;
            }
            continue;
        }

        const size_t line_len = (size_t)written;

        // Grow buffer if needed
        if(used + line_len + 1 > capacity) {
            capacity = (used + line_len + 1) * 2;
            char* grown = realloc(buf, capacity);
            if(grown == nullptr) {
                free(buf);
                return nullptr;
            }
            buf = grown;
        }

        memcpy(buf + used, line, line_len + 1);
        used += line_len;

        if(is_terminal) {
            break;
        }
    }

    return buf;
}
