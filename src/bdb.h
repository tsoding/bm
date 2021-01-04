#ifndef BDB_H
#define BDB_H

#include "bm.h"

typedef enum {
   BDB_OK = 0,
   BDB_FAIL = 1
} bdb_status;

typedef struct bdb_state
{
    Bm bm;
    String_View cood_file_name;
    String_View symtab_file_name;
} bdb_state;

void bdb_state_init(bdb_state *, const char *executable, const char *symtab);
bdb_status bdb_step_instr(bdb_state *);
void bdb_print_instr(FILE *, Inst *);

#endif // BDB_H
