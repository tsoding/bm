#ifndef BDB_H
#define BDB_H

#include "bm.h"

typedef enum {
   BDB_OK = 0,
   BDB_FAIL = 1,
   BDB_EXIT
} bdb_status;

typedef struct bdb_breakpoint
{
    int is_enabled;
    int id;
} bdb_breakpoint;

typedef struct bdb_state
{
    Bm bm;
    String_View cood_file_name;
    String_View symtab_file_name;
    bdb_breakpoint breakpoints[BM_PROGRAM_CAPACITY];
} bdb_state;

void bdb_state_init(bdb_state *, const char *executable, const char *symtab);
bdb_status bdb_step_instr(bdb_state *);
bdb_status bdb_continue(bdb_state *);
void bdb_print_instr(FILE *, Inst *);
void bdb_add_breakpoint(bdb_state *, Inst_Addr);
void bdb_delete_breakpoint(bdb_state *, Inst_Addr);
bdb_status bdb_fault(bdb_state *, Err);

#endif // BDB_H
