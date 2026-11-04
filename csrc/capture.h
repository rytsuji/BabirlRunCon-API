#ifndef BABI_CAPTURE_H
#define BABI_CAPTURE_H
#include <stdio.h>
#include <sys/sem.h>
int babi_printf(const char *, ...);
unsigned short babi_eb_port(void);
int babi_get_es_events(void);
void babi_abort(const char *);
#define printf babi_printf
#endif
