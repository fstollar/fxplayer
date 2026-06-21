#pragma once

enum class FxKey {
    NONE,
    PAUSE,
    QUIT,
    ORDER_FWD,
    ORDER_BWD,
    VOL_UP,
    VOL_DOWN
};

/* Put stdin into raw+noecho mode and set O_NONBLOCK. Call before the input loop. */
void   tty_raw_enable();

/* Restore the terminal to the state saved by tty_raw_enable. Safe to call multiple times. */
void   tty_raw_disable();

/* Non-blocking key read. Returns FxKey::NONE when no input is available.
   Handles VT100 arrow escape sequences (ESC [ A/B/C/D) and bare Esc. */
FxKey  tty_read_key();
