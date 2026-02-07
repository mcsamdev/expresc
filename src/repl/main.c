//
// Created by sammc on 2/6/26.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../parsing/lexer.h"

int main(const int argc, char* argv[]) {
    if(argc != 2) {
        printf("Usage: %s <expression>\n", argv[0]);
        return 1;
    }

    const token_array result = lex_expression(argv[1], strlen(argv[1]));
    if(result.tokens == nullptr) {
        fprintf(stderr, "Memory allocation failed during lexing\n");
        return 1;
    }

    char* debug_out = sprint_tokens(result.tokens, result.amount + 1);
    if(debug_out) {
        printf("\n%s", debug_out);
        free(debug_out);
    }

    free_tokens(result.tokens);
    return 0;
}
