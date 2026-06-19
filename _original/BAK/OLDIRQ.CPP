/*
unsigned short _ourss;     // big THX to KARL for his help on IRQ handling
unsigned short _ourds;
unsigned short _oures;
unsigned short _ourfs;
unsigned short _ourgs;
unsigned short _backss;
unsigned short _backds;
unsigned short _backes;
unsigned short _backfs;
unsigned short _backgs;
unsigned long  _backesp;
unsigned long  _backebp;
unsigned long stacklength=32*1024;
void *ourstack;

void backupPMstack(void);
#pragma aux backupPMstack = \
    "cli"\
    "mov _ourss,ss"\
    "mov _ourds,ds"\
    "mov _oures,es"\
    "mov _ourfs,fs"\
    "mov _ourgs,gs"\
    "sti";

void restorePM(void);
#pragma aux restorePM= \
    "mov es,_oures"\
    "mov fs,_ourfs"\
    "mov gs,_ourgs"\
    "mov ebp,ourstack"\
    "mov esp,ourstack"\
    "mov ss,_ourss"\
    "mov ds,_ourds";

void backupCALLERstack(void);
#pragma aux backupCALLERstack = \
    "mov _backesp,esp"\
    "mov _backss,ss" \
    "mov _backds,ds"\
    "mov _backes,es"\
    "mov _backfs,fs"\
    "mov _backgs,gs"\
    "mov _backebp,ebp";

void restoreCALLERstack(void);
#pragma aux restoreCALLERstack = \
    "mov esp,_backesp"\
    "mov ss,_backss"\
    "mov ebp,_backebp"\
    "mov es,_backes"\
    "mov fs,_backfs"\
    "mov gs,_backgs"\
    "mov ds,_backds";
*/