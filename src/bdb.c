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
                       const char *executable)
{
    assert(state);
    assert(executable);

    state->cood_file_name = sv_from_cstr(executable);
    bm_load_program_from_file(&state->bm, executable);
    bm_load_standard_natives(&state->bm);

    fprintf(stdout, "INFO : Loading debug symbols...\n");
    return bdb_load_symtab(state, arena_sv_concat2(&state->arena, executable, ".sym"));
}

Bdb_Err bdb_find_addr_of_label(Bdb_State *state, String_View name, Inst_Addr *out)
{
    assert(state);
    assert(out);

    for (Inst_Addr i = 0; i < BM_PROGRAM_CAPACITY; ++i)
    {
        if (state->labels[i].data && sv_eq(state->labels[i], name))
        {
            *out = i;
            return BDB_OK;
        }
    }

    return BDB_FAIL;
}

Bdb_Err bdb_load_symtab(Bdb_State *state, String_View symtab_file)
{
    assert(state);

    String_View symtab = {0};
    if (arena_slurp_file(&state->arena, symtab_file, &symtab) < 0) {
        fprintf(stderr, "ERROR: could not read file "SV_Fmt": %s\n",
                SV_Arg(symtab_file), strerror(errno));
        exit(1);
    }

    while (symtab.count > 0)
    {
        symtab                    = sv_trim_left(symtab);
        String_View  raw_addr     = sv_chop_by_delim(&symtab, '\t');
        symtab                    = sv_trim_left(symtab);
        String_View  raw_sym_type = sv_chop_by_delim(&symtab, '\t');
        symtab                    = sv_trim_left(symtab);
        String_View  label_name   = sv_chop_by_delim(&symtab, '\n');
        Word         value        = word_u64(sv_to_u64(raw_addr));
        Binding_Kind kind         = (Binding_Kind)sv_to_u64(raw_sym_type);

        switch (kind)
        {
        case BINDING_CONST:
        {
            Bdb_BindingConstant *it = &state->constants[state->constants_size++];
            it->name                = label_name;
            it->value               = value;

        } break;
        case BINDING_LABEL:
        {
            /*
             * Huh? you ask? Yes, if we have a label, whose size is bigger
             * than the program size, which is to say, that it is a
             * preprocessor label, then we don't wanna overrun our label
             * storage buffer.
             */
            if (value.as_u64 < BM_PROGRAM_CAPACITY)
            {
                state->labels[value.as_u64] = label_name;
            }

        } break;
        case BINDING_NATIVE:
        {
            state->native_labels[value.as_u64] = label_name;
        } break;
        }


    }

    return BDB_OK;
}

void bdb_print_instr(Bdb_State *state, FILE *f, Inst *i)
{
    fprintf(f, "%s ", inst_name(i->type));
    if (inst_has_operand(i->type))
    {
        if (i->type == INST_NATIVE)
        {
            fprintf(f, SV_Fmt, SV_Arg(state->native_labels[i->operand.as_u64]));
        }
        else if (i->type == INST_CALL || i->type == INST_JMP || i->type == INST_JMP_IF)
        {
            fprintf(f, SV_Fmt, SV_Arg(state->labels[i->operand.as_u64]));
        }
        else
        {
            fprintf(f, "%" PRIu64, i->operand.as_i64);
        }
    }
}

void bdb_add_breakpoint(Bdb_State *state, Inst_Addr addr)
{
    assert(state);

    if (addr > state->bm.program_size)
    {
        fprintf(stderr, "ERR : Symbol out of program\n");
        return;
    }

    if (addr > BM_PROGRAM_CAPACITY)
    {
        fprintf(stderr, "ERR : Symbol out of memory\n");
        return;
    }

    if (state->breakpoints[addr].is_enabled)
    {
        fprintf(stderr, "ERR : Breakpoint already set\n");
        return;
    }

    state->breakpoints[addr].is_enabled = 1;
}

void bdb_delete_breakpoint(Bdb_State *state, Inst_Addr addr)
{
    assert(state);

    if (addr > state->bm.program_size)
    {
        fprintf(stderr, "ERR : Symbol out of program\n");
        return;
    }

    if (addr > BM_PROGRAM_CAPACITY)
    {
        fprintf(stderr, "ERR : Symbol out of memory\n");
        return;
    }

    if (!state->breakpoints[addr].is_enabled)
    {
        fprintf(stderr, "ERR : No such breakpoint\n");
        return;
    }

    state->breakpoints[addr].is_enabled = 0;
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
        Bdb_Breakpoint *bp = &state->breakpoints[state->bm.ip];
        if (!bp->is_broken && bp->is_enabled)
        {
            fprintf(stdout, "Hit breakpoint at %"PRIu64, state->bm.ip);
            if (state->labels[state->bm.ip].data)
            {
                fprintf(stdout, " label '"SV_Fmt"'", SV_Arg(state->labels[state->bm.ip]));
            }

            fprintf(stdout, "\n");
            bp->is_broken = 1;

            return BDB_OK;
        }

        bp->is_broken = 0;

        Err err = bm_execute_inst(&state->bm);
        if (err)
        {
            return bdb_fault(state, err);
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

Bdb_Err bdb_parse_label_or_addr(Bdb_State *st, String_View addr, Inst_Addr *out)
{
    assert(st);
    assert(out);

    if (addr.count == 0)
    {
        return BDB_FAIL;
    }

    char *endptr = NULL;

    *out = strtoull(addr.data, &endptr, 10);
    if (endptr != addr.data + addr.count)
    {
        if (bdb_find_addr_of_label(st, addr, out) == BDB_FAIL)
        {
            return BDB_FAIL;
        }
    }

    return BDB_OK;
}

Bdb_Err bdb_parse_label_addr_or_constant(Bdb_State *st, String_View in, Word *out)
{
    if (bdb_parse_label_or_addr(st, in, &out->as_u64) == BDB_OK)
    {
        return BDB_OK;
    }

    for (size_t i = 0; i < st->constants_size; ++i) {
        if (sv_eq(in, st->constants[i].name))
        {
            *out = st->constants[i].value;
            return BDB_OK;
        }
    }

    return BDB_FAIL;
}

Bdb_Err bdb_run_command(Bdb_State *state, String_View command_word, String_View arguments)
{
    switch (*command_word.data)
    {
    /*
     * Next instruction
     */
    case 'n':
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
     * Print the IP
     */
    case 'i':
    {
        printf("ip = %" PRIu64 "\n", state->bm.ip);
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
        if (bdb_parse_label_addr_or_constant(state, where_sv, &where) == BDB_FAIL)
        {
            fprintf(stderr, "ERR : Cannot parse address, label or constant `"SV_Fmt"`\n", SV_Arg(where_sv));
            return BDB_FAIL;
        }

        Word count = word_u64(0);
        if (bdb_parse_label_addr_or_constant(state, count_sv, &count) == BDB_FAIL)
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
    case 's':
    {
        bm_dump_stack(stdout, &state->bm);
    } break;
    case 'b':
    {
        Inst_Addr break_addr;
        String_View addr = sv_trim(arguments);

        if (bdb_parse_label_or_addr(state, addr, &break_addr) == BDB_FAIL)
        {
            fprintf(stderr, "ERR : Cannot parse address or label\n");
            return BDB_FAIL;
        }

        bdb_add_breakpoint(state, break_addr);
        fprintf(stdout, "INFO : Breakpoint set at %"PRIu64"\n", break_addr);
    } break;
    case 'd':
    {
        Inst_Addr break_addr;
        String_View addr = sv_trim(arguments);

        if (bdb_parse_label_or_addr(state, addr, &break_addr) == BDB_FAIL)
        {
            fprintf(stderr, "ERR : Cannot parse address or label\n");
            return BDB_FAIL;
        }

        bdb_delete_breakpoint(state, break_addr);
        fprintf(stdout, "INFO : Deleted breakpoint at %"PRIu64"\n", break_addr);
    } break;
    case 'r':
    {
        if (!state->bm.halt)
        {
            fprintf(stderr, "ERR : Program is already running\n");
            return BDB_FAIL;
            /* TODO(#88): Reset bm and restart program */
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
               "n - next instruction\n"
               "c - continue program execution\n"
               "s - stack dump\n"
               "i - instruction pointer\n"
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
    state.bm.halt = 1;

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

    return EXIT_SUCCESS;
}
