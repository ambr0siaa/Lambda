#ifndef READLINE_H_
#define READLINE_H_

#include <stdlib.h>
#include <termios.h>

#define RLAPI static inline

#define READER_STATUS_OK  1
#define READER_STATUS_ERR 2
#define READER_STATUS_EXT 3
#define READER_STATUS_CON 0

#define CTRL_KEY(k) ((k) & 0x1f)
#define BUFF_INIT_CAPACITY 32

typedef enum {
    EVENT_NONE = 0,   // Empty event that doing anything
    EVENT_CUR_LEFT,   // Moving cursor to left                              | left arrow
    EVENT_CUR_RIGHT,  // Moving cursor to right                             | right arrow
    EVENT_HIS_PREV,   // Put in buffer previous line from history file      | up arrow
    EVENT_HIS_NEXT,   // Put in buffer next line from history file          | down arrow
    EVENT_NEWLINE,    // Returns buffer items if new line `\n` was provided | enter
    EVENT_WRITE_CHAR, // Inserting input character to buffer
    EVENT_BACKSPACE,  // Deleting character from buffer
    EVENT_EXIT        // Breaking reading of line
} Event;

// Dynamic array of characters
typedef struct {
    size_t capacity;
    size_t count;
    char *items;
} Buffer;

typedef struct {
    char ch;             // Current character
    int status;          // Event status
    Buffer buf;          // Output text
    Event event;         // Current event
    size_t cx, cy;       // Cursor possition
    size_t offset;       // Offset made by prompt
    struct termios term; // Original terminal setup
} Reader;

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

#define line_destroy(l) free(l);            // Uses when read line doesn't need more

extern char *readline(const char *prompt);  // Returns read line, at start prints provided prompt

#endif // READLINE_H_
