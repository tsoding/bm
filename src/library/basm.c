#ifndef _WIN32
#    define _DEFAULT_SOURCE
#    define _POSIX_C_SOURCE 200112L
#    include <sys/types.h>
#    include <sys/stat.h>
#    include <unistd.h>
#else
#    define  WIN32_LEAN_AND_MEAN
#    include "windows.h"
#endif // _WIN32

#include <stdio.h>

#include "./basm.h"

Binding *basm_resolve_binding(Basm *basm, String_View name)
{
    for (size_t i = 0; i < basm->bindings_size; ++i) {
        if (sv_eq(basm->bindings[i].name, name)) {
            return &basm->bindings[i];
        }
    }

    return NULL;
}

void basm_bind_value(Basm *basm, String_View name, Word value, Binding_Kind kind, File_Location location)
{
    assert(basm->bindings_size < BASM_BINDINGS_CAPACITY);

    Binding *existing = basm_resolve_binding(basm, name);
    if (existing) {
        fprintf(stderr,
                FL_Fmt": ERROR: name `"SV_Fmt"` is already bound\n",
                FL_Arg(location),
                SV_Arg(name));
        fprintf(stderr,
                FL_Fmt": NOTE: first binding is located here\n",
                FL_Arg(existing->location));
        exit(1);
    }

    basm->bindings[basm->bindings_size++] = (Binding) {
        .name = name,
        .value = value,
        .status = BINDING_EVALUATED,
        .kind = kind,
        .location = location,
    };
}

void basm_bind_expr(Basm *basm, String_View name, Expr expr, Binding_Kind kind, File_Location location)
{
    assert(basm->bindings_size < BASM_BINDINGS_CAPACITY);

    Binding *existing = basm_resolve_binding(basm, name);
    if (existing) {
        fprintf(stderr,
                FL_Fmt": ERROR: name `"SV_Fmt"` is already bound\n",
                FL_Arg(location),
                SV_Arg(name));
        fprintf(stderr,
                FL_Fmt": NOTE: first binding is located here\n",
                FL_Arg(existing->location));
        exit(1);
    }

    basm->bindings[basm->bindings_size++] = (Binding) {
        .name = name,
        .expr = expr,
        .kind = kind,
        .location = location,
    };
}

void basm_push_deferred_operand(Basm *basm, Inst_Addr addr, Expr expr, File_Location location)
{
    assert(basm->deferred_operands_size < BASM_DEFERRED_OPERANDS_CAPACITY);
    basm->deferred_operands[basm->deferred_operands_size++] =
    (Deferred_Operand) {
        .addr = addr, .expr = expr, .location = location
    };
}

Word basm_push_string_to_memory(Basm *basm, String_View sv)
{
    assert(basm->memory_size + sv.count <= BM_MEMORY_CAPACITY);

    Word result = word_u64(basm->memory_size);
    memcpy(basm->memory + basm->memory_size, sv.data, sv.count);
    basm->memory_size += sv.count;

    if (basm->memory_size > basm->memory_capacity) {
        basm->memory_capacity = basm->memory_size;
    }

    basm->string_lengths[basm->string_lengths_size++] = (String_Length) {
        .addr = result.as_u64,
        .length = sv.count,
    };

    return result;
}

bool basm_string_length_by_addr(Basm *basm, Inst_Addr addr, Word *length)
{
    for (size_t i = 0; i < basm->string_lengths_size; ++i) {
        if (basm->string_lengths[i].addr == addr) {
            if (length) {
                *length = word_u64(basm->string_lengths[i].length);
            }
            return true;
        }
    }

    return false;
}

void basm_save_to_file(Basm *basm, const char *file_path)
{
    FILE *f = fopen(file_path, "wb");
    if (f == NULL) {
        fprintf(stderr, "ERROR: Could not open file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    Bm_File_Meta meta = {
        .magic = BM_FILE_MAGIC,
        .version = BM_FILE_VERSION,
        .entry = basm->entry,
        .program_size = basm->program_size,
        .memory_size = basm->memory_size,
        .memory_capacity = basm->memory_capacity,
    };

    fwrite(&meta, sizeof(meta), 1, f);
    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not write to file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    fwrite(basm->program, sizeof(basm->program[0]), basm->program_size, f);
    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not write to file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    fwrite(basm->memory, sizeof(basm->memory[0]), basm->memory_size, f);
    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not write to file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    fclose(f);
}

static void basm_translate_bind_directive(Basm *basm, String_View *line, File_Location location, Binding_Kind binding_kind)
{
    *line = sv_trim(*line);
    String_View name = sv_chop_by_delim(line, ' ');
    if (name.count > 0) {
        *line = sv_trim(*line);
        Expr expr = parse_expr_from_sv(&basm->arena, *line, location);

        basm_bind_expr(basm, name, expr, binding_kind, location);

    } else {
        fprintf(stderr,
                FL_Fmt": ERROR: binding name is not provided\n",
                FL_Arg(location));
        exit(1);
    }
}

void basm_translate_source(Basm *basm, String_View input_file_path_)
{
    {
        String_View resolved_path = SV_NULL;
        if (basm_resolve_include_file_path(basm, input_file_path_, &resolved_path)) {
            input_file_path_ = resolved_path;
        }
    }

    String_View original_source = {0};
    if (arena_slurp_file(&basm->arena, input_file_path_, &original_source) < 0) {
        if (basm->include_level > 0) {
            fprintf(stderr, FL_Fmt": ERROR: could not read file `"SV_Fmt"`: %s\n",
                    FL_Arg(basm->include_location),
                    SV_Arg(input_file_path_), strerror(errno));
        } else {
            fprintf(stderr, "ERROR: could not read file `"SV_Fmt"`: %s\n",
                    SV_Arg(input_file_path_), strerror(errno));
        }
        exit(1);
    }

    String_View source = original_source;

    File_Location location = {
        .file_path = input_file_path_,
    };

    // First pass
    while (source.count > 0) {
        String_View line = sv_trim(sv_chop_by_delim(&source, '\n'));
        line = sv_trim(sv_chop_by_delim(&line, BASM_COMMENT_SYMBOL));
        location.line_number += 1;
        if (line.count > 0) {
            String_View token = sv_trim(sv_chop_by_delim(&line, ' '));

            // Pre-processor
            if (token.count > 0 && *token.data == BASM_PP_SYMBOL) {
                token.count -= 1;
                token.data  += 1;
                if (sv_eq(token, sv_from_cstr("bind"))) {
                    fprintf(stderr, FL_Fmt": ERROR: %%bind directive has been removed! Use %%const directive to define consts. Use %%native directive to define native functions.\n", FL_Arg(location));
                    exit(1);
                } else if (sv_eq(token, sv_from_cstr("const"))) {
                    basm_translate_bind_directive(basm, &line, location, BINDING_CONST);
                } else if (sv_eq(token, sv_from_cstr("native"))) {
                    basm_translate_bind_directive(basm, &line, location, BINDING_NATIVE);
                } else if (sv_eq(token, sv_from_cstr("assert"))) {
                    Expr expr = parse_expr_from_sv(&basm->arena, sv_trim(line), location);
                    basm->deferred_asserts[basm->deferred_asserts_size++] = (Deferred_Assert) {
                        .expr = expr,
                        .location = location,
                    };
                } else if (sv_eq(token, sv_from_cstr("include"))) {
                    line = sv_trim(line);


                    if (line.count > 0) {
                        if (*line.data == '"' && line.data[line.count - 1] == '"') {
                            line.data  += 1;
                            line.count -= 2;

                            if (basm->include_level + 1 >= BASM_MAX_INCLUDE_LEVEL) {
                                fprintf(stderr,
                                        FL_Fmt": ERROR: exceeded maximum include level\n",
                                        FL_Arg(location));
                                exit(1);
                            }

                            {
                                File_Location prev_include_location = basm->include_location;
                                basm->include_level += 1;
                                basm->include_location = location;
                                basm_translate_source(basm, line);
                                basm->include_location = prev_include_location;
                                basm->include_level -= 1;
                            }
                        } else {
                            fprintf(stderr,
                                    FL_Fmt": ERROR: include file path has to be surrounded with quotation marks\n",
                                    FL_Arg(location));
                            exit(1);
                        }
                    } else {
                        fprintf(stderr,
                                FL_Fmt": ERROR: include file path is not provided\n",
                                FL_Arg(location));
                        exit(1);
                    }
                } else if (sv_eq(token, sv_from_cstr("entry"))) {
                    if (basm->has_entry) {
                        fprintf(stderr,
                                FL_Fmt": ERROR: entry point has been already set!\n",
                                FL_Arg(location));
                        fprintf(stderr, FL_Fmt": NOTE: the first entry point\n", FL_Arg(basm->entry_location));
                        exit(1);
                    }

                    line = sv_trim(line);

                    Expr expr = parse_expr_from_sv(&basm->arena, line, location);

                    if (expr.kind != EXPR_KIND_BINDING) {
                        fprintf(stderr, FL_Fmt": ERROR: only bindings are allowed to be set as entry points for now.\n",
                                FL_Arg(location));
                        exit(1);
                    }

                    basm->deferred_entry_binding_name = expr.value.as_binding;
                    basm->has_entry = true;
                    basm->entry_location = location;
                } else {
                    fprintf(stderr,
                            FL_Fmt": ERROR: unknown pre-processor directive `"SV_Fmt"`\n",
                            FL_Arg(location),
                            SV_Arg(token));
                    exit(1);

                }
            } else {
                // Label binding
                if (token.count > 0 && token.data[token.count - 1] == ':') {
                    String_View label = {
                        .count = token.count - 1,
                        .data = token.data
                    };

                    basm_bind_value(basm, label, word_u64(basm->program_size), BINDING_LABEL, location);
                    token = sv_trim(sv_chop_by_delim(&line, ' '));
                }

                // Instruction
                if (token.count > 0) {
                    String_View operand = line;
                    Inst_Type inst_type = INST_NOP;
                    if (inst_by_name(token, &inst_type)) {
                        assert(basm->program_size < BM_PROGRAM_CAPACITY);
                        basm->program[basm->program_size].type = inst_type;

                        if (inst_has_operand(inst_type)) {
                            if (operand.count == 0) {
                                fprintf(stderr, FL_Fmt": ERROR: instruction `"SV_Fmt"` requires an operand\n",
                                        FL_Arg(location),
                                        SV_Arg(token));
                                exit(1);
                            }

                            Expr expr = parse_expr_from_sv(&basm->arena, operand, location);

                            if (expr.kind == EXPR_KIND_BINDING) {
                                basm_push_deferred_operand(basm, basm->program_size, expr, location);
                            } else {
                                assert(expr.kind != EXPR_KIND_BINDING);
                                basm->program[basm->program_size].operand =
                                    basm_expr_eval(basm, expr, location);
                            }
                        }

                        basm->program_size += 1;
                    } else {
                        fprintf(stderr, FL_Fmt": ERROR: unknown instruction `"SV_Fmt"`\n",
                                FL_Arg(location),
                                SV_Arg(token));
                        exit(1);
                    }
                }
            }
        }
    }

    // Second pass
    for (size_t i = 0; i < basm->deferred_operands_size; ++i) {
        Expr expr = basm->deferred_operands[i].expr;
        assert(expr.kind == EXPR_KIND_BINDING);
        String_View name = expr.value.as_binding;

        Inst_Addr addr = basm->deferred_operands[i].addr;
        Binding *binding = basm_resolve_binding(basm, name);
        if (binding == NULL) {
            fprintf(stderr, FL_Fmt": ERROR: unknown binding `"SV_Fmt"`\n",
                    FL_Arg(basm->deferred_operands[i].location),
                    SV_Arg(name));
            exit(1);
        }

        if (basm->program[addr].type == INST_CALL && binding->kind != BINDING_LABEL) {
            fprintf(stderr, FL_Fmt": ERROR: trying to call not a label. `"SV_Fmt"` is %s, but the call instructions accepts only literals or labels.\n", FL_Arg(basm->deferred_operands[i].location), SV_Arg(name), binding_kind_as_cstr(binding->kind));
            exit(1);
        }

        if (basm->program[addr].type == INST_NATIVE && binding->kind != BINDING_NATIVE) {
            fprintf(stderr, FL_Fmt": ERROR: trying to invoke native function from a binding that is %s. Bindings for native functions have to be defined via `%%native` basm directive.\n", FL_Arg(basm->deferred_operands[i].location), binding_kind_as_cstr(binding->kind));
            exit(1);
        }

        basm->program[addr].operand = basm_binding_eval(basm, binding, basm->deferred_operands[i].location);
    }

    // Eval deferred asserts
    for (size_t i = 0; i < basm->deferred_asserts_size; ++i) {
        Word value = basm_expr_eval(
                         basm,
                         basm->deferred_asserts[i].expr,
                         basm->deferred_asserts[i].location);
        if (!value.as_u64) {
            fprintf(stderr, FL_Fmt": ERROR: assertion failed\n",
                    FL_Arg(basm->deferred_asserts[i].location));
            exit(1);
        }
    }

    // Resolving deferred entry point
    if (basm->has_entry && basm->deferred_entry_binding_name.count > 0) {
        Binding *binding = basm_resolve_binding(
                               basm,
                               basm->deferred_entry_binding_name);
        if (binding == NULL) {
            fprintf(stderr, FL_Fmt": ERROR: unknown binding `"SV_Fmt"`\n",
                    FL_Arg(basm->entry_location),
                    SV_Arg(basm->deferred_entry_binding_name));
            exit(1);
        }

        if (binding->kind != BINDING_LABEL) {
            fprintf(stderr, FL_Fmt": ERROR: trying to set a %s as an entry point. Entry point has to be a label.\n", FL_Arg(basm->entry_location), binding_kind_as_cstr(binding->kind));
            exit(1);
        }

        basm->entry = basm_binding_eval(basm, binding, basm->entry_location).as_u64;
    }
}

Word basm_binding_eval(Basm *basm, Binding *binding, File_Location location)
{
    if (binding->status == BINDING_EVALUATING) {
        fprintf(stderr, FL_Fmt": ERROR: cycling binding definition.\n",
                FL_Arg(binding->location));
        exit(1);
    }

    if (binding->status == BINDING_UNEVALUATED) {
        binding->status = BINDING_EVALUATING;
        Word value = basm_expr_eval(basm, binding->expr, location);
        binding->status = BINDING_EVALUATED;
        binding->value = value;
    }

    return binding->value;
}

static Word basm_binary_op_eval(Basm *basm, Binary_Op *binary_op, File_Location location)
{
    Word left = basm_expr_eval(basm, binary_op->left, location);
    Word right = basm_expr_eval(basm, binary_op->right, location);

    switch (binary_op->kind) {
    case BINARY_OP_PLUS: {
        // TODO(#183): compile-time sum can only work with integers
        return word_u64(left.as_u64 + right.as_u64);
    }
    break;

    case BINARY_OP_GT: {
        return word_u64(left.as_u64 > right.as_u64);
    }
    break;

    default: {
        assert(false && "basm_binary_op_eval: unreachable");
        exit(1);
    }
    }
}

Word basm_expr_eval(Basm *basm, Expr expr, File_Location location)
{
    switch (expr.kind) {
    case EXPR_KIND_LIT_INT:
        return word_u64(expr.value.as_lit_int);

    case EXPR_KIND_LIT_FLOAT:
        return word_f64(expr.value.as_lit_float);

    case EXPR_KIND_LIT_CHAR:
        return word_u64((uint64_t) expr.value.as_lit_char);

    case EXPR_KIND_LIT_STR:
        return basm_push_string_to_memory(basm, expr.value.as_lit_str);

    case EXPR_KIND_FUNCALL: {
        if (sv_eq(expr.value.as_funcall->name, sv_from_cstr("len"))) {
            const size_t actual_arity = funcall_args_len(expr.value.as_funcall->args);
            if (actual_arity != 1) {
                fprintf(stderr, FL_Fmt": ERROR: len() expects 1 argument but got %zu\n",
                        FL_Arg(location), actual_arity);
                exit(1);
            }

            Word addr = basm_expr_eval(basm, expr.value.as_funcall->args->value, location);
            Word length = {0};
            if (!basm_string_length_by_addr(basm, addr.as_u64, &length)) {
                fprintf(stderr, FL_Fmt": ERROR: Could not compute the length of string at address %"PRIu64"\n", FL_Arg(location), addr.as_u64);
                exit(1);
            }

            return length;
        } else {
            fprintf(stderr,
                    FL_Fmt": ERROR: Unknown translation time function `"SV_Fmt"`\n",
                    FL_Arg(location), SV_Arg(expr.value.as_funcall->name));
            exit(1);
        }
    }
    break;

    case EXPR_KIND_BINDING: {
        String_View name = expr.value.as_binding;
        Binding *binding = basm_resolve_binding(basm, name);
        if (binding == NULL) {
            fprintf(stderr, FL_Fmt": ERROR: could find binding `"SV_Fmt"`.\n",
                    FL_Arg(location), SV_Arg(name));
            exit(1);
        }

        return basm_binding_eval(basm, binding, location);
    }
    break;

    case EXPR_KIND_BINARY_OP: {
        return basm_binary_op_eval(basm, expr.value.as_binary_op, location);
    }
    break;

    default: {
        assert(false && "basm_expr_eval: unreachable");
        exit(1);
    }
    break;
    }
}

const char *binding_kind_as_cstr(Binding_Kind kind)
{
    switch (kind) {
    case BINDING_CONST:
        return "const";
    case BINDING_LABEL:
        return "label";
    case BINDING_NATIVE:
        return "native";
    default:
        assert(false && "binding_kind_as_cstr: unreachable");
        exit(0);
    }
}

void basm_push_include_path(Basm *basm, String_View path)
{
    assert(basm->include_paths_size < BASM_INCLUDE_PATHS_CAPACITY);
    basm->include_paths[basm->include_paths_size++] = path;
}

static bool path_file_exist(const char *file_path)
{
#ifndef _WIN32
    struct stat statbuf = {0};
    return stat(file_path, &statbuf) == 0 &&
           ((statbuf.st_mode & S_IFMT) == S_IFREG);
#else
    // https://stackoverflow.com/a/6218957
    DWORD dwAttrib = GetFileAttributes(file_path);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
            !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#endif
}

static String_View path_join(Arena *arena, String_View base, String_View file_path)
{
#ifndef _WIN32
    const String_View sep = sv_from_cstr("/");
#else
    const String_View sep = sv_from_cstr("\\");
#endif // _WIN32
    const size_t result_size = base.count + sep.count + file_path.count;
    char *result = arena_alloc(arena, result_size);
    assert(result);

    {
        char *append = result;

        memcpy(append, base.data, base.count);
        append += base.count;

        memcpy(append, sep.data, sep.count);
        append += sep.count;

        memcpy(append, file_path.data, file_path.count);
        append += file_path.count;
    }

    return (String_View) {
        .count = result_size,
        .data = result,
    };
}

bool basm_resolve_include_file_path(Basm *basm,
                                    String_View file_path,
                                    String_View *resolved_path)
{
    for (size_t i = 0; i < basm->include_paths_size; ++i) {
        String_View path = path_join(&basm->arena, basm->include_paths[i],
                                     file_path);
        if (path_file_exist(arena_sv_to_cstr(&basm->arena, path))) {
            if (resolved_path) {
                *resolved_path = path;
            }
            return true;
        }
    }

    return false;
}
