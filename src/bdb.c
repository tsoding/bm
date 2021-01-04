#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "bdb.h"

#define BM_IMPLEMENTATION
#include "bm.h"

void bdb_state_init(bdb_state *state,
                    const char *executable,
                    const char *symtab)
{
    assert(state);
    assert(executable);
    assert(symtab);

    state->cood_file_name   = cstr_as_sv(executable);
    state->symtab_file_name = cstr_as_sv(executable);
    bm_load_program_from_file(&state->bm, executable);
}

static
void usage(void)
{
    fprintf(stderr,
            "usage: bdb <executable> <debug symbol table>\n");
}

void bdb_print_instr(FILE *f, Inst *i)
{
    fprintf(f, "%s ", inst_name(i->type));
    if (inst_has_operand(i->type))
    {
        fprintf(f, "%" PRIu64, i->operand.as_i64);
    }
}

bdb_status bdb_step_instr(bdb_state *state)
{
    Err err = bm_execute_inst(&state->bm);
    if (!err)
    {
        return BDB_OK;
    }

    fprintf(stderr, "%s at %" PRIu64 " (INSTR: ",
            err_as_cstr(err), state->bm.ip);
    bdb_print_instr(stderr, &state->bm.program[state->bm.ip]);
    fprintf(stderr, ")\n");
    return BDB_FAIL;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        usage();
        return EXIT_FAILURE;
    }

    /*
     * Create the BDB state and initialize it with the file names
     */

    bdb_state state = {0};

    printf("BDB - The birtual machine debugger.\n");
    bdb_state_init(&state, argv[1], argv[2]);

    while (1)
    {
        printf("(bdb) ");
        char input_buf[64];
        fgets(input_buf, 64, stdin);

        switch (*input_buf)
        {
        /*
         * Next instruction
         */
        case 'n':
        {
            bdb_status res = bdb_step_instr(&state);
            if (res == BDB_FAIL)
            {
                return EXIT_FAILURE;
            }

            printf("-> ");
            bdb_print_instr(stdout, &state.bm.program[state.bm.ip]);
            printf("\n");
        } break;
        /*
         * Print the IP
         */
        case 'i':
        {
            printf("ip = %" PRIu64 "\n", state.bm.ip);
        } break;
        /*
         * Dump the stack
         */
        case 's':
        {
            bm_dump_stack(stdout, &state.bm);
        } break;
        case EOF:
        case 'q':
        {
            return EXIT_SUCCESS;
        }
        default:
        {
            fprintf(stderr, "?\n");
        } break;
        }
    }

    return EXIT_SUCCESS;
}
