#include <assert.h>
#include "parser.h"

static char *builtin_funcs = "+-*/";

String_View *sv_dy(Arena *a, String_View sv)
{
    String_View *s = arena_alloc(a, sizeof(String_View));
    s->data = arena_alloc(a, sv.count);
    memcpy(s->data, sv.data, sv.count);
    s->count = sv.count;
    return s;
}

LObject obj_from_atom(Arena *a, Atom atom)
{
    LObject o = {0};
     
    switch (atom.t) {
        case ATOM_FLT: o = OBJ_FLT(atom.v.as_flt); break;
        case ATOM_INT: o = OBJ_INT(atom.v.as_int); break;
        case ATOM_STR: o = OBJ_STR(sv_dy(a, atom.v.as_str)); break;
        case ATOM_NIL: o = OBJ_NIL; break;
        default: {
            assert(0 && "Unreachable atom type");
        }
    }
    
    return o;
}

Atom parse_atom(Token tk)
{
    Atom a = {0};

    switch (tk.type) {
        case TK_STRING: {
            a.t = ATOM_STR;
            a.v.as_str = tk.text;
            break;
        }
        case TK_NUMBER: {
            if (sv_is_float(tk.text)) {
                a.t = ATOM_FLT;
                a.v.as_flt = sv_to_flt(tk.text);
            } else {
                a.t = ATOM_INT;
                a.v.as_int = sv_to_int(tk.text);
            }
            break;
        }
        default: {
            report("Invalid token for expr: %u\n", tk.type);
            exit(1);
        }
    }

    return a;
}

Funcall *funcall_new(Arena *a, String_View name)
{
    Funcall *f = arena_alloc(a, sizeof(Funcall));
    f->name = name;
    return f;
}

Funcall *parse_funcall(Arena *a, Lexer *L)
{
    Token tk = lexer_yield(L, TK_OPERATOR);
    if (lexstatus_err(L)) return NULL;

    Funcall *f = funcall_new(a, tk.text);
    tk = lexer_peek(L);

    while (tk.type != TK_CLOSE_PAREN && tk.type != TK_NONE) {
        Expr e = parse_expr(a, L);
        if (e.t == EXPR_NONE) {
            f = NULL;
            break;
        }

        funarg_append(a, &f->args, e);
        tk = lexer_peek(L);
    }

    return f;
}

Expr parse_expr(Arena *a, Lexer *L)
{
    Expr e = {0};
    Token tk = lexer_peek(L);

    switch (tk.type) {
        case TK_NIL:
        case TK_STRING:
        case TK_NUMBER: {
            tk = lexer_next(L);
            e.v.a = parse_atom(tk);
            e.t = EXPR_ATOM;
            break;
        }
        case TK_OPERATOR: case TK_TEXT: {
            e.v.f = parse_funcall(a, L);
            if (!e.v.f) break;
            e.t = EXPR_FUNCALL;
            break;
        }
        case TK_OPEN_PAREN: {
            Statement s = parse_statement(a, L);
            if (s.t == STATEMENT_NONE) break;
            e = stateval(a, &s);
            break;
        }
        default: {
            assert(0 && "Cannot parse current expresion");
        }
    }

    return e;
}

Statement parse_statement(Arena *a, Lexer *L)
{
    Statement s = {0};
    lexer_yield(L, TK_OPEN_PAREN);

    if (lexstatus_err(L)) {
        report("Expected context. Cannot parse anything");
        return STATE_NONE;
    }
   
    s.t = STATEMENT_VOID;
    s.v.e = parse_expr(a, L);

    if (s.v.e.t == EXPR_NONE) goto defer;
    
    lexer_yield(L, TK_CLOSE_PAREN);
    if (lexstatus_err(L)) goto defer;

    return s;

defer:
    return STATE_NONE;
}

Expr statfuncall(Funcall *f)
{
    Expr out = {0};
    String_View builtins = sv_from_cstr(builtin_funcs);
    if (!char_in_sv(builtins, f->name.data[0])) {
        report("Unknown function name `"SV_Fmt"`", SV_Args(f->name));
        report("Note: for now lambda can parse only builtins: + - * /");
        return EXPR_EMPTY;
    }

    if (f->args.items[0].t != EXPR_ATOM) {
        report("Cannot parse non-atom type `%u`", f->args.items[0].t);
        return EXPR_EMPTY;
    } else {
        out.t = EXPR_ATOM;
        out.v.a.t = f->args.items[0].v.a.t;
    }

    switch (f->name.data[0]) {
        case '+': {
            arethOp(+, f->args.items,  f->args.count, &out.v.a);
            break;
        }
        case '-': {
            arethOp(-, f->args.items,  f->args.count, &out.v.a);
            break;
        }
        case '*': {
            arethOp(*, f->args.items,  f->args.count, &out.v.a);
            break;
        }
        case '/': {
            arethOp(/, f->args.items,  f->args.count, &out.v.a);
            break;
        }
        default: {
            assert(0 && "Unreachable funcall");
        }
    }

    return out;
}

Expr stateval(Arena *a, Statement *s)
{
    (void)a;
    Expr output = {0};

    switch (s->t) {
        case STATEMENT_VOID: {
            Expr e = s->v.e;
            if (e.t == EXPR_FUNCALL) {
                output = statfuncall(e.v.f);
                break;
            } else if (e.t == EXPR_ATOM) {
                output = e;
                break;
            } else {
                assert(0 && "Unreachable expr type");
            }
            break;
        }
        default: {
            assert(0 && "Unreachable state type");
        }
    }

    return output;
}

/*
 * Some prints for debuging
 */

#define PADDING(p)      for (size_t i = 0; i < (p); ++i) printf(" ")

LAM_FUNC const char *st2s(Statement_Type t)
{
    switch (t) {
        case STATEMENT_VOID: return "void";
        default: assert(0 && "uncreachable statement type");
    }
}

LAM_FUNC void atom_dump(Atom a)
{
    switch (a.t) {
        case ATOM_INT: printf("%lli", a.v.as_int); break;
        case ATOM_FLT: printf("%lf", a.v.as_flt); break;
        case ATOM_STR: printf(SV_Fmt, SV_Args(a.v.as_str)); break;
        case ATOM_NIL: printf("nil"); break;
        default: {
            assert(0 && "unreachable type of atom");
        }
    }
}

LAM_FUNC void expr_dump(Expr e, size_t pad)
{
     switch (e.t) {
        case EXPR_ATOM: {
            atom_dump(e.v.a);
            break;
        }

        case EXPR_FUNCALL: {
            Funcall *f = e.v.f;
            PADDING(2*pad);
            printf("(funcall\n");

            PADDING(3*pad);
            printf("(name ("SV_Fmt"))\n", SV_Args(f->name));
            
            PADDING(3*pad);
            printf("(args (");

            for (size_t i = 0; i < f->args.count; ++i) {
                expr_dump(f->args.items[i], pad);
                if (i + 1 != f->args.count) printf(" ");
            }

            printf("))\n");
            PADDING(2*pad);
            printf(")\n");
            break;
        }

        default: {
            assert(0 && "unreachable type for expr");
        }
     }
}

LAM_FUNC void statement_dump(Statement *s)
{
    printf("(Statement\n");
    printf("  (type (%s))\n", st2s(s->t));
    printf("  (value\n");
    
    expr_dump(s->v.e, 2);

    printf("  )\n");
    printf(")\n");
}
