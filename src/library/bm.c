#include "./bm.h"

static Inst_Def inst_defs[NUMBER_OF_INSTS] = {
    [INST_NOP]     = {.type = INST_NOP,     .name = "nop",     .has_operand = false},
    [INST_PUSH]    = {
        .type = INST_PUSH,    .name = "push",    .has_operand = true,
        .operand_type = TYPE_ANY
    },
    [INST_DROP]    = {
        .type = INST_DROP,    .name = "drop",    .has_operand = false,
        .input = TYPE_LIST(TYPE_ANY)
    },
    // TODO(#346): `dup` and `swap` should accept operands of type TYPE_STACK_ADDR
    // But it's difficult to enforce right now because of the legacy code base and other semantic problems with the language
    [INST_DUP]     = {
        .type = INST_DUP,     .name = "dup",     .has_operand = true,
        .operand_type = TYPE_UNSIGNED_INT
    },
    [INST_SWAP]    = {
        .type = INST_SWAP,    .name = "swap",    .has_operand = true,
        .operand_type = TYPE_UNSIGNED_INT
    },
    // TODO(#355): PLUSI and MINUSI both accept Signed and Unsigned Ints but our type system is incapable of handling that right now
    [INST_PLUSI]   = {
        .type = INST_PLUSI,   .name = "plusi",   .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_UNSIGNED_INT)
    },
    [INST_MINUSI]  = {
        .type = INST_MINUSI,  .name = "minusi",  .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_UNSIGNED_INT)
    },
    [INST_MULTI]   = {
        .type = INST_MULTI,   .name = "multi",   .has_operand = false,
        .input = TYPE_LIST(TYPE_SIGNED_INT, TYPE_SIGNED_INT),
        .output = TYPE_LIST(TYPE_SIGNED_INT)
    },
    [INST_DIVI]    = {
        .type = INST_DIVI,    .name = "divi",    .has_operand = false,
        .input = TYPE_LIST(TYPE_SIGNED_INT, TYPE_SIGNED_INT),
        .output = TYPE_LIST(TYPE_SIGNED_INT)
    },
    [INST_MODI]    = {
        .type = INST_MODI,    .name = "modi",    .has_operand = false,
        .input = TYPE_LIST(TYPE_SIGNED_INT, TYPE_SIGNED_INT),
        .output = TYPE_LIST(TYPE_SIGNED_INT)
    },
    [INST_MULTU]   = {
        .type = INST_MULTU,   .name = "multu",   .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_UNSIGNED_INT)
    },
    [INST_DIVU]    = {
        .type = INST_DIVU,    .name = "divu",    .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_UNSIGNED_INT)
    },
    [INST_MODU]    = {
        .type = INST_MODU,    .name = "modu",    .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_UNSIGNED_INT)
    },
    [INST_PLUSF]   = {
        .type = INST_PLUSF,   .name = "plusf",   .has_operand = false,
        .input = TYPE_LIST(TYPE_FLOAT, TYPE_FLOAT),
        .output = TYPE_LIST(TYPE_FLOAT)
    },
    [INST_MINUSF]  = {
        .type = INST_MINUSF,  .name = "minusf",  .has_operand = false,
        .input = TYPE_LIST(TYPE_FLOAT, TYPE_FLOAT),
        .output = TYPE_LIST(TYPE_FLOAT)
    },
    [INST_MULTF]   = {
        .type = INST_MULTF,   .name = "multf",   .has_operand = false,
        .input = TYPE_LIST(TYPE_FLOAT, TYPE_FLOAT),
        .output = TYPE_LIST(TYPE_FLOAT)
    },
    [INST_DIVF]    = {
        .type = INST_DIVF,    .name = "divf",    .has_operand = false,
        .input = TYPE_LIST(TYPE_FLOAT, TYPE_FLOAT),
        .output = TYPE_LIST(TYPE_FLOAT)
    },
    [INST_JMP]     = {
        .type = INST_JMP,     .name = "jmp",     .has_operand = true,
        .operand_type = TYPE_INST_ADDR
    },
    [INST_JMP_IF]  = {
        .type = INST_JMP_IF,  .name = "jmp_if",  .has_operand = true,
        .operand_type = TYPE_INST_ADDR
    },
    [INST_RET]     = {
        .type = INST_RET,     .name = "ret",     .has_operand = false,
        .input = TYPE_LIST(TYPE_INST_ADDR)
    },
    [INST_CALL]    = {
        .type = INST_CALL,    .name = "call",    .has_operand = true,
        .operand_type = TYPE_INST_ADDR,
        .output = TYPE_LIST(TYPE_INST_ADDR)
    },
    [INST_NATIVE]  = {
        .type = INST_NATIVE,  .name = "native",  .has_operand = true,
        .operand_type = TYPE_NATIVE_ID
    },
    [INST_HALT]    = {.type = INST_HALT,    .name = "halt",    .has_operand = false},
    // TODO(#356): introduce booleans to the type system
    // The comparison operations should return them instead of Unsigned_Int-s
    [INST_NOT]     = {
        .type = INST_NOT,     .name = "not",     .has_operand = false,
        .input = TYPE_LIST(TYPE_BOOL),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_EQI]     = {
        .type = INST_EQI,     .name = "eqi",     .has_operand = false,
        .input = TYPE_LIST(TYPE_SIGNED_INT, TYPE_SIGNED_INT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_GEI]     = {
        .type = INST_GEI,     .name = "gei",     .has_operand = false,
        .input = TYPE_LIST(TYPE_SIGNED_INT, TYPE_SIGNED_INT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_GTI]     = {
        .type = INST_GTI,     .name = "gti",     .has_operand = false,
        .input = TYPE_LIST(TYPE_SIGNED_INT, TYPE_SIGNED_INT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_LEI]     = {
        .type = INST_LEI,     .name = "lei",     .has_operand = false,
        .input = TYPE_LIST(TYPE_SIGNED_INT, TYPE_SIGNED_INT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_LTI]     = {
        .type = INST_LTI,     .name = "lti",     .has_operand = false,
        .input = TYPE_LIST(TYPE_SIGNED_INT, TYPE_SIGNED_INT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_NEI]     = {
        .type = INST_NEI,     .name = "nei",     .has_operand = false,
        .input = TYPE_LIST(TYPE_SIGNED_INT, TYPE_SIGNED_INT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_EQU]     = {
        .type = INST_EQU,     .name = "equ",     .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_GEU]     = {
        .type = INST_GEU,     .name = "geu",     .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_GTU]     = {
        .type = INST_GTU,     .name = "gtu",     .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_LEU]     = {
        .type = INST_LEU,     .name = "leu",     .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_LTU]     = {
        .type = INST_LTU,     .name = "ltu",     .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_NEU]     = {
        .type = INST_NEU,     .name = "neu",     .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_EQF]     = {
        .type = INST_EQF,     .name = "eqf",     .has_operand = false,
        .input = TYPE_LIST(TYPE_FLOAT, TYPE_FLOAT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_GEF]     = {
        .type = INST_GEF,     .name = "gef",     .has_operand = false,
        .input = TYPE_LIST(TYPE_FLOAT, TYPE_FLOAT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_GTF]     = {
        .type = INST_GTF,     .name = "gtf",     .has_operand = false,
        .input = TYPE_LIST(TYPE_FLOAT, TYPE_FLOAT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_LEF]     = {
        .type = INST_LEF,     .name = "lef",     .has_operand = false,
        .input = TYPE_LIST(TYPE_FLOAT, TYPE_FLOAT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_LTF]     = {
        .type = INST_LTF,     .name = "ltf",     .has_operand = false,
        .input = TYPE_LIST(TYPE_FLOAT, TYPE_FLOAT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_NEF]     = {
        .type = INST_NEF,     .name = "nef",     .has_operand = false,
        .input = TYPE_LIST(TYPE_FLOAT, TYPE_FLOAT),
        .output = TYPE_LIST(TYPE_BOOL)
    },
    [INST_ANDB]    = {
        .type = INST_ANDB,    .name = "andb",    .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_UNSIGNED_INT)
    },
    [INST_ORB]     = {
        .type = INST_ORB,     .name = "orb",     .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_UNSIGNED_INT)
    },
    [INST_XOR]     = {
        .type = INST_XOR,     .name = "xor",     .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_UNSIGNED_INT)
    },
    [INST_SHR]     = {
        .type = INST_SHR,     .name = "shr",     .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_UNSIGNED_INT)
    },
    [INST_SHL]     = {
        .type = INST_SHL,     .name = "shl",     .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_UNSIGNED_INT)
    },
    [INST_NOTB]    = {
        .type = INST_NOTB,    .name = "notb",    .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_UNSIGNED_INT)
    },
    [INST_READ8U]  = {
        .type = INST_READ8U,  .name = "read8u",  .has_operand = false,
        .input = TYPE_LIST(TYPE_MEM_ADDR),
        .output = TYPE_LIST(TYPE_UNSIGNED_INT)
    },
    [INST_READ16U] = {
        .type = INST_READ16U, .name = "read16u", .has_operand = false,
        .input = TYPE_LIST(TYPE_MEM_ADDR),
        .output = TYPE_LIST(TYPE_UNSIGNED_INT)
    },
    [INST_READ32U] = {
        .type = INST_READ32U, .name = "read32u", .has_operand = false,
        .input = TYPE_LIST(TYPE_MEM_ADDR),
        .output = TYPE_LIST(TYPE_UNSIGNED_INT)
    },
    [INST_READ64U] = {
        .type = INST_READ64U, .name = "read64u", .has_operand = false,
        .input = TYPE_LIST(TYPE_MEM_ADDR),
        .output = TYPE_LIST(TYPE_UNSIGNED_INT)
    },
    [INST_READ8I]  = {
        .type = INST_READ8I,  .name = "read8i",  .has_operand = false,
        .input = TYPE_LIST(TYPE_MEM_ADDR),
        .output = TYPE_LIST(TYPE_SIGNED_INT)
    },
    [INST_READ16I] = {
        .type = INST_READ16I, .name = "read16i", .has_operand = false,
        .input = TYPE_LIST(TYPE_MEM_ADDR),
        .output = TYPE_LIST(TYPE_SIGNED_INT)
    },
    [INST_READ32I] = {
        .type = INST_READ32I, .name = "read32i", .has_operand = false,
        .input = TYPE_LIST(TYPE_MEM_ADDR),
        .output = TYPE_LIST(TYPE_SIGNED_INT)
    },
    [INST_READ64I] = {
        .type = INST_READ64I, .name = "read64i", .has_operand = false,
        .input = TYPE_LIST(TYPE_MEM_ADDR),
        .output = TYPE_LIST(TYPE_SIGNED_INT)
    },
    [INST_WRITE8]  = {
        .type = INST_WRITE8,  .name = "write8",  .has_operand = false,
        .input = TYPE_LIST(TYPE_MEM_ADDR, TYPE_UNSIGNED_INT)
    },
    [INST_WRITE16] = {
        .type = INST_WRITE16, .name = "write16", .has_operand = false,
        .input = TYPE_LIST(TYPE_MEM_ADDR, TYPE_UNSIGNED_INT)
    },
    [INST_WRITE32] = {
        .type = INST_WRITE32, .name = "write32", .has_operand = false,
        .input = TYPE_LIST(TYPE_MEM_ADDR, TYPE_UNSIGNED_INT)
    },
    [INST_WRITE64] = {
        .type = INST_WRITE64, .name = "write64", .has_operand = false,
        .input = TYPE_LIST(TYPE_MEM_ADDR, TYPE_UNSIGNED_INT)
    },
    [INST_I2F]     = {
        .type = INST_I2F,     .name = "i2f",     .has_operand = false,
        .input = TYPE_LIST(TYPE_SIGNED_INT),
        .output = TYPE_LIST(TYPE_FLOAT)
    },
    [INST_U2F]     = {
        .type = INST_U2F,     .name = "u2f",     .has_operand = false,
        .input = TYPE_LIST(TYPE_UNSIGNED_INT),
        .output = TYPE_LIST(TYPE_FLOAT)
    },
    [INST_F2I]     = {
        .type = INST_F2I,     .name = "f2i",     .has_operand = false,
        .input = TYPE_LIST(TYPE_FLOAT),
        .output = TYPE_LIST(TYPE_SIGNED_INT)
    },
    [INST_F2U]     = {
        .type = INST_F2U,     .name = "f2u",     .has_operand = false,
        .input = TYPE_LIST(TYPE_FLOAT),
        .output = TYPE_LIST(TYPE_UNSIGNED_INT)
    },
};
static_assert(
    NUMBER_OF_INSTS == 64,
    "You probably added or removed an instruction. "
    "Please update the definitions above accordingly");

bool inst_by_name(String_View name, Inst_Def *inst_def)
{
    for (Inst_Type type = 0; type < NUMBER_OF_INSTS; type += 1) {
        if (sv_eq(sv_from_cstr(inst_defs[type].name), name)) {
            *inst_def = inst_defs[type];
            return true;
        }
    }

    return false;
}

Inst_Def get_inst_def(Inst_Type type)
{
    assert(type < NUMBER_OF_INSTS);
    return inst_defs[type];
}

const char *err_as_cstr(Err err)
{
    switch (err) {
    case ERR_OK:
        return "ERR_OK";
    case ERR_STACK_OVERFLOW:
        return "ERR_STACK_OVERFLOW";
    case ERR_STACK_UNDERFLOW:
        return "ERR_STACK_UNDERFLOW";
    case ERR_ILLEGAL_INST:
        return "ERR_ILLEGAL_INST";
    case ERR_ILLEGAL_OPERAND:
        return "ERR_ILLEGAL_OPERAND";
    case ERR_ILLEGAL_INST_ACCESS:
        return "ERR_ILLEGAL_INST_ACCESS";
    case ERR_DIV_BY_ZERO:
        return "ERR_DIV_BY_ZERO";
    case ERR_ILLEGAL_MEMORY_ACCESS:
        return "ERR_ILLEGAL_MEMORY_ACCESS";
    case ERR_NULL_NATIVE:
        return "ERR_NULL_NATIVE";
    default:
        assert(false && "err_as_cstr: Unreachable");
        exit(1);
    }
}

Err bm_execute_program(Bm *bm, int limit)
{
    while (limit != 0 && !bm->halt) {
        Err err = bm_execute_inst(bm);
        if (err != ERR_OK) {
            return err;
        }
        if (limit > 0) {
            --limit;
        }
    }

    return ERR_OK;
}

#define READ_OP(bm, type, out) \
    do { \
        if ((bm)->stack_size < 1) { \
            return ERR_STACK_UNDERFLOW; \
        } \
        const Memory_Addr addr = (bm)->stack[(bm)->stack_size - 1].as_u64; \
        if (addr >= BM_MEMORY_CAPACITY) { \
            return ERR_ILLEGAL_MEMORY_ACCESS; \
        } \
        /* note: since we are relying on some integer widening conversions here, */ \
        /* we must create a temporary variable so the compiler can do the implicit cast. */ \
        type tmp; \
        memcpy(&tmp, &(bm)->memory[addr], sizeof(type)); \
        (bm)->stack[(bm)->stack_size - 1].as_##out = tmp; \
        (bm)->ip += 1; \
    } while(false)

#define BINARY_OP(bm, in, out, op)                                      \
    do {                                                                \
        if ((bm)->stack_size < 2) {                                     \
            return ERR_STACK_UNDERFLOW;                                 \
        }                                                               \
                                                                        \
        (bm)->stack[(bm)->stack_size - 2].as_##out = (bm)->stack[(bm)->stack_size - 2].as_##in op (bm)->stack[(bm)->stack_size - 1].as_##in; \
        (bm)->stack_size -= 1;                                          \
        (bm)->ip += 1;                                                  \
    } while (false)

#define CAST_OP(bm, src, dst, cast)             \
    do {                                        \
        if ((bm)->stack_size < 1) {             \
            return ERR_STACK_UNDERFLOW;         \
        }                                                               \
                                                                        \
        (bm)->stack[(bm)->stack_size - 1].as_##dst = cast (bm)->stack[(bm)->stack_size - 1].as_##src; \
                                                                        \
        (bm)->ip += 1;                                                  \
    } while (false)


Err bm_execute_inst(Bm *bm)
{
    if (bm->ip >= bm->program_size) {
        return ERR_ILLEGAL_INST_ACCESS;
    }

    Inst inst = bm->program[bm->ip];

    switch (inst.type) {
    case INST_NOP:
        bm->ip += 1;
        break;

    case INST_PUSH:
        if (bm->stack_size >= BM_STACK_CAPACITY) {
            return ERR_STACK_OVERFLOW;
        }
        bm->stack[bm->stack_size++] = inst.operand;
        bm->ip += 1;
        break;

    case INST_DROP:
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_PLUSI:
        BINARY_OP(bm, u64, u64, +);
        break;

    case INST_MINUSI:
        BINARY_OP(bm, u64, u64, -);
        break;

    case INST_MULTI:
        BINARY_OP(bm, i64, i64, *);
        break;

    case INST_MULTU:
        BINARY_OP(bm, u64, u64, *);
        break;

    case INST_DIVI: {
        if (bm->stack[bm->stack_size - 1].as_i64 == 0) {
            return ERR_DIV_BY_ZERO;
        }
        BINARY_OP(bm, i64, i64, /);
    }
    break;

    case INST_DIVU: {
        if (bm->stack[bm->stack_size - 1].as_u64 == 0) {
            return ERR_DIV_BY_ZERO;
        }
        BINARY_OP(bm, u64, u64, /);
    }
    break;

    case INST_MODI: {
        if (bm->stack[bm->stack_size - 1].as_i64 == 0) {
            return ERR_DIV_BY_ZERO;
        }
        BINARY_OP(bm, i64, i64, %);
    }
    break;

    case INST_MODU: {
        if (bm->stack[bm->stack_size - 1].as_u64 == 0) {
            return ERR_DIV_BY_ZERO;
        }
        BINARY_OP(bm, u64, u64, %);
    }
    break;

    case INST_PLUSF:
        BINARY_OP(bm, f64, f64, +);
        break;

    case INST_MINUSF:
        BINARY_OP(bm, f64, f64, -);
        break;

    case INST_MULTF:
        BINARY_OP(bm, f64, f64, *);
        break;

    case INST_DIVF:
        BINARY_OP(bm, f64, f64, /);
        break;

    case INST_JMP:
        bm->ip = inst.operand.as_u64;
        break;

    case INST_RET:
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->ip = bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 1;
        break;

    case INST_CALL:
        if (bm->stack_size >= BM_STACK_CAPACITY) {
            return ERR_STACK_OVERFLOW;
        }

        bm->stack[bm->stack_size++].as_u64 = bm->ip + 1;
        bm->ip = inst.operand.as_u64;
        break;

    case INST_NATIVE:
        if (inst.operand.as_u64 > bm->natives_size) {
            return ERR_ILLEGAL_OPERAND;
        }

        if (!bm->natives[inst.operand.as_u64]) {
            return ERR_NULL_NATIVE;
        }

        const Err err = bm->natives[inst.operand.as_u64](bm);
        if (err != ERR_OK) {
            return err;
        }
        bm->ip += 1;
        break;

    case INST_HALT:
        bm->halt = 1;
        break;

    case INST_EQF:
        BINARY_OP(bm, f64, u64, ==);
        break;

    case INST_GEF:
        BINARY_OP(bm, f64, u64, >=);
        break;

    case INST_GTF:
        BINARY_OP(bm, f64, u64, >);
        break;

    case INST_LEF:
        BINARY_OP(bm, f64, u64, <=);
        break;

    case INST_LTF:
        BINARY_OP(bm, f64, u64, <);
        break;

    case INST_NEF:
        BINARY_OP(bm, f64, u64, !=);
        break;

    case INST_EQI:
        BINARY_OP(bm, i64, u64, ==);
        break;

    case INST_GEI:
        BINARY_OP(bm, i64, u64, >=);
        break;

    case INST_GTI:
        BINARY_OP(bm, i64, u64, >);
        break;

    case INST_LEI:
        BINARY_OP(bm, i64, u64, <=);
        break;

    case INST_LTI:
        BINARY_OP(bm, i64, u64, <);
        break;

    case INST_NEI:
        BINARY_OP(bm, i64, u64, !=);
        break;

    case INST_EQU:
        BINARY_OP(bm, u64, u64, ==);
        break;

    case INST_GEU:
        BINARY_OP(bm, u64, u64, >=);
        break;

    case INST_GTU:
        BINARY_OP(bm, u64, u64, >);
        break;

    case INST_LEU:
        BINARY_OP(bm, u64, u64, <=);
        break;

    case INST_LTU:
        BINARY_OP(bm, u64, u64, <);
        break;

    case INST_NEU:
        BINARY_OP(bm, u64, u64, !=);
        break;

    case INST_JMP_IF:
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }

        if (bm->stack[bm->stack_size - 1].as_u64) {
            bm->ip = inst.operand.as_u64;
        } else {
            bm->ip += 1;
        }

        bm->stack_size -= 1;
        break;

    case INST_DUP:
        if (bm->stack_size >= BM_STACK_CAPACITY) {
            return ERR_STACK_OVERFLOW;
        }

        if (bm->stack_size - inst.operand.as_u64 <= 0) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size] = bm->stack[bm->stack_size - 1 - inst.operand.as_u64];
        bm->stack_size += 1;
        bm->ip += 1;
        break;

    case INST_SWAP:
        if (inst.operand.as_u64 >= bm->stack_size) {
            return ERR_STACK_UNDERFLOW;
        }

        const uint64_t a = bm->stack_size - 1;
        const uint64_t b = bm->stack_size - 1 - inst.operand.as_u64;

        Word t = bm->stack[a];
        bm->stack[a] = bm->stack[b];
        bm->stack[b] = t;
        bm->ip += 1;
        break;

    case INST_NOT:
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size - 1].as_u64 = !bm->stack[bm->stack_size - 1].as_u64;
        bm->ip += 1;
        break;

    case INST_ANDB:
        BINARY_OP(bm, u64, u64, &);
        break;

    case INST_ORB:
        BINARY_OP(bm, u64, u64, |);
        break;

    case INST_XOR:
        BINARY_OP(bm, u64, u64, ^);
        break;

    case INST_SHR:
        BINARY_OP(bm, u64, u64, >>);
        break;

    case INST_SHL:
        BINARY_OP(bm, u64, u64, <<);
        break;

    case INST_NOTB:
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size - 1].as_u64 = ~bm->stack[bm->stack_size - 1].as_u64;
        bm->ip += 1;
        break;

    case INST_READ8U:
        READ_OP(bm, uint8_t, u64);
        break;

    case INST_READ16U:
        READ_OP(bm, uint16_t, u64);
        break;

    case INST_READ32U:
        READ_OP(bm, uint32_t, u64);
        break;

    case INST_READ64U:
        READ_OP(bm, uint64_t, u64);
        break;

    case INST_READ8I:
        READ_OP(bm, int8_t, i64);
        break;

    case INST_READ16I:
        READ_OP(bm, int16_t, i64);
        break;

    case INST_READ32I:
        READ_OP(bm, int32_t, i64);
        break;

    case INST_READ64I:
        READ_OP(bm, int64_t, i64);
        break;

    case INST_WRITE8: {
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        const Memory_Addr addr = bm->stack[bm->stack_size - 2].as_u64;
        if (addr >= BM_MEMORY_CAPACITY) {
            return ERR_ILLEGAL_MEMORY_ACCESS;
        }
        bm->memory[addr] = (uint8_t) bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 2;
        bm->ip += 1;
    }
    break;

    case INST_WRITE16: {
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        const Memory_Addr addr = bm->stack[bm->stack_size - 2].as_u64;
        if (addr >= BM_MEMORY_CAPACITY - 1) {
            return ERR_ILLEGAL_MEMORY_ACCESS;
        }
        uint16_t value = (uint16_t) bm->stack[bm->stack_size - 1].as_u64;
        memcpy(&bm->memory[addr], &value, sizeof(value));
        bm->stack_size -= 2;
        bm->ip += 1;
    }
    break;

    case INST_WRITE32: {
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        const Memory_Addr addr = bm->stack[bm->stack_size - 2].as_u64;
        if (addr >= BM_MEMORY_CAPACITY - 3) {
            return ERR_ILLEGAL_MEMORY_ACCESS;
        }
        uint32_t value = (uint32_t) bm->stack[bm->stack_size - 1].as_u64;
        memcpy(&bm->memory[addr], &value, sizeof(value));
        bm->stack_size -= 2;
        bm->ip += 1;
    }
    break;

    case INST_WRITE64: {
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        const Memory_Addr addr = bm->stack[bm->stack_size - 2].as_u64;
        if (addr >= BM_MEMORY_CAPACITY - 7) {
            return ERR_ILLEGAL_MEMORY_ACCESS;
        }
        uint64_t value = bm->stack[bm->stack_size - 1].as_u64;
        memcpy(&bm->memory[addr], &value, sizeof(value));
        bm->stack_size -= 2;
        bm->ip += 1;
    }
    break;

    case INST_I2F:
        CAST_OP(bm, i64, f64, (double));
        break;

    case INST_U2F:
        CAST_OP(bm, u64, f64, (double));
        break;

    case INST_F2I:
        CAST_OP(bm, f64, i64, (int64_t));
        break;

    case INST_F2U:
        CAST_OP(bm, f64, u64, (uint64_t) (int64_t));
        break;

    case NUMBER_OF_INSTS:
    default:
        return ERR_ILLEGAL_INST;
    }

    return ERR_OK;
}

void bm_push_native(Bm *bm, Bm_Native native)
{
    assert(bm->natives_size < BM_NATIVES_CAPACITY);
    bm->natives[bm->natives_size++] = native;
}

void bm_dump_stack(FILE *stream, const Bm *bm)
{
    fprintf(stream, "Stack:\n");
    if (bm->stack_size > 0) {
        for (Inst_Addr i = 0; i < bm->stack_size; ++i) {
            fprintf(stream, "  u64: %" PRIu64 ", i64: %" PRId64 ", f64: %lf, ptr: %p\n",
                    bm->stack[i].as_u64,
                    bm->stack[i].as_i64,
                    bm->stack[i].as_f64,
                    bm->stack[i].as_ptr);
        }
    } else {
        fprintf(stream, "  [empty]\n");
    }
}

void bm_load_program_from_file(Bm *bm, const char *file_path)
{
    memset(bm, 0, sizeof(*bm));

    FILE *f = fopen(file_path, "rb");
    if (f == NULL) {
        fprintf(stderr, "ERROR: Could not open file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    Bm_File_Meta meta = {0};

    size_t n = fread(&meta, sizeof(meta), 1, f);
    if (n < 1) {
        fprintf(stderr, "ERROR: Could not read meta data from file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    if (meta.magic != BM_FILE_MAGIC) {
        fprintf(stderr,
                "ERROR: %s does not appear to be a valid BM file. "
                "Unexpected magic %04X. Expected %04X.\n",
                file_path,
                meta.magic, BM_FILE_MAGIC);
        exit(1);
    }

    if (meta.version != BM_FILE_VERSION) {
        fprintf(stderr,
                "ERROR: %s: unsupported version of BM file %d. Expected version %d.\n",
                file_path,
                meta.version, BM_FILE_VERSION);
        exit(1);
    }

    if (meta.program_size > BM_PROGRAM_CAPACITY) {
        fprintf(stderr,
                "ERROR: %s: program section is too big. The file contains %" PRIu64 " program instruction. But the capacity is %"  PRIu64 "\n",
                file_path,
                meta.program_size,
                (uint64_t) BM_PROGRAM_CAPACITY);
        exit(1);
    }

    bm->ip = meta.entry;

    if (meta.memory_capacity > BM_MEMORY_CAPACITY) {
        fprintf(stderr,
                "ERROR: %s: memory section is too big. The file wants %" PRIu64 " bytes. But the capacity is %"  PRIu64 " bytes\n",
                file_path,
                meta.memory_capacity,
                (uint64_t) BM_MEMORY_CAPACITY);
        exit(1);
    }

    if (meta.memory_size > meta.memory_capacity) {
        fprintf(stderr,
                "ERROR: %s: memory size %"PRIu64" is greater than declared memory capacity %"PRIu64"\n",
                file_path,
                meta.memory_size,
                meta.memory_capacity);
        exit(1);
    }

    if (meta.externals_size > BM_EXTERNAL_NATIVES_CAPACITY) {
        fprintf(stderr,
                "ERROR: %s: external names section is too big. The file contains %" PRIu64 " external names. But the capacity is %"  PRIu64 " external names\n",
                file_path,
                meta.externals_size,
                (uint64_t) BM_EXTERNAL_NATIVES_CAPACITY);
        exit(1);
    }

    bm->program_size = fread(bm->program, sizeof(bm->program[0]), meta.program_size, f);

    if (bm->program_size != meta.program_size) {
        fprintf(stderr, "ERROR: %s: read %"PRIu64" program instructions, but expected %"PRIu64"\n",
                file_path,
                bm->program_size,
                meta.program_size);
        exit(1);
    }

    n = fread(bm->memory, sizeof(bm->memory[0]), meta.memory_size, f);

    if (n != meta.memory_size) {
        fprintf(stderr, "ERROR: %s: read %zd bytes of memory section, but expected %"PRIu64" bytes.\n",
                file_path,
                n,
                meta.memory_size);
        exit(1);
    }

    bm->externals_size = fread(bm->externals, sizeof(bm->externals[0]), meta.externals_size, f);
    if (bm->externals_size != meta.externals_size) {
        fprintf(stderr, "ERROR: %s: read %zu external names, but expected %"PRIu64"\n",
                file_path,
                bm->externals_size,
                meta.externals_size);
        exit(1);
    }

    fclose(f);
}

Err native_external(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    Memory_Addr addr = bm->stack[bm->stack_size - 1].as_u64;

    if (addr >= BM_MEMORY_CAPACITY) {
        return ERR_ILLEGAL_MEMORY_ACCESS;
    }

    bm->stack[bm->stack_size - 1].as_ptr = &bm->memory[addr];

    return ERR_OK;
}

Err native_write(Bm *bm)
{
    if (bm->stack_size < 2) {
        return ERR_STACK_UNDERFLOW;
    }

    Memory_Addr addr = bm->stack[bm->stack_size - 2].as_u64;
    uint64_t count = bm->stack[bm->stack_size - 1].as_u64;

    if (addr >= BM_MEMORY_CAPACITY) {
        return ERR_ILLEGAL_MEMORY_ACCESS;
    }

    if (addr + count < addr || addr + count >= BM_MEMORY_CAPACITY) {
        return ERR_ILLEGAL_MEMORY_ACCESS;
    }

    fwrite(&bm->memory[addr], sizeof(bm->memory[0]), count, stdout);

    bm->stack_size -= 2;

    return ERR_OK;
}
