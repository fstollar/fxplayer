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

void   tty_raw_enable();
void   tty_raw_disable();
FxKey  tty_read_key();
