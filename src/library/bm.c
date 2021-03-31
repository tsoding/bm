#include "./bm.h"

Word word_u64(uint64_t u64)
{
    return (Word) {
        .as_u64 = u64
    };
}

Word word_i64(int64_t i64)
{
    return (Word) {
        .as_i64 = i64
    };
}

Word word_f64(double f64)
{
    return (Word) {
        .as_f64 = f64
    };
}

Word word_ptr(void *ptr)
{
    return (Word) {
        .as_ptr = ptr
    };
}

bool inst_has_operand(Inst_Type type)
{
    switch (type) {
    case INST_NOP:
        return false;
    case INST_PUSH:
        return true;
    case INST_DROP:
        return false;
    case INST_DUP:
        return true;
    case INST_PLUSI:
        return false;
    case INST_MINUSI:
        return false;
    case INST_MULTI:
        return false;
    case INST_DIVI:
        return false;
    case INST_MODI:
        return false;
    case INST_MULTU:
        return false;
    case INST_DIVU:
        return false;
    case INST_MODU:
        return false;
    case INST_PLUSF:
        return false;
    case INST_MINUSF:
        return false;
    case INST_MULTF:
        return false;
    case INST_DIVF:
        return false;
    case INST_JMP:
        return true;
    case INST_JMP_IF:
        return true;
    case INST_HALT:
        return false;
    case INST_SWAP:
        return true;
    case INST_NOT:
        return false;
    case INST_EQF:
        return false;
    case INST_GEF:
        return false;
    case INST_GTF:
        return false;
    case INST_LEF:
        return false;
    case INST_LTF:
        return false;
    case INST_NEF:
        return false;

    case INST_EQI:
        return false;
    case INST_GEI:
        return false;
    case INST_GTI:
        return false;
    case INST_LEI:
        return false;
    case INST_LTI:
        return false;
    case INST_NEI:
        return false;

    case INST_EQU:
        return false;
    case INST_GEU:
        return false;
    case INST_GTU:
        return false;
    case INST_LEU:
        return false;
    case INST_LTU:
        return false;
    case INST_NEU:
        return false;

    case INST_RET:
        return false;
    case INST_CALL:
        return true;
    case INST_NATIVE:
        return true;
    case INST_ANDB:
        return false;
    case INST_ORB:
        return false;
    case INST_XOR:
        return false;
    case INST_SHR:
        return false;
    case INST_SHL:
        return false;
    case INST_NOTB:
        return false;
    case INST_READ8:
        return false;
    case INST_READ16:
        return false;
    case INST_READ32:
        return false;
    case INST_READ64:
        return false;
    case INST_WRITE8:
        return false;
    case INST_WRITE16:
        return false;
    case INST_WRITE32:
        return false;
    case INST_WRITE64:
        return false;
    case INST_I2F:
        return false;
    case INST_U2F:
        return false;
    case INST_F2I:
        return false;
    case INST_F2U:
        return false;
    case NUMBER_OF_INSTS:
    default:
        assert(false && "inst_has_operand: unreachable");
        exit(1);
    }
}

bool inst_by_name(String_View name, Inst_Type *output)
{
    for (Inst_Type type = (Inst_Type) 0; type < NUMBER_OF_INSTS; type += 1) {
        if (sv_eq(sv_from_cstr(inst_name(type)), name)) {
            *output = type;
            return true;
        }
    }

    return false;
}

const char *inst_name(Inst_Type type)
{
    switch (type) {
    case INST_NOP:
        return "nop";
    case INST_PUSH:
        return "push";
    case INST_DROP:
        return "drop";
    case INST_DUP:
        return "dup";
    case INST_PLUSI:
        return "plusi";
    case INST_MINUSI:
        return "minusi";
    case INST_MULTI:
        return "multi";
    case INST_DIVI:
        return "divi";
    case INST_MODI:
        return "modi";
    case INST_MULTU:
        return "multu";
    case INST_DIVU:
        return "divu";
    case INST_MODU:
        return "modu";
    case INST_PLUSF:
        return "plusf";
    case INST_MINUSF:
        return "minusf";
    case INST_MULTF:
        return "multf";
    case INST_DIVF:
        return "divf";
    case INST_JMP:
        return "jmp";
    case INST_JMP_IF:
        return "jmp_if";
    case INST_HALT:
        return "halt";
    case INST_SWAP:
        return "swap";
    case INST_NOT:
        return "not";

    case INST_EQI:
        return "eqi";
    case INST_GEI:
        return "gei";
    case INST_GTI:
        return "gti";
    case INST_LEI:
        return "lei";
    case INST_LTI:
        return "lti";
    case INST_NEI:
        return "nei";

    case INST_EQU:
        return "equ";
    case INST_GEU:
        return "geu";
    case INST_GTU:
        return "gtu";
    case INST_LEU:
        return "leu";
    case INST_LTU:
        return "ltu";
    case INST_NEU:
        return "neu";

    case INST_EQF:
        return "eqf";
    case INST_GEF:
        return "gef";
    case INST_GTF:
        return "gtf";
    case INST_LEF:
        return "lef";
    case INST_LTF:
        return "ltf";
    case INST_NEF:
        return "nef";
    case INST_RET:
        return "ret";
    case INST_CALL:
        return "call";
    case INST_NATIVE:
        return "native";
    case INST_ANDB:
        return "andb";
    case INST_ORB:
        return "orb";
    case INST_XOR:
        return "xor";
    case INST_SHR:
        return "shr";
    case INST_SHL:
        return "shl";
    case INST_NOTB:
        return "notb";
    case INST_READ8:
        return "read8";
    case INST_READ16:
        return "read16";
    case INST_READ32:
        return "read32";
    case INST_READ64:
        return "read64";
    case INST_WRITE8:
        return "write8";
    case INST_WRITE16:
        return "write16";
    case INST_WRITE32:
        return "write32";
    case INST_WRITE64:
        return "write64";
    case INST_I2F:
        return "i2f";
    case INST_U2F:
        return "u2f";
    case INST_F2I:
        return "f2i";
    case INST_F2U:
        return "f2u";
    case NUMBER_OF_INSTS:
    default:
        assert(false && "inst_name: unreachable");
        exit(1);
    }
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

    case INST_READ8: {
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }
        const Memory_Addr addr = bm->stack[bm->stack_size - 1].as_u64;
        if (addr >= BM_MEMORY_CAPACITY) {
            return ERR_ILLEGAL_MEMORY_ACCESS;
        }
        bm->stack[bm->stack_size - 1].as_u64 = bm->memory[addr];
        bm->ip += 1;
    }
    break;

    case INST_READ16: {
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }
        const Memory_Addr addr = bm->stack[bm->stack_size - 1].as_u64;
        if (addr >= BM_MEMORY_CAPACITY - 1) {
            return ERR_ILLEGAL_MEMORY_ACCESS;
        }
        bm->stack[bm->stack_size - 1].as_u64 = *(uint16_t*)&bm->memory[addr];
        bm->ip += 1;
    }
    break;

    case INST_READ32: {
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }
        const Memory_Addr addr = bm->stack[bm->stack_size - 1].as_u64;
        if (addr >= BM_MEMORY_CAPACITY - 3) {
            return ERR_ILLEGAL_MEMORY_ACCESS;
        }
        bm->stack[bm->stack_size - 1].as_u64 = *(uint32_t*)&bm->memory[addr];
        bm->ip += 1;
    }
    break;

    case INST_READ64: {
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }
        const Memory_Addr addr = bm->stack[bm->stack_size - 1].as_u64;
        if (addr >= BM_MEMORY_CAPACITY - 7) {
            return ERR_ILLEGAL_MEMORY_ACCESS;
        }
        bm->stack[bm->stack_size - 1].as_u64 = *(uint64_t*)&bm->memory[addr];
        bm->ip += 1;
    }
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
        *(uint16_t*)&bm->memory[addr] = (uint16_t) bm->stack[bm->stack_size - 1].as_u64;
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
        *(uint32_t*)&bm->memory[addr] = (uint32_t) bm->stack[bm->stack_size - 1].as_u64;
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
        *(uint64_t*)&bm->memory[addr] = bm->stack[bm->stack_size - 1].as_u64;
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
        fprintf(stderr, "ERROR: %s: read %"PRIu64" external names, but expected %"PRIu64"\n",
                file_path,
                bm->externals_size,
                meta.externals_size);
        exit(1);
    }

    fclose(f);
}

void bm_load_standard_natives(Bm *bm)
{
    // TODO(#35): some sort of mechanism to load native functions from DLLs
    bm_push_native(bm, native_write); // 0
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
