/*
 * BDB - Debugger for the Birtual Machine
 * Copyright 2020 by Nico Sonack
 *
 * Contribution to https://github.com/tsoding/bm
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

Bdb_Err bdb_state_init(Bdb_State *state,
                       const char *program_file_path)
{
    assert(state);
    assert(program_file_path);

    state->program_file_path = program_file_path;
    bm_load_program_from_file(&state->bm, program_file_path);
    state->bm.halt = 1;
    bm_load_standard_natives(&state->bm);

    fprintf(stdout, "INFO : Loading debug symbols...\n");
    return bdb_load_symtab(state, program_file_path);
}

Bdb_Err bdb_resolve_binding(const Bdb_State *bdb, String_View name, Bdb_Binding *binding)
{
    assert(bdb);

    for (size_t i = 0; i < bdb->bindings_size; ++i) {
        if (sv_eq(bdb->bindings[i].name, name)) {
            if (binding) *binding = bdb->bindings[i];
            return BDB_OK;
        }
    }

    return BDB_FAIL;
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
        Binding_Kind kind         = (Binding_Kind)sv_to_u64(raw_sym_type);

        state->bindings[state->bindings_size].name = name;
        state->bindings[state->bindings_size].value = value;
        state->bindings[state->bindings_size].kind = kind;
        state->bindings_size += 1;
    }

    return BDB_OK;
}

// TODO: bdb_print_instr should take information from the actual source code
void bdb_print_instr(Bdb_State *state, FILE *f, Inst *i)
{
    (void) state;               // NOTE: don't forget to remove this
                                // when you start using the state.

    fprintf(f, "%s ", inst_name(i->type));
    if (inst_has_operand(i->type))
    {
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

void bdb_add_breakpoint(Bdb_State *state, Inst_Addr addr)
{
    assert(state);
    assert(state->breakpoints_size < BDB_BREAKPOINTS_CAPACITY);

    if (bdb_find_breakpoint_by_addr(state, addr)) {
        fprintf(stderr, "ERR : Breakpoint already set\n");
        return;
    }

    state->breakpoints[state->breakpoints_size].is_broken = 0;
    state->breakpoints[state->breakpoints_size].label = SV_NULL;
    state->breakpoints[state->breakpoints_size].addr = addr;
    state->breakpoints_size += 1;
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
    memmove(breakpoint,
            breakpoint + 1,
            state->breakpoints_size - (size_t) (state->breakpoints - breakpoint));
}

Bdb_Err bdb_continue(Bdb_State *state)
{
    assert(state);

    if (state->bm.halt)
    {
        fprintf(stderr, "ERR : Program is not being run\n");
        return BDB_OK;
    }

    do
    {
        if (state->is_in_step_over_mode)
        {
            if (state->bm.program[state->bm.ip].type == INST_CALL)
            {
                state->step_over_mode_call_depth += 1;
            }
            else if (state->bm.program[state->bm.ip].type == INST_RET)
            {
                state->step_over_mode_call_depth -= 1;
            }
        }

        Bdb_Breakpoint *bp = bdb_find_breakpoint_by_addr(state, state->bm.ip);
        if (bp != NULL) {
            if (!bp->is_broken) {
                fprintf(stdout, "Hit breakpoint at %"PRIu64, state->bm.ip);

                // TODO: hitting breakpoint does not print the label anymore
                if (bp->label.data)
                {
                    fprintf(stdout, " label '"SV_Fmt"'", SV_Arg(bp->label));
                }

                fprintf(stdout, "\n");
                bp->is_broken = 1;

                state->step_over_mode_call_depth = 0;
                state->is_in_step_over_mode = 0;

                return BDB_OK;
            }
        }

        Err err = bm_execute_inst(&state->bm);
        if (err)
        {
            return bdb_fault(state, err);
        }

        if (state->is_in_step_over_mode &&
            state->step_over_mode_call_depth == 0)
        {
            return BDB_OK;
        }

    }
    while (!state->bm.halt);

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

    if (state->bm.halt)
    {
        fprintf(stderr, "ERR : Program is not being run\n");
        return BDB_OK;
    }

    Err err = bm_execute_inst(&state->bm);
    if (!err)
    {
        return BDB_OK;
    }
    else
    {
        return bdb_fault(state, err);
    }
}

// TODO: bdb should be able to break on BASM expression
Bdb_Err bdb_parse_binding_or_value(Bdb_State *st, String_View addr, Word *out)
{
    assert(st);
    assert(out);

    if (addr.count == 0)
    {
        return BDB_FAIL;
    }

    char *endptr = NULL;

    uint64_t value = strtoull(addr.data, &endptr, 10);
    if (endptr != addr.data + addr.count) {
        Bdb_Binding binding = {0};

        if (bdb_resolve_binding(st, addr, &binding) == BDB_FAIL)
        {
            return BDB_FAIL;
        }

        value = binding.value.as_u64;
    }

    if (out) out->as_u64 = value;

    return BDB_OK;
}

void bdb_print_location(Bdb_State *state)
{
    assert(state);

    Inst_Addr ip = state->bm.ip;
    const Bdb_Binding *location = NULL;

    for (size_t i = 0; i < state->bindings_size; ++i) {
        const Bdb_Binding *current = &state->bindings[i];
        if (current->kind == BINDING_LABEL && current->value.as_u64 <= ip) {
            if (location == NULL || location->value.as_u64 > current->value.as_u64) {
                location = current;
            }
        }
    }

    if (location) {
        printf("At address %"PRIu64": "SV_Fmt"+%"PRIu64"\n",
               ip, SV_Arg(location->name), location->value.as_u64);
    } else {
        printf("ip = %"PRIu64"\n"
               "WARN : No location info available\n",
               ip);
    }
}

Bdb_Err bdb_reset(Bdb_State *state)
{
    bm_load_program_from_file(&state->bm, state->program_file_path);
    bm_load_standard_natives(&state->bm);

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
        if (state->breakpoints[i].label.data) {
            Bdb_Binding binding = {0};
            if (bdb_resolve_binding(state, state->breakpoints[i].label, &binding) == BDB_OK) {
                state->breakpoints[i].addr = binding.value.as_u64;
            } else {
                fprintf(stderr, "WARNING: label `"SV_Fmt"` got removed\n",
                        SV_Arg(state->breakpoints[i].label));
                state->breakpoints[i].label = SV_NULL;
            }
        }
    }

    return BDB_OK;
}

Bdb_Err bdb_run_command(Bdb_State *state, String_View command_word, String_View arguments)
{
    switch (*command_word.data)
    {
    /*
     * Next instruction
     */
    case 's':
    {
        Bdb_Err err = bdb_step_instr(state);
        if (err)
        {
            return err;
        }

        printf("-> ");
        bdb_print_instr(state, stdout, &state->bm.program[state->bm.ip]);
        printf("\n");
    } break;
    /*
     * Step over instruction
     */
    case 'n':
    {
        Bdb_Err err = bdb_step_over_instr(state);
        if (err)
        {
            return err;
        }

        printf("-> ");
        bdb_print_instr(state, stdout, &state->bm.program[state->bm.ip]);
        printf("\n");
    } break;
    /*
     * Inspect the memory
     */
    case 'x':
    {
        arguments = sv_trim(arguments);
        String_View where_sv = sv_chop_by_delim(&arguments, ' ');
        String_View count_sv = arguments;

        Word where = word_u64(0);
        if (bdb_parse_binding_or_value(state, where_sv, &where) == BDB_FAIL)
        {
            fprintf(stderr, "ERR : Cannot parse address, label or constant `"SV_Fmt"`\n", SV_Arg(where_sv));
            return BDB_FAIL;
        }

        Word count = word_u64(0);
        if (bdb_parse_binding_or_value(state, count_sv, &count) == BDB_FAIL)
        {
            fprintf(stderr, "ERR : Cannot parse address, label or constant `"SV_Fmt"`\n", SV_Arg(count_sv));
            return BDB_FAIL;
        }

        for (uint64_t i = 0;
             i < count.as_u64 && where.as_u64 + i < BM_MEMORY_CAPACITY;
             ++i)
        {
            printf("%02X ", state->bm.memory[where.as_u64 + i]);
        }
        printf("\n");

    } break;
    /*
     * Dump the stack
     */
    case 'p':
    {
        bm_dump_stack(stdout, &state->bm);
    } break;
    /*
     * Where - print information about the current location in the program
     */
    case 'w':
    {
        bdb_print_location(state);
    } break;
    case 'b':
    {
        Word break_addr;
        String_View addr = sv_trim(arguments);

        if (bdb_parse_binding_or_value(state, addr, &break_addr) == BDB_FAIL)
        {
            fprintf(stderr, "ERR : Cannot parse address or label\n");
            return BDB_FAIL;
        }

        bdb_add_breakpoint(state, break_addr.as_u64);
        fprintf(stdout, "INFO : Breakpoint set at %"PRIu64"\n", break_addr.as_u64);
    } break;
    case 'd':
    {
        Word break_addr;
        String_View addr = sv_trim(arguments);

        if (bdb_parse_binding_or_value(state, addr, &break_addr) == BDB_FAIL)
        {
            fprintf(stderr, "ERR : Cannot parse address or label\n");
            return BDB_FAIL;
        }

        bdb_delete_breakpoint(state, break_addr.as_u64);
        fprintf(stdout, "INFO : Deleted breakpoint at %"PRIu64"\n", break_addr.as_u64);
    } break;
    case 'r':
    {
        if (!state->bm.halt || (state->bm.halt && state->bm.program[state->bm.ip].type == INST_HALT))
        {
            if (state->bm.halt)
            {
                fprintf(stderr,
                        "INFO : Program has halted.\n");
            }
            else
            {
                fprintf(stderr,
                        "INFO : Program is already running.\n");
            }

            char answer;

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
            answer = (char)(getchar());

            switch (answer)
            {
            case 'y':
            case 'Y':
                getchar(); // Consume the '\n'
                return bdb_reset(state);
            case 'n':
            case 'N':
                getchar(); // See above
                /* fall through */
            case '\n':
                return BDB_OK;
            default:
                goto ask_again;
            }
        }

        state->bm.halt = 0;
    } /* fall through */
    case 'c':
    {
        return bdb_continue(state);
    }
    case 'q':
    {
        return BDB_EXIT;
    }
    case 'h':
    {
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
    } break;
    default:
    {
        fprintf(stderr, "?\n");
    } break;
    }

    return BDB_OK;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        usage();
        return EXIT_FAILURE;
    }

    // NOTE: The structure might be quite big due its arena. Better allocate it in the static memory.
    static Bdb_State state = {0};

    printf("BDB - The birtual machine debugger.\n"
           "Type 'h' and enter for a quick help\n");
    if (bdb_state_init(&state, argv[1]) == BDB_FAIL)
    {
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

    while (!feof(stdin))
    {
        printf("(bdb) ");
        char input_buf[INPUT_CAPACITY] = {0};
        fgets(input_buf, INPUT_CAPACITY, stdin);

        String_View
            input_sv = sv_trim(sv_from_cstr(input_buf)),
            control_word = sv_trim(sv_chop_by_delim(&input_sv, ' '));

        /*
         * Check if we need to repeat the previous command
         */
        if (control_word.count == 0)
        {
            if (*previous_command == '\0')
            {
                fprintf(stderr, "ERR : No previous command\n");
                continue;
            }

            input_sv = sv_trim(sv_from_cstr(previous_command));
            control_word = sv_trim(sv_chop_by_delim(&input_sv, ' '));
        }

        Bdb_Err err = bdb_run_command(&state, control_word, input_sv);

        if ( (err == BDB_OK)
             && (*input_buf != '\n') )
        {
            /*
             * Store the previous command for repeating it.
             */
            memcpy(previous_command, input_buf, sizeof(char) * INPUT_CAPACITY);
        }
        else if (err == BDB_EXIT)
        {
            printf("Bye\n");
            return EXIT_SUCCESS;
        }
    }

    arena_free(&state.sym_arena);
    arena_free(&state.break_arena);

    return EXIT_SUCCESS;
}
