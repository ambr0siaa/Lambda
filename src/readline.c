#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include "readline.h"

RLAPI void reader_print(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    fflush(stdout);
    va_end(args);
}

#define reader_draw_cursor(r) reader_print("\x1b[%zu;%zuH", (r)->cy, (r)->cx);

#define reader_retline(r) \
    do { \
        buff_insert(&(r)->buf, '\0', (r)->buf.count); \
        return (r)->buf.items; \
    } while (0)

RLAPI void buff_insert(Buffer *buf, char item, size_t index)
{
    if (index >= 0 && index <= buf->count) {
        Buffer new = {0};
        new.count = buf->count + 1;
        new.capacity = buf->capacity > 0 ? buf->capacity : BUFF_INIT_CAPACITY;
        new.capacity = new.capacity < new.count ? new.capacity*2 : new.capacity;
        new.items = malloc(new.capacity);

        new.items[index] = item;
        memcpy(new.items, buf->items, index);
        memcpy(new.items + index + 1, buf->items + index, buf->count - index);
        
        new.items[new.count] = '\0';
        free(buf->items);
        *buf = new;
    }
}

RLAPI void buff_delete(Buffer *buf, char index)
{
    Buffer new = {0};
    new.count = buf->count - 1;
    new.capacity = buf->count * 2;
    new.items = malloc(new.capacity);
    memset(new.items, '\0', new.capacity);

    memcpy(new.items, buf->items, index);
    memcpy(new.items + index, buf->items + index + 1, buf->count - index - 1);

    free(buf->items);
    *buf = new;
}

RLAPI int reader_cursor_pos(size_t *y, size_t *x)
{
    char buf[32];
    size_t i = 0;
   
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
    
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1 || buf[i] == 'R') break;
        ++i;
    }

    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return 0;
    if (sscanf(&buf[2], "%zu;%zu", y, x) != 2) return 0;

    return 1;
}

RLAPI void reader_defaultmode(Reader *r)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &r->term);
}

RLAPI void reader_readmode(Reader *r)
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

RLAPI Reader reader_init(const char *prompt)
{
    Reader r = {0};
    r.status = READER_STATUS_CON;
    r.ch = r.cx = r.cy = r.event = 0;
    r.offset = prompt != NULL ? strlen(prompt) : 0;

    reader_readmode(&r);
    reader_cursor_pos(&r.cy, &r.cx);
    r.cx += r.offset;
    
    return r;
}

RLAPI void reader_readchar(Reader *r)
{
    ssize_t n = 0;
    while ((n = read(STDIN_FILENO, &r->ch, 1)) != 1)
        if (n == -1) reader_break(r);
}

RLAPI void reader_handle_event(Reader *r)
{
    reader_readchar(r);
    if (reader_statuserr(r)) return;
    
    switch (r->ch) {
        case CTRL_KEY('q'): {
            r->event = EVENT_EXIT;
            return;
        }
        case '\n': {
            r->event = EVENT_NEWLINE;
            return;
        }
        case '\x8': case '\x7f': {
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

RLAPI char *reader_doevent(Reader *r)
{
    size_t left_boundary = r->cx > r->offset + 1;
    size_t right_boundary = r->cx <= r->buf.count + r->offset;

    switch (r->event) {
        case EVENT_NEWLINE: {
            reader_print("\n");
            r->status = READER_STATUS_OK;
            reader_retline(r);
        }
        case EVENT_WRITE_CHAR: {
            size_t pos = r->cx;
            buff_insert(&r->buf, r->ch, pos - r->offset - 1);

            r->cx -= (pos - r->offset - 1);
            reader_draw_cursor(r);

            reader_print("%s", r->buf.items);

            r->cx = pos + 1;
            reader_draw_cursor(r);
        
            break;
        }
        case EVENT_BACKSPACE: {
            if (left_boundary && right_boundary + 1) {
                size_t pos = r->cx;
                buff_delete(&r->buf, pos - r->offset - 2);

                r->cx -= (pos - r->offset - 1);
                reader_draw_cursor(r);
                reader_print("\x1b[K");

                reader_print("%s", r->buf.items);

                r->cx = pos - 1;
                reader_draw_cursor(r);
            }
            break;
        }
        case EVENT_CUR_LEFT: {
            if (left_boundary) {
                r->cx -= 1;
                reader_draw_cursor(r);
            }
            break;
        }
        case EVENT_CUR_RIGHT: {
            if (right_boundary) {
                r->cx += 1;
                reader_draw_cursor(r);
            }
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

char *readline(const char *prompt)
{
    char *line = NULL;
    Reader r = reader_init(prompt);
    reader_print(prompt);

    while (reader_statuscon(&r)) {
        reader_handle_event(&r);
        if (!reader_statuserr(&r)) {
            line = reader_doevent(&r);
        }
    }
    
    reader_defaultmode(&r);
    return line;
}
