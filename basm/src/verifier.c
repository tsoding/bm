#include "./verifier.h"

bool verifier_push_frame(Verifier *verifier, Type type, File_Location location)
{
    if (verifier->stack_size < BM_STACK_CAPACITY) {
        verifier->stack[verifier->stack_size].type = type;
        verifier->stack[verifier->stack_size].location = location;
        verifier->stack_size += 1;
        return true;
    }

    return false;
}

bool verifier_pop_frame(Verifier *verifier, Frame *output)
{
    if (verifier->stack_size > 0) {
        Frame frame = verifier->stack[--verifier->stack_size];
        if (output) {
            *output = frame;
        }
        return true;
    }

    return false;
}

// TODO(#357): unreachable code elimination

void verifier_verify(Verifier *verifier, const Basm *basm)
{
    if (basm->entry >= BM_PROGRAM_CAPACITY) {
        fprintf(stderr, FL_Fmt": ERROR: entry point is an illegal instruction address\n",
                FL_Arg(basm->entry_location));
        exit(1);
    }

    Inst_Addr ip = basm->entry;
    bool halt = false;

    while (!halt) {
        Inst_Def def = get_inst_def(basm->program[ip].type);

        assert(ip < BM_PROGRAM_CAPACITY);

        switch (basm->program[ip].type) {
        case INST_HALT: {
            halt = true;
        }
        break;

        case INST_NOP: {
            // Nothing to verify
            ip += 1;
        }
        break;

        case INST_PUSH: {
            // TODO(#358): customizable stack size that is communicated between bme and basm somehow
            if (!verifier_push_frame(verifier, basm->program_operand_types[ip], basm->program_locations[ip])) {
                fprintf(stderr, FL_Fmt": ERROR: stack overflow\n",
                        FL_Arg(basm->program_locations[ip]));
                exit(1);
            }
            ip += 1;
        }
        break;

        case INST_DROP: {
            if (!verifier_pop_frame(verifier, NULL)) {
                fprintf(stderr, FL_Fmt": ERROR: stack underflow\n",
                        FL_Arg(basm->program_locations[ip]));
                exit(1);
            }
            ip += 1;
        }
        break;

        case INST_DUP: {
            assert(basm->program_operand_types[ip] == TYPE_UNSIGNED_INT);
            Stack_Addr stack_addr = basm->program[ip].operand.as_u64;
            if (stack_addr < verifier->stack_size) {
                Frame frame = verifier->stack[verifier->stack_size - 1 - stack_addr];
                if (!verifier_push_frame(verifier, frame.type, frame.location)) {
                    fprintf(stderr, FL_Fmt": ERROR: stack overflow\n",
                            FL_Arg(basm->program_locations[ip]));
                    exit(1);
                }
            } else {
                fprintf(stderr, FL_Fmt": ERROR: stack underflow\n",
                        FL_Arg(basm->program_locations[ip]));
                exit(1);
            }

            ip += 1;
        }
        break;

        case INST_SWAP: {
            assert(basm->program_operand_types[ip] == TYPE_UNSIGNED_INT);
            Stack_Addr stack_addr = basm->program[ip].operand.as_u64;
            if (stack_addr < verifier->stack_size) {
                Frame frame = verifier->stack[verifier->stack_size - 1 - stack_addr];
                verifier->stack[verifier->stack_size - 1 - stack_addr] = verifier->stack[verifier->stack_size - 1];
                verifier->stack[verifier->stack_size - 1] = frame;
            } else {
                fprintf(stderr, FL_Fmt": ERROR: stack underflow\n",
                        FL_Arg(basm->program_locations[ip]));
                exit(1);
            }

            ip += 1;
        }
        break;

        case INST_PLUSI:
        case INST_MINUSI:
        case INST_MULTI:
        case INST_DIVI:
        case INST_MODI:
        case INST_MULTU:
        case INST_DIVU:
        case INST_MODU:
        case INST_PLUSF:
        case INST_MINUSF:
        case INST_MULTF:
        case INST_DIVF:
        case INST_NOT:
        case INST_EQI:
        case INST_GEI:
        case INST_GTI:
        case INST_LEI:
        case INST_LTI:
        case INST_NEI:
        case INST_EQU:
        case INST_GEU:
        case INST_GTU:
        case INST_LEU:
        case INST_LTU:
        case INST_NEU:
        case INST_EQF:
        case INST_GEF:
        case INST_GTF:
        case INST_LEF:
        case INST_LTF:
        case INST_NEF:
        case INST_ANDB:
        case INST_ORB:
        case INST_XOR:
        case INST_SHR:
        case INST_SHL:
        case INST_NOTB:
        case INST_READ8U:
        case INST_READ16U:
        case INST_READ32U:
        case INST_READ64U:
        case INST_READ8I:
        case INST_READ16I:
        case INST_READ32I:
        case INST_READ64I:
        case INST_WRITE8:
        case INST_WRITE16:
        case INST_WRITE32:
        case INST_WRITE64:
        case INST_I2F:
        case INST_U2F:
        case INST_F2I:
        case INST_F2U: {
            for (size_t i = def.input.size; i > 0; --i) {
                Frame frame = {0};
                if (!verifier_pop_frame(verifier, &frame)) {
                    fprintf(stderr, FL_Fmt": ERROR: stack underflow\n",
                            FL_Arg(basm->program_locations[ip]));
                    exit(1);
                }

                Type expected_type = def.input.types[i - 1];
                if (frame.type != expected_type) {
                    fprintf(stderr, FL_Fmt": ERROR: TYPE CHECK ERROR! Instruction `%s` expected `%s`, but found `%s`\n",
                            FL_Arg(basm->program_locations[ip]),
                            def.name,
                            type_name(expected_type),
                            type_name(frame.type));
                    fprintf(stderr, FL_Fmt": NOTE: the argument was introduced here\n",
                            FL_Arg(frame.location));
                    exit(1);
                }
            }

            for (size_t i = 0; i < def.output.size; ++i) {
                if (!verifier_push_frame(verifier, def.output.types[i], basm->program_locations[ip])) {
                    fprintf(stderr, FL_Fmt": ERROR: stack overflow\n",
                            FL_Arg(basm->program_locations[ip]));
                    exit(1);
                }
            }

            ip += 1;
        }
        break;

        case INST_JMP:
        case INST_JMP_IF:
        case INST_RET:
        case INST_CALL:
        // TODO(#360): verification for control-flow instructions is not implemented
        case INST_NATIVE:
            // TODO(#361): verification for native calls is not implemented
            fprintf(stderr, FL_Fmt": ERROR: verification for instruction `%s` is not implemented yet\n", FL_Arg(basm->program_locations[ip]), def.name);
            exit(1);
            break;
        case NUMBER_OF_INSTS:
        default:
            assert(0 && "basm_verify_program: unreachable");
            exit(1);
        }
    }
}
