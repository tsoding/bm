/*
 * BDB - Debugger for the Birtual Machine
 * Copyright 2020, 2021 by Nico Sonack and Alexey Kutepov
 *
 * Contribution to https://github.com/tsoding/bm
 *
 * License: MIT License. Please refer to the LICENSE file attached to
 * this distribution for more details.
 */

#ifndef BDB_H
#define BDB_H

#include "./bm.h"
#include "./arena.h"
//#include "./basm.h"

#define BDB_BREAKPOINTS_CAPACITY 256
#define BDB_BINDINGS_CAPACITY 1024

typedef enum {
    BDB_OK = 0,
    BDB_FAIL = 1
} Bdb_Err ;

typedef struct Bdb_Breakpoint {
    int is_broken;
    String_View label;
    Inst_Addr addr;
} Bdb_Breakpoint;

typedef struct {
    Type type;
    String_View name;
    Word value;
} Bdb_Binding;

typedef struct Bdb_State {
    Bm bm;

    const char *program_file_path;

    Bdb_Breakpoint breakpoints[BDB_BREAKPOINTS_CAPACITY];
    size_t breakpoints_size;

    Bdb_Binding bindings[BDB_BINDINGS_CAPACITY];
    size_t bindings_size;

    int is_in_step_over_mode;
    unsigned int step_over_mode_call_depth;

    // For temporary things that will be cleaned up on each iterations of "event loop".
    // Lifetime: single iterations of the event loop
    Arena tmp_arena;

    // For any strings from the .sym file or related to it.
    // Lifetime: objects that live for a single execution session
    Arena sym_arena;

    // For labels of breakpoints.
    // Lifetime: objects that survive even after bdb_reset()
    Arena break_arena;
} Bdb_State;

Bdb_Err bdb_state_init(Bdb_State *, const char *program_file_path);
Bdb_Err bdb_load_symtab(Bdb_State *state, const char *program_file_path);
Bdb_Err bdb_step_instr(Bdb_State *);
Bdb_Err bdb_step_over_instr(Bdb_State *state);
Bdb_Err bdb_continue(Bdb_State *);
Bdb_Binding *bdb_resolve_binding(Bdb_State *bdb, String_View name);
Bdb_Err bdb_parse_word(Bdb_State *st, String_View input, Word *output);
Bdb_Err bdb_parse_binding_or_word(Bdb_State *st, String_View input, Word *out);
Bdb_Err bdb_run_command(Bdb_State *, String_View command_word, String_View arguments);
void bdb_print_location(Bdb_State*);
Bdb_Err bdb_reset(Bdb_State *);
void bdb_print_instr(Bdb_State *, FILE *, Inst *);
void bdb_add_breakpoint(Bdb_State *, Inst_Addr, String_View label);
void bdb_delete_breakpoint(Bdb_State *, Inst_Addr);
Bdb_Breakpoint *bdb_find_breakpoint_by_addr(Bdb_State *, Inst_Addr);
Bdb_Err bdb_fault(Bdb_State *, Err);
void bdb_exit(Bdb_State *);

#endif // BDB_H
