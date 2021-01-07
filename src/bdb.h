/*
 * BDB - Debugger for the Birtual Machine
 * Copyright 2020 by Nico Sonack
 *
 * Contribution to https://github.com/tsoding/bm
 */

#ifndef BDB_H
#define BDB_H

#include "bm.h"

typedef enum {
   BDB_OK = 0,
   BDB_FAIL = 1,
   BDB_EXIT
} Bdb_Err;

typedef struct bdb_breakpoint
{
    int is_enabled;
    int id;
    int is_broken;
} bdb_breakpoint;

typedef struct bdb_state
{
    Bm bm;
    String_View cood_file_name;
    bdb_breakpoint breakpoints[BM_PROGRAM_CAPACITY];
    String_View labels[BM_PROGRAM_CAPACITY];
} bdb_state;

Bdb_Err bdb_state_init(bdb_state *, const char *executable);
Bdb_Err bdb_load_symtab(bdb_state *, const char *symtab);
Bdb_Err bdb_step_instr(bdb_state *);
Bdb_Err bdb_continue(bdb_state *);
Bdb_Err bdb_find_addr_of_label(bdb_state *, const char *, Inst_Addr *);
Bdb_Err bdb_parse_label_or_addr(bdb_state *, const char *, Inst_Addr *);
Bdb_Err bdb_mmap_file(const char *, String_View *);
void bdb_print_instr(FILE *, Inst *);
void bdb_add_breakpoint(bdb_state *, Inst_Addr);
void bdb_delete_breakpoint(bdb_state *, Inst_Addr);
Bdb_Err bdb_fault(bdb_state *, Err);

#endif // BDB_H
