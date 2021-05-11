#include "./verifier.h"

bool verifier_push_type(Verifier *verifier, Type type)
{
    if (verifier->types_size < BM_STACK_CAPACITY) {
        verifier->types[verifier->types_size++] = type;
        return true;
    }

    return false;
}

bool verifier_pop_type(Verifier *verifier, Type *output)
{
    if (verifier->types_size > 0) {
        Type frame = verifier->types[--verifier->types_size];
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
    Inst_Addr ip = basm->entry;
    bool halt = false;

    while (!halt) {
        Inst_Def def = get_inst_def(basm->program[ip].type);

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
            if (!verifier_push_type(verifier, basm->program_operand_types[ip])) {
                fprintf(stderr, FL_Fmt": ERROR: stack overflow\n",
                        FL_Arg(basm->program_locations[ip]));
                exit(1);
            }
            ip += 1;
        }
        break;

        case INST_DROP: {
            if (!verifier_pop_type(verifier, NULL)) {
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
            if (stack_addr < verifier->types_size) {
                Type type = verifier->types[verifier->types_size - 1 - stack_addr];
                if (!verifier_push_type(verifier, type)) {
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
            if (stack_addr < verifier->types_size) {
                Type type = verifier->types[verifier->types_size - 1 - stack_addr];
                verifier->types[verifier->types_size - 1 - stack_addr] = verifier->types[verifier->types_size - 1];
                verifier->types[verifier->types_size - 1] = type;
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
                Type actual_type = 0;
                if (!verifier_pop_type(verifier, &actual_type)) {
                    fprintf(stderr, FL_Fmt": ERROR: stack underflow\n",
                            FL_Arg(basm->program_locations[ip]));
                    exit(1);
                }

                Type expected_type = def.input.types[i - 1];
                if (actual_type != expected_type) {
                    // TODO(#359): type check error in the basm_verify_program() should report the argument number as well
                    fprintf(stderr, FL_Fmt": ERROR: TYPE CHECK ERROR! Instruction `%s` expected `%s`, but found `%s`\n",
                            FL_Arg(basm->program_locations[ip]),
                            def.name,
                            type_name(expected_type),
                            type_name(actual_type));
                    exit(1);
                }
            }

            for (size_t i = 0; i < def.output.size; ++i) {
                if (!verifier_push_type(verifier, def.output.types[i])) {
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
