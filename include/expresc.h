#pragma once
#ifndef EXPRESC_LIBRARY_H
#define EXPRESC_LIBRARY_H

#include <stddef.h>

typedef struct ExprescExpr expresc_expr;

typedef enum {
    EXPRESC_OK = 0,
    EXPRESC_ERR_NULL_ARG,
    EXPRESC_ERR_EMPTY_INPUT,
    EXPRESC_ERR_SYNTAX,
    EXPRESC_ERR_UNKNOWN_FUNC,
    EXPRESC_ERR_UNKNOWN_VAR,
    EXPRESC_ERR_DIV_BY_ZERO,
    EXPRESC_ERR_DOMAIN, // e.g., sqrt(-1)
    EXPRESC_ERR_ALLOC,
} expresc_error;

typedef struct {
    expresc_error error;
    double value; // meaningful only when error == EXPRESC_OK
} expresc_result;

// Lex, parse and compile an expression string.
// On success, *out points to a newly allocated expression; caller must free
// it with expresc_free(). On failure, *out is set to NULL.
expresc_error expresc_compile(const char* input, expresc_expr** out);

// Release all resources associated with an expression. Safe to call with NULL.
void expresc_free(expresc_expr* expr);

// Evaluation

// Evaluate a previously compiled expression with no variables.
expresc_result expresc_eval(const expresc_expr* expr);

// Evaluate with a set of named variable bindings.
// `names` and `values` are parallel arrays of length `count`.
expresc_result expresc_eval_with_vars(const expresc_expr* expr,
                                      const char* const * names,
                                      const double* values,
                                      size_t count);


expresc_result expresc_evaluate(const char* input);

expresc_result expresc_evaluate_with_vars(const char* input,
                                          const char* const * names,
                                          const double* values,
                                          size_t count);


// Human-readable string for an error code (never returns NULL).
const char* expresc_error_string(expresc_error err);

// After a failed expresc_compile(), get the byte-offset where the
// error was detected. Returns (size_t)-1 if unknown or not applicable.
size_t expresc_error_offset(const expresc_expr* expr);

#endif // EXPRESC_LIBRARY_H
