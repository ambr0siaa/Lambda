/*
 * It's my version of `readline` library
 * For now it's just testing file. All common fitures not implemented
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#include "arena.h"

#define CTRL_KEY(k) ((k) & 0x1f)
#define INSBUFF_INIT_CAPACITY 32

#define READER_STATUS_OK  1
#define READER_STATUS_ERR 2
#define READER_STATUS_EXT 3
#define READER_STATUS_CON 0

// Dynamic array of characters
typedef struct {
    size_t capacity;
    size_t count;
    char *items;
} Buffer;

typedef enum {
    EVENT_NONE = 0,
    EVENT_CUR_LEFT,
    EVENT_CUR_RIGHT,
    EVENT_HIS_PREV,
    EVENT_HIS_NEXT,
    EVENT_WRITE_CHAR,
    EVENT_BACKSPACE,
    EVENT_NEWLINE,
    EVENT_EXIT
} Event;

typedef struct {
    char c;              // Current character
    int status;          // Event status
    size_t pos;          // Cursor possition
    Buffer buf;          // Output text
    Event event;         // Current event
    struct termios term; // Original terminal setup
} Reader;

#define buff_append(a, ib, item) \
    arena_da_append(a, ib, item, INSBUFF_INIT_CAPACITY)

#define reader_break(r) \
    ((r)->status = READER_STATUS_ERR)

#define reader_statusok(r) \
    ((r)->status == READER_STATUS_OK)

#define reader_statuserr(r) \
    ((r)->status == READER_STATUS_ERR)

#define reader_statuscon(r) \
    ((r)->status == READER_STATUS_CON)

#define reader_statusext(r) \
    ((r)->status == READER_STATUS_EXT)

#define reader_retline(r) \
    do { \
        char nul = '\0'; \
        buff_append(a, &(r)->buf, nul); \
        return (r)->buf.items; \
    } while (0)

void reader_defaultmode(Reader *r)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &r->term);
}

void reader_readmode(Reader *r)
{
    tcgetattr(STDIN_FILENO, &r->term);
    struct termios t = r->term;

    t.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    t.c_iflag &= ~(BRKINT | IXON | ISTRIP | INPCK);
    t.c_cflag |= (CS8);
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 1;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
}

Reader reader_init(const char *prompt)
{
    Reader r = {0};
    r.status = READER_STATUS_CON;
    r.c = r.pos = r.event = 0;
  
    size_t len = strlen(prompt);
    write(STDOUT_FILENO, prompt, len);

    reader_readmode(&r);
    return r;
}

void reader_readchar(Reader *r)
{
    ssize_t n = 0;
    while ((n = read(STDIN_FILENO, &r->c, 1)) != 1)
        if (n == -1) reader_break(r);
}

void reader_handle_event(Reader *r)
{
    reader_readchar(r);
    if (reader_statuserr(r)) return;
    
    switch (r->c) {

        case CTRL_KEY('q'): {
            r->event = EVENT_EXIT;
            return;
        }
        case '\n': {
            r->event = EVENT_NEWLINE;
            return;
        }
        case '\b': {
            r->event = EVENT_BACKSPACE;
            return;
        }
        case '\x1b': {
            char seq[2];

            if (read(STDIN_FILENO, &seq[0], 1) != 1) break;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) break;

            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A': r->event = EVENT_HIS_NEXT;  break;
                    case 'B': r->event = EVENT_HIS_PREV;  break;
                    case 'C': r->event = EVENT_CUR_RIGHT; break;
                    case 'D': r->event = EVENT_CUR_LEFT;  break;
                }
            }

            return;
        }
    }

    r->event = EVENT_WRITE_CHAR;
}

void reader_movecursor(Reader *r, int direct)
{
    char cursor[5];
    if (direct) {
        snprintf(cursor, 5, "\x1b[%dC", 1);
        r->pos += 1;
    } else {
        snprintf(cursor, 5, "\x1b[%dD", 1); 
        r->pos -= 1;
    }
    write(STDOUT_FILENO, cursor, 5);
}

char *reader_doevent(Arena *a, Reader *r)
{
    switch (r->event) {
        case EVENT_NEWLINE: {
            printf("\n");
            r->status = READER_STATUS_OK;
            reader_retline(r);
        }
        case EVENT_WRITE_CHAR: {
            write(STDOUT_FILENO, &r->c, 1);
            buff_append(a, &r->buf, r->c);
            r->pos += 1;
            break;
        }
        case EVENT_CUR_LEFT: {
            if (r->pos > 0)
                reader_movecursor(r, 0);
            break;
        }
        case EVENT_CUR_RIGHT: {
            if (r->pos < r->buf.count)
                reader_movecursor(r, 1);
            break;
        }
        case EVENT_EXIT: {
            r->status = READER_STATUS_EXT;
            break;
        }
        default: {
            r->status = READER_STATUS_ERR;
            break;
        }
    }
    
    return NULL;
}

char *readline(Arena *a, const char *prompt)
{
    char *line = NULL;
    Reader r = reader_init(prompt);

    while (reader_statuscon(&r)) {
        reader_handle_event(&r);
        if (!reader_statuserr(&r)) {
            line = reader_doevent(a, &r);
        }
    }
    
    reader_defaultmode(&r);
    return line;
}

/*
 * TODO:
 *  - Introduce `EVENT_BACKSPACE` and moving cursor
 *  - Introduce history, searching in it and loading
 */
int main(void)
{
    Arena a = {0};

    for (;;) {
        char *line = readline(&a, "> ");
        if (line) {
            printf("%s\n", line);
        } else {
            break;
        }
    }
    
    arena_free(&a);
    return 0;
}
