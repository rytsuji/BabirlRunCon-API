#ifndef BBJSON_H
#define BBJSON_H
void bbjson_begin(void);
void bbjson_charobj(char *key, char *val);
void bbjson_charobj_single(char *key, char *val);
void bbjson_intobj(char *key, int val);
void bbjson_intobj_single(char *key, int val);
void bbjson_uintobj(char *key, unsigned int val);
void bbjson_lluintobj(char *key, unsigned long long int val);
void bbjson_begin_obj(char *key);
void bbjson_end_obj(void);
void bbjson_begin_array(char *key);
void bbjson_end_array(void);
void bbjson_end(void);
#endif
