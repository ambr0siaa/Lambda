#ifndef PARSER_H_
#define PARSER_H_

#include "types.h"
#include "lexer.h"
#include "arena.h"

LAM_API String_View *sv_dy(Arena *a, String_View sv);
LAM_API LObject obj_from_atom(Arena *a, Atom atom);

LAM_API Funcall *funcall_new(Arena *a, String_View name);

LAM_API Statement parse_statement(Arena *a, Lexer *L);
LAM_API Funcall *parse_funcall(Arena *a, Lexer *L);
LAM_API Expr parse_expr(Arena *a, Lexer *L);
LAM_API Atom parse_atom(Token tk);

LAM_API Expr stateval(Arena *a, Statement *s);
LAM_API Expr statfuncall(Funcall *f);

#define arethOp_cast(a, expected) \
    do { \
        if ((a)->t != (expected)) { \
            (a)->t = (expected); \
            switch ((a)->t) { \
                case ATOM_FLT: { \
                    (a)->v.as_flt = (double)(a)->v.as_int;\
                    break; \
                } \
                case ATOM_INT: { \
                    (a)->v.as_int = (i64)(a)->v.as_flt;\
                    break; \
                } \
                default: { \
                    report("Cannot cast to type `%u` for arethemtic op", (a)->t);\
                    exit(1); \
                } \
            } \
        } \
    } while(0)

#define integerOp(op, args, count, dest) \
    (dest)->v.as_int = (args)[0].v.a.v.as_int; \
    for (size_t i = 1; i < (count); ++i) { \
        arethOp_cast(&(args)[i].v.a, ATOM_INT); \
        (dest)->v.as_int = (dest)->v.as_int op (args)[i].v.a.v.as_int; \
    }

#define floatOp(op, args, count, dest) \
    (dest)->v.as_flt = (args)[0].v.a.v.as_flt; \
    for (size_t i = 1; i < (count); ++i) { \
        arethOp_cast(&(args)[i].v.a, ATOM_FLT); \
        (dest)->v.as_flt = (dest)->v.as_flt op (args)[i].v.a.v.as_flt; \
    }

#define arethOp(op, args, count, dest) \
    do { \
        if ((dest)->t == ATOM_FLT) { \
            floatOp(op, args, count, dest) \
        } else if ((dest)->t == ATOM_INT) { \
            integerOp(op, args, count, dest) \
        } else { \
            report("Invalid type `%u` for arethmetic op", (dest)->t); \
            (dest)->t = ATOM_NIL; \
        } \
    } while(0)

#endif // PARSER_H_
