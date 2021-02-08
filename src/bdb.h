/*
 * BDB - Debugger for the Birtual Machine
 * Copyright 2020 by Nico Sonack
 *
 * Contribution to https://github.com/tsoding/bm
 */

#ifndef BDB_H
#define BDB_H

#include "bm.h"

#define BDB_BREAKPOINTS_CAPACITY 256

typedef enum {
   BDB_OK = 0,
   BDB_FAIL = 1,
   BDB_EXIT
} Bdb_Err ;

typedef struct Bdb_Breakpoint {
    int is_broken;
    String_View label;
    Inst_Addr addr;
} Bdb_Breakpoint;

typedef struct {
    Binding_Kind kind;
    String_View name;
    Word value;
} Bdb_Binding;

typedef struct Bdb_State {
    Bm bm;

    const char *program_file_path;

    Bdb_Breakpoint breakpoints[BDB_BREAKPOINTS_CAPACITY];
    size_t breakpoints_size;

    Bdb_Binding bindings[BASM_BINDINGS_CAPACITY];
    size_t bindings_size;

    int is_in_step_over_mode;
    unsigned int step_over_mode_call_depth;

    Arena sym_arena;
    Arena break_arena;
} Bdb_State;

Bdb_Err bdb_state_init(Bdb_State *, const char *program_file_path);
Bdb_Err bdb_load_symtab(Bdb_State *state, const char *program_file_path);
Bdb_Err bdb_step_instr(Bdb_State *);
Bdb_Err bdb_step_over_instr(Bdb_State *state);
Bdb_Err bdb_continue(Bdb_State *);
Bdb_Binding *bdb_resolve_binding(Bdb_State *bdb, String_View name);
Bdb_Err bdb_parse_binding_or_value(Bdb_State *st, String_View addr, Word *out);
Bdb_Err bdb_run_command(Bdb_State *, String_View command_word, String_View arguments);
void bdb_print_location(Bdb_State*);
Bdb_Err bdb_reset(Bdb_State *);
void bdb_print_instr(Bdb_State *, FILE *, Inst *);
void bdb_add_breakpoint(Bdb_State *, Inst_Addr, String_View label);
void bdb_delete_breakpoint(Bdb_State *, Inst_Addr);
Bdb_Breakpoint *bdb_find_breakpoint_by_addr(Bdb_State *, Inst_Addr);
Bdb_Err bdb_fault(Bdb_State *, Err);

#endif // BDB_H
