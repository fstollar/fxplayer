#include "tty.h"

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

static struct termios s_saved_termios;
static int            s_saved_flags = -1;

void tty_raw_enable()
{
    struct termios raw;
    tcgetattr(STDIN_FILENO, &s_saved_termios);
    raw = s_saved_termios;
    raw.c_lflag &= ~(tcflag_t)(ECHO | ICANON);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    s_saved_flags = fcntl(STDIN_FILENO, F_GETFL);
    fcntl(STDIN_FILENO, F_SETFL, s_saved_flags | O_NONBLOCK);
}

void tty_raw_disable()
{
    if (s_saved_flags >= 0) {
        fcntl(STDIN_FILENO, F_SETFL, s_saved_flags);
        s_saved_flags = -1;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &s_saved_termios);
}

FxKey tty_read_key()
{
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1)
        return FxKey::NONE;

    switch (c) {
    case ' ':  return FxKey::PAUSE;
    case 'q':  return FxKey::QUIT;
    case ',':  return FxKey::ORDER_BWD;
    case '.':  return FxKey::ORDER_FWD;
    case '+':  return FxKey::VOL_UP;
    case '-':  return FxKey::VOL_DOWN;

    case 0x1b: {
        /*
         * Disambiguate bare Esc from VT100 arrow-key escape sequences.
         * Arrow keys send three bytes: ESC '[' followed by A/B/C/D.
         * A bare Esc has nothing following it, so the next read returns 0
         * bytes (stdin is O_NONBLOCK). Any other sequence is discarded.
         */
        unsigned char seq[2] = {0, 0};
        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return FxKey::QUIT;   /* bare Esc — no continuation byte */
        if (seq[0] != '[')
            return FxKey::NONE;   /* unknown escape sequence */
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return FxKey::NONE;
        switch (seq[1]) {
        case 'A': return FxKey::VOL_UP;     /* arrow up    */
        case 'B': return FxKey::VOL_DOWN;   /* arrow down  */
        case 'C': return FxKey::ORDER_FWD;  /* arrow right */
        case 'D': return FxKey::ORDER_BWD;  /* arrow left  */
        default:  return FxKey::NONE;
        }
    }

    default: return FxKey::NONE;
    }
}
