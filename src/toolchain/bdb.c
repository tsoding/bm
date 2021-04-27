/*
 * BDB - Debugger for the Birtual Machine
 * Copyright 2020, 2021 by Nico Sonack and Alexey Kutepov
 *
 * Contribution to https://github.com/tsoding/bm
 *
 * License: MIT License. Please refer to the LICENSE file attached to
 * this distribution for more details.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "bdb.h"

#define BM_IMPLEMENTATION
#include "bm.h"

#define INPUT_CAPACITY 32

static
void usage(void)
{
    fprintf(stderr,
            "usage: bdb <executable>\n");
}

Bdb_Binding *bdb_resolve_binding(Bdb_State *bdb, String_View name)
{
    assert(bdb);

    for (size_t i = 0; i < bdb->bindings_size; ++i) {
        if (sv_eq(bdb->bindings[i].name, name)) {
            return &bdb->bindings[i];
        }
    }

    return NULL;
}

Bdb_Err bdb_load_symtab(Bdb_State *state, const char *program_file_path)
{
    assert(state);

    String_View symtab_file =
        sv_from_cstr(CSTR_CONCAT(&state->sym_arena, program_file_path, ".sym"));

    String_View symtab = {0};
    if (arena_slurp_file(&state->sym_arena, symtab_file, &symtab) < 0) {
        fprintf(stderr, "ERROR: could not read file "SV_Fmt": %s\n",
                SV_Arg(symtab_file), strerror(errno));
        exit(1);
    }

    while (symtab.count > 0) {
        symtab                    = sv_trim_left(symtab);
        String_View  raw_addr     = sv_chop_by_delim(&symtab, '\t');
        symtab                    = sv_trim_left(symtab);
        String_View  raw_sym_type = sv_chop_by_delim(&symtab, '\t');
        symtab                    = sv_trim_left(symtab);
        String_View  name         = sv_chop_by_delim(&symtab, '\n');
        Word         value        = word_u64(sv_to_u64(raw_addr));
        Type type                 = (Type)sv_to_u64(raw_sym_type);

        state->bindings[state->bindings_size].name = name;
        state->bindings[state->bindings_size].value = value;
        state->bindings[state->bindings_size].type = type;
        state->bindings_size += 1;
    }

    return BDB_OK;
}

// TODO(#187): bdb_print_instr should take information from the actual source code
void bdb_print_instr(Bdb_State *state, FILE *f, Inst *i)
{
    (void) state;               // NOTE: don't forget to remove this
    // when you start using the state.

    Inst_Def inst_def = get_inst_def(i->type);
    fprintf(f, "%s ", inst_def.name);
    if (inst_def.has_operand) {
        fprintf(f, "%" PRIu64, i->operand.as_i64);
    }
}

Bdb_Breakpoint *bdb_find_breakpoint_by_addr(Bdb_State *bdb, Inst_Addr addr)
{
    for (size_t i = 0; i < bdb->breakpoints_size; ++i) {
        if (bdb->breakpoints[i].addr == addr) {
            return &bdb->breakpoints[i];
        }
    }

    return NULL;
}

void bdb_add_breakpoint(Bdb_State *state, Inst_Addr addr, String_View label)
{
    assert(state);
    assert(state->breakpoints_size < BDB_BREAKPOINTS_CAPACITY);

    if (bdb_find_breakpoint_by_addr(state, addr)) {
        fprintf(stderr, "ERR : Breakpoint already set\n");
        return;
    }

    state->breakpoints[state->breakpoints_size].is_broken = 0;
    state->breakpoints[state->breakpoints_size].label =
        arena_sv_dup(&state->break_arena, label); // NOTE: moving the label to an arena
    // with longer lifetime
    state->breakpoints[state->breakpoints_size].addr = addr;
    state->breakpoints_size += 1;

    fprintf(stdout, "INFO : Breakpoint set at %"PRIu64"\n", addr);
}

void bdb_delete_breakpoint(Bdb_State *state, Inst_Addr addr)
{
    assert(state);

    Bdb_Breakpoint *breakpoint = bdb_find_breakpoint_by_addr(state, addr);
    if (breakpoint == NULL) {
        fprintf(stderr, "ERR : No such breakpoint\n");
        return;
    }

    assert(breakpoint <= state->breakpoints);

    state->breakpoints_size -= 1;

    const size_t n = (state->breakpoints_size - (size_t) (state->breakpoints - breakpoint)) * sizeof(state->breakpoints[0]);
    memmove(breakpoint, breakpoint + 1, n);

    if (state->breakpoints_size == 0) {
        // NOTE: since all of the breakpoints are removed nothing from break_arena
        // need at this point. We can safely clean it up.
        arena_clean(&state->break_arena);
    }

    fprintf(stdout, "INFO : Deleted breakpoint at %"PRIu64"\n", addr);
}

Bdb_Err bdb_continue(Bdb_State *state)
{
    assert(state);

    if (state->bm.halt) {
        fprintf(stderr, "ERR : Program is not being run\n");
        return BDB_OK;
    }

    do {
        if (state->is_in_step_over_mode) {
            if (state->bm.program[state->bm.ip].type == INST_CALL) {
                state->step_over_mode_call_depth += 1;
            } else if (state->bm.program[state->bm.ip].type == INST_RET) {
                state->step_over_mode_call_depth -= 1;
            }
        }

        Bdb_Breakpoint *bp = bdb_find_breakpoint_by_addr(state, state->bm.ip);
        if (bp != NULL) {
            if (!bp->is_broken) {
                fprintf(stdout, "Hit breakpoint at %"PRIu64, state->bm.ip);

                if (bp->label.data) {
                    fprintf(stdout, " label '"SV_Fmt"'", SV_Arg(bp->label));
                }

                fprintf(stdout, "\n");
                bp->is_broken = 1;

                state->step_over_mode_call_depth = 0;
                state->is_in_step_over_mode = 0;

                return BDB_OK;
            } else {
                bp->is_broken = 0;
            }
        }

        Err err = bm_execute_inst(&state->bm);
        if (err) {
            return bdb_fault(state, err);
        }

        if (state->is_in_step_over_mode &&
                state->step_over_mode_call_depth == 0) {
            return BDB_OK;
        }

    } while (!state->bm.halt);

    printf("Program halted.\n");

    return BDB_OK;
}

Bdb_Err bdb_fault(Bdb_State *state, Err err)
{
    assert(state);

    fprintf(stderr, "%s at %" PRIu64 " (INSTR: ",
            err_as_cstr(err), state->bm.ip);
    bdb_print_instr(state, stderr, &state->bm.program[state->bm.ip]);
    fprintf(stderr, ")\n");
    state->bm.halt = 1;
    return BDB_OK;
}

Bdb_Err bdb_step_over_instr(Bdb_State *state)
{
    state->is_in_step_over_mode = 1;
    state->step_over_mode_call_depth = 0;

    Bdb_Err err = bdb_continue(state);
    state->is_in_step_over_mode = 0;

    return err;
}

Bdb_Err bdb_step_instr(Bdb_State *state)
{
    assert(state);

    if (state->bm.halt) {
        fprintf(stderr, "ERR : Program is not being run\n");
        return BDB_OK;
    }

    Err err = bm_execute_inst(&state->bm);
    if (!err) {
        return BDB_OK;
    } else {
        return bdb_fault(state, err);
    }
}

Bdb_Err bdb_parse_word(Bdb_State *st, String_View sv, Word *output)
{
    const char *cstr = arena_sv_to_cstr(&st->tmp_arena, sv);
    char *endptr = NULL;

    // IMPORTANT!!! strtoull expects NULL-terminated string.
    uint64_t value = strtoull(cstr, &endptr, 10);
    if (endptr != cstr + sv.count) {
        return BDB_FAIL;
    }

    if (output) {
        output->as_u64 = value;
    }

    return BDB_OK;
}

// TODO(#188): bdb should be able to break on BASM expression
Bdb_Err bdb_parse_binding_or_word(Bdb_State *st, String_View input, Word *output)
{
    if (bdb_parse_word(st, input, output) == BDB_FAIL) {
        Bdb_Binding *binding = bdb_resolve_binding(st, input);
        if (binding) {
            *output = binding->value;
        } else {
            return BDB_FAIL;
        }
    }

    return BDB_OK;
}

void bdb_print_location(Bdb_State *state)
{
    assert(state);

    Inst_Addr ip = state->bm.ip;
    const Bdb_Binding *location = NULL;

    for (size_t i = 0; i < state->bindings_size; ++i) {
        const Bdb_Binding *current = &state->bindings[i];
        if (current->type == TYPE_INST_ADDR && current->value.as_u64 <= ip) {
            if (location == NULL || location->value.as_u64 < current->value.as_u64) {
                location = current;
            }
        }
    }

    if (location) {
        printf("At address %"PRIu64": "SV_Fmt"+%"PRIu64"\n",
               ip, SV_Arg(location->name), ip - location->value.as_u64);
    } else {
        printf("ip = %"PRIu64"\n"
               "WARN : No location info available\n",
               ip);
    }
}

Bdb_Err bdb_reset(Bdb_State *state)
{
    // TODO(#276): bdb does not support native function loading
    bm_load_program_from_file(&state->bm, state->program_file_path);
    state->bm.halt = 1;

    arena_clean(&state->sym_arena);
    state->bindings_size = 0;
    state->is_in_step_over_mode = 0;
    state->step_over_mode_call_depth = 0;

    fprintf(stdout, "INFO : Loading debug symbols...\n");
    if (bdb_load_symtab(state, state->program_file_path) == BDB_FAIL) {
        return BDB_FAIL;
    }

    // Update addresses of breakpoints on labels
    for (size_t i = 0; i < state->breakpoints_size; ++i) {
        state->breakpoints[i].is_broken = 0;

        if (state->breakpoints[i].label.data) {
            Bdb_Binding *binding = bdb_resolve_binding(state, state->breakpoints[i].label);
            if (binding) {
                state->breakpoints[i].addr = binding->value.as_u64;
            } else {
                fprintf(stderr, "WARNING: label `"SV_Fmt"` got removed\n",
                        SV_Arg(state->breakpoints[i].label));
                state->breakpoints[i].label = SV_NULL;
            }
        }
    }

    return BDB_OK;
}

void bdb_exit(Bdb_State *state)
{
    printf("Bye\n");
    arena_free(&state->sym_arena);
    arena_free(&state->break_arena);
    arena_free(&state->tmp_arena);
    exit(EXIT_SUCCESS);
}

Bdb_Err bdb_run_command(Bdb_State *state, String_View command_word, String_View arguments)
{
    switch (*command_word.data) {
    /*
     * Next instruction
     */
    case 's': {
        Bdb_Err err = bdb_step_instr(state);
        if (err) {
            return err;
        }

        printf("-> ");
        bdb_print_instr(state, stdout, &state->bm.program[state->bm.ip]);
        printf("\n");
    }
    break;
    /*
     * Step over instruction
     */
    case 'n': {
        Bdb_Err err = bdb_step_over_instr(state);
        if (err) {
            return err;
        }

        printf("-> ");
        bdb_print_instr(state, stdout, &state->bm.program[state->bm.ip]);
        printf("\n");
    }
    break;
    /*
     * Inspect the memory
     */
    case 'x': {
        arguments = sv_trim(arguments);
        String_View where_sv = sv_chop_by_delim(&arguments, ' ');
        String_View count_sv = arguments;

        Word where = word_u64(0);
        if (bdb_parse_binding_or_word(state, where_sv, &where) == BDB_FAIL) {
            fprintf(stderr, "ERR : Cannot parse address, label or constant `"SV_Fmt"`\n", SV_Arg(where_sv));
            return BDB_FAIL;
        }

        Word count = word_u64(0);
        if (bdb_parse_binding_or_word(state, count_sv, &count) == BDB_FAIL) {
            fprintf(stderr, "ERR : Cannot parse address, label or constant `"SV_Fmt"`\n", SV_Arg(count_sv));
            return BDB_FAIL;
        }

        for (uint64_t i = 0;
                i < count.as_u64 && where.as_u64 + i < BM_MEMORY_CAPACITY;
                ++i) {
            printf("%02X ", state->bm.memory[where.as_u64 + i]);
        }
        printf("\n");

    }
    break;
    /*
     * Dump the stack
     */
    case 'p': {
        bm_dump_stack(stdout, &state->bm);
    }
    break;
    /*
     * Where - print information about the current location in the program
     */
    case 'w': {
        bdb_print_location(state);
    }
    break;

    case 'b': {
        String_View addr = sv_trim(arguments);
        Bdb_Binding *binding = bdb_resolve_binding(state, addr);
        if (binding) {
            if (binding->type == TYPE_INST_ADDR) {
                bdb_add_breakpoint(state, binding->value.as_u64, binding->name);
            } else {
                fprintf(stderr, "ERR : Expected %s but got %s\n",
                        type_name(TYPE_INST_ADDR),
                        type_name(binding->type));
            }
        } else {
            Word value = {0};

            if (bdb_parse_word(state, addr, &value) == BDB_FAIL) {
                fprintf(stderr, "ERR : `"SV_Fmt"` is not a number\n",
                        SV_Arg(addr));
            }

            bdb_add_breakpoint(state, value.as_u64, SV_NULL);
        }
    }
    break;
    case 'd': {
        String_View addr = sv_trim(arguments);
        Bdb_Binding *binding = bdb_resolve_binding(state, addr);
        if (binding) {
            if (binding->type == TYPE_INST_ADDR) {
                bdb_delete_breakpoint(state, binding->value.as_u64);
            } else {
                fprintf(stderr, "ERR : Expected %s but got %s\n",
                        type_name(TYPE_INST_ADDR),
                        type_name(binding->type));
            }
        } else {
            Word value = {0};

            if (bdb_parse_word(state, addr, &value) == BDB_FAIL) {
                fprintf(stderr, "ERR : `"SV_Fmt"` is not a number\n",
                        SV_Arg(addr));
            }

            bdb_delete_breakpoint(state, value.as_u64);
        }
    }
    break;
    case 'r': {
        if (!state->bm.halt || (state->bm.halt && state->bm.program[state->bm.ip].type == INST_HALT)) {
            if (state->bm.halt) {
                fprintf(stderr,
                        "INFO : Program has halted.\n");
            } else {
                fprintf(stderr,
                        "INFO : Program is already running.\n");
            }

            int answer;

            /*
             * NOTE(Nico):
             *
             * Dear Dijkstra-Gang, if I wouldn't use a goto here, I
             * would be using a do-while-loop. That however would
             * duplicate the iteration condition and the switch
             * cases. So please don't sue me for using a goto. It is
             * way easier to read this way.
             */
ask_again:
            printf("     : Restart the program? [y|N] ");
            answer = getchar();

            switch (answer) {
            case 'y':
            case 'Y': {
                // Consume the '\n'
                getchar();

                Bdb_Err err = bdb_reset(state);
                if (err != BDB_OK) {
                    return err;
                }
            }
            break;
            case 'n':
            case 'N':
                getchar(); // See above
            /* fall through */
            case '\n':
                return BDB_OK;

            case EOF:
                bdb_exit(state);
            default:
                goto ask_again;
            }
        }

        state->bm.halt = 0;
        } /* fall through */
    case 'c': {
        return bdb_continue(state);
    }
    case 'q': {
        bdb_exit(state);
    }
    break;
    case 'h': {
        printf("r - run program\n"
               "n - step over instruction\n"
               "s - step instruction\n"
               "c - continue program execution\n"
               "p - print a stack dump\n"
               "w - print location info\n"
               "x - inspect the memory\n"
               "b - set breakpoint at address or label\n"
               "d - destroy breakpoint at address or label\n"
               "q - quit\n");
    }
    break;
    default: {
        fprintf(stderr, "?\n");
    }
    break;
    }

    return BDB_OK;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage();
        return EXIT_FAILURE;
    }

    // NOTE: The structure might be quite big due its arena. Better allocate it in the static memory.
    static Bdb_State state = {0};
    state.program_file_path = argv[1];

    printf("BDB - The birtual machine debugger.\n"
           "Type 'h' and enter for a quick help\n");
    if (bdb_reset(&state) == BDB_FAIL) {
        fprintf(stderr,
                "FATAL : Unable to initialize the debugger: %s\n",
                strerror(errno));
    }

    /*
     * NOTE: This is a temporary buffer to store the previous
     * command. In the future, this may be removed, if we decide to do
     * proper command parsing and have a dedicated place to store the
     * command history. For now we will just store the last one and
     * repeat that if the user just hits enter.
     */
    char previous_command[INPUT_CAPACITY] = {0};

    while (!feof(stdin)) {
        printf("(bdb) ");
        char input_buf[INPUT_CAPACITY] = {0};
        if (fgets(input_buf, INPUT_CAPACITY, stdin) == NULL) {
            bdb_exit(&state);
        }

        String_View
        input_sv = sv_trim(sv_from_cstr(input_buf)),
        control_word = sv_trim(sv_chop_by_delim(&input_sv, ' '));

        /*
         * Check if we need to repeat the previous command
         */
        if (control_word.count == 0 && *previous_command != '\0') {
            input_sv = sv_trim(sv_from_cstr(previous_command));
            control_word = sv_trim(sv_chop_by_delim(&input_sv, ' '));
        }

        if (control_word.count > 0) {
            Bdb_Err err = bdb_run_command(&state, control_word, input_sv);

            if (err == BDB_OK && *input_buf != '\n') {
                /*
                 * Store the previous command for repeating it.
                 */
                memcpy(previous_command, input_buf, sizeof(char) * INPUT_CAPACITY);
            }
        }

        arena_clean(&state.tmp_arena);
    }

    bdb_exit(&state);
}
