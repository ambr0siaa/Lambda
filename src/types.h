#ifndef TYPES_H_
#define TYPES_H_

#define LAM_API extern
#define LAM_FUNC static inline

#include "sv.h"

typedef unsigned long long u64;
typedef signed long long i64;
typedef unsigned int u32;
typedef unsigned char u8;

typedef unsigned short Instruction;
typedef u64 Address;

#define defer_status(s) status = (s); goto defer

typedef enum {
    OBJ_TYPE_NIL = 0,
    OBJ_TYPE_INT,
    OBJ_TYPE_FLT,
    OBJ_TYPE_BOOLEAN,
    OBJ_TYPE_STR
} LObj_Type;

// Lambda Value
typedef union {
    String_View *s;
    double f;
    u64 u;
    i64 i;
    int b;
} LValue;

// Lambda Object
typedef struct {
    LValue v;
    u8 t;
} LObject;

#define OBJ_NIL (LObject) { .t = OBJ_TYPE_NIL }
#define OBJ_FLT(val) (LObject) { .t = OBJ_TYPE_FLT, .v = (LValue) { .f = (val) } }
#define OBJ_INT(val) (LObject) { .t = OBJ_TYPE_INT, .v = (LValue) { .i = (val) } }
#define OBJ_BOOL(val) (LObject) { .t = OBJ_TYPE_BOOLEAN, .v = (LValue) { .b = (val) } }
#define OBJ_STR(val) (LObject) { .t = OBJ_TYPE_STR, .v = (LValue) { .s = (val)}}

typedef enum {
    ATOM_NIL = 0,
    ATOM_INT,
    ATOM_FLT,
    ATOM_STR,
} Atom_Type;

typedef union {
    i64 as_int;
    double as_flt;
    String_View as_str;
} Atom_Value;

typedef struct {
    Atom_Type t;
    Atom_Value v;
} Atom;

typedef struct Funcall Funcall;

typedef enum {
    EXPR_NONE = 0,
    EXPR_ATOM,
    EXPR_FUNCALL,
} Expr_Type;

typedef union {
    Atom a;
    Funcall *f;
} Expr_Value;

typedef struct {
    Expr_Type t;
    Expr_Value v;
} Expr;

#define EXPR_EMPTY (Expr) {0}

typedef struct {
    size_t count;
    size_t capacity;
    Expr *items;
} Funargs;

struct Funcall {
    String_View name;
    Funargs args;
};

#define funarg_append(a, buf, item) arena_da_append(a,  buf, item, 16)

typedef enum {
    STATEMENT_NONE = 0,
    STATEMENT_VOID,
} Statement_Type;

typedef union {
    Expr e;         // void statement respresent a single expresion
} Statement_Value;

typedef struct {
    Statement_Type t;
    Statement_Value v;
} Statement;

#define STATE_NONE (Statement) {0}

LAM_API void report(const char *fmt, ...);

#endif // TYPES_H_
