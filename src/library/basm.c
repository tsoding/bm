#include <stdio.h>

#include "./basm.h"
#include "./linizer.h"
#include "./path.h"

Eval_Status eval_status_ok(void)
{
    return (Eval_Status) {
        .kind = EVAL_STATUS_KIND_OK
    };
}

Eval_Status eval_status_deferred(Binding *deferred_binding)
{
    return (Eval_Status) {
        .kind = EVAL_STATUS_KIND_DEFERRED,
        .deferred_binding = deferred_binding
    };
}

Binding *scope_resolve_binding(Scope *scope, String_View name)
{
    for (size_t i = 0; i < scope->bindings_size; ++i) {
        if (sv_eq(scope->bindings[i].name, name)) {
            return &scope->bindings[i];
        }
    }

    return NULL;
}

void basm_push_scope(Basm *basm, Scope *scope)
{
    assert(scope->previous == NULL);
    scope->previous = basm->scope;
    basm->scope = scope;
}

void basm_push_new_scope(Basm *basm)
{
    basm_push_scope(basm, arena_alloc(&basm->arena, sizeof(*basm->scope)));
}

void basm_pop_scope(Basm *basm)
{
    assert(basm->scope != NULL);
    basm->scope = basm->scope->previous;
}

Binding *basm_resolve_binding(Basm *basm, String_View name)
{
    for (Scope *scope = basm->scope;
            scope != NULL;
            scope = scope->previous) {
        Binding *binding = scope_resolve_binding(scope, name);
        if (binding) {
            return binding;
        }
    }

    return NULL;
}

void scope_bind_value(Scope *scope, String_View name, Word value, Binding_Kind kind, File_Location location)
{
    Binding *existing = scope_resolve_binding(scope, name);
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

    assert(scope->bindings_size < BASM_BINDINGS_CAPACITY);
    scope->bindings[scope->bindings_size++] = (Binding) {
        .name = name,
        .value = value,
        .status = BINDING_EVALUATED,
        .kind = kind,
        .location = location,
    };
}

void scope_defer_binding(Scope *scope, String_View name, Binding_Kind kind, File_Location location)
{
    assert(scope->bindings_size < BASM_BINDINGS_CAPACITY);

    Binding *existing = scope_resolve_binding(scope, name);
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

    scope->bindings[scope->bindings_size++] = (Binding) {
        .name = name,
        .status = BINDING_DEFERRED,
        .kind = kind,
        .location = location,
    };
}

void scope_bind_expr(Scope *scope, String_View name, Expr expr, Binding_Kind kind, File_Location location)
{
    assert(scope->bindings_size < BASM_BINDINGS_CAPACITY);

    Binding *existing = scope_resolve_binding(scope, name);
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

    scope->bindings[scope->bindings_size++] = (Binding) {
        .name = name,
        .expr = expr,
        .kind = kind,
        .location = location,
    };
}

void basm_bind_value(Basm *basm, String_View name, Word value, Binding_Kind kind, File_Location location)
{
    assert(basm->scope != NULL);
    scope_bind_value(basm->scope, name, value, kind, location);
}

void basm_defer_binding(Basm *basm, String_View name, Binding_Kind kind, File_Location location)
{
    assert(basm->scope != NULL);
    scope_defer_binding(basm->scope, name, kind, location);
}

void basm_bind_expr(Basm *basm, String_View name, Expr expr, Binding_Kind kind, File_Location location)
{
    assert(basm->scope != NULL);
    scope_bind_expr(basm->scope, name, expr, kind, location);
}

void basm_push_deferred_operand(Basm *basm, Inst_Addr addr, Expr expr, File_Location location)
{
    assert(basm->deferred_operands_size < BASM_DEFERRED_OPERANDS_CAPACITY);
    basm->deferred_operands[basm->deferred_operands_size++] = (Deferred_Operand) {
        .addr = addr,
        .expr = expr,
        .location = location,
        .scope = basm->scope,
    };
}

Word basm_push_buffer_to_memory(Basm *basm, uint8_t *buffer, uint64_t buffer_size)
{
    assert(basm->memory_size + buffer_size <= BM_MEMORY_CAPACITY);

    Word result = word_u64(basm->memory_size);
    memcpy(basm->memory + basm->memory_size, buffer, buffer_size);
    basm->memory_size += buffer_size;

    if (basm->memory_size > basm->memory_capacity) {
        basm->memory_capacity = basm->memory_size;
    }

    basm->string_lengths[basm->string_lengths_size++] = (String_Length) {
        .addr = result.as_u64,
        .length = buffer_size,
    };

    return result;
}

Word basm_push_byte_array_to_memory(Basm *basm, uint64_t size, uint8_t value)
{
    assert(basm->memory_size + size <= BM_MEMORY_CAPACITY);

    Word result = word_u64(basm->memory_size);
    memset(basm->memory + basm->memory_size, value, size);
    basm->memory_size += size;

    if (basm->memory_size > basm->memory_capacity) {
        basm->memory_capacity = basm->memory_size;
    }

    basm->string_lengths[basm->string_lengths_size++] = (String_Length) {
        .addr = result.as_u64,
        .length = size,
    };

    return result;
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
        .externals_size = basm->external_natives_size,
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

    fwrite(basm->external_natives, sizeof(basm->external_natives[0]), basm->external_natives_size, f);
    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not write to file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    fclose(f);
}

const char *binding_status_as_cstr(Binding_Status status)
{
    switch (status) {
    case BINDING_UNEVALUATED:
        return "BINDING_UNEVALUATED";
    case BINDING_EVALUATING:
        return "BINDING_EVALUATING";
    case BINDING_EVALUATED:
        return "BINDING_EVALUATED";
    case BINDING_DEFERRED:
        return "BINDING_DEFERRED";
    default: {
        assert(false && "binding_status_as_cstr: unreachable");
        exit(1);
    }
    }
}

void basm_translate_block(Basm *basm, Block_Statement *block)
{
    // first pass begin
    {
        for (Block_Statement *iter = block; iter != NULL; iter = iter->next) {
            Statement statement = iter->statement;
            switch (statement.kind) {
            case STATEMENT_KIND_LABEL: {
                basm_defer_binding(basm,
                                   statement.value.as_label.name,
                                   BINDING_LABEL,
                                   statement.location);
            }
            break;
            case STATEMENT_KIND_CONST:
                basm_translate_const(basm, statement.value.as_const, statement.location);
                break;
            case STATEMENT_KIND_NATIVE:
                basm_translate_native(basm, statement.value.as_native, statement.location);
                break;
            case STATEMENT_KIND_INCLUDE:
                basm_translate_include(basm, statement.value.as_include, statement.location);
                break;
            case STATEMENT_KIND_ASSERT:
                basm_translate_assert(basm, statement.value.as_assert, statement.location);
                break;
            case STATEMENT_KIND_ERROR:
                basm_translate_error(statement.value.as_error, statement.location);
                break;
            case STATEMENT_KIND_ENTRY:
                basm_translate_entry(basm, statement.value.as_entry, statement.location);
                break;
            case STATEMENT_KIND_BLOCK:
                basm_translate_block(basm, statement.value.as_block);
                break;

            case STATEMENT_KIND_MACRODEF:
                basm_translate_macrodef_statement(basm, statement.value.as_macrodef, statement.location);
                break;

            case STATEMENT_KIND_MACROCALL:
            case STATEMENT_KIND_FUNCDEF:
            case STATEMENT_KIND_FOR:
            case STATEMENT_KIND_SCOPE:
            case STATEMENT_KIND_EMIT_INST:
            case STATEMENT_KIND_IF:
                // NOTE: ignored at the first pass
                break;

            default:
                assert(false && "basm_translate_statement: unreachable");
                exit(1);
            }
        }
    }
    // first pass end

    // second pass begin
    {
        for (Block_Statement *iter = block; iter != NULL; iter = iter->next) {
            Statement statement = iter->statement;
            switch (statement.kind) {
            case STATEMENT_KIND_EMIT_INST: {
                basm_translate_emit_inst(basm, statement.value.as_emit_inst, statement.location);
            }
            break;

            case STATEMENT_KIND_LABEL: {
                Binding *binding = basm_resolve_binding(basm, statement.value.as_label.name);
                assert(binding != NULL);
                assert(binding->kind == BINDING_LABEL);
                assert(binding->status == BINDING_DEFERRED);

                binding->status = BINDING_EVALUATED;
                binding->value.as_u64 = basm->program_size;
            }
            break;

            case STATEMENT_KIND_IF: {
                basm_translate_if(basm, statement.value.as_if, statement.location);
            }
            break;

            case STATEMENT_KIND_SCOPE: {
                basm_push_new_scope(basm);
                basm_translate_block(basm, statement.value.as_scope);
                basm_pop_scope(basm);
            }
            break;

            case STATEMENT_KIND_FOR: {
                basm_translate_for(basm, statement.value.as_for, statement.location);
            }
            break;

            case STATEMENT_KIND_MACROCALL:
                basm_translate_macrocall_statement(basm, statement.value.as_macrocall, statement.location);
                break;

            case STATEMENT_KIND_MACRODEF:
            case STATEMENT_KIND_NATIVE:
            case STATEMENT_KIND_FUNCDEF:
            case STATEMENT_KIND_BLOCK:
            case STATEMENT_KIND_ENTRY:
            case STATEMENT_KIND_ERROR:
            case STATEMENT_KIND_ASSERT:
            case STATEMENT_KIND_INCLUDE:
            case STATEMENT_KIND_CONST:
                // NOTE: ignored at the second pass
                break;

            default:
                assert(false && "basm_translate_statement: unreachable");
                exit(1);
            }
        }
    }
    // second pass end
}

void basm_eval_deferred_asserts(Basm *basm)
{
    Scope *saved_basm_scope = basm->scope;

    for (size_t i = 0; i < basm->deferred_asserts_size; ++i) {
        Word value = {0};
        assert(basm->deferred_asserts[i].scope);
        basm->scope = basm->deferred_asserts[i].scope;
        // TODO(#285): make basm_expr_eval accept the scope in which you want to evaluate the expression
        // So there is no need for that silly `saved_basm_scope` hack above
        Eval_Status status = basm_expr_eval(
                                 basm,
                                 basm->deferred_asserts[i].expr,
                                 basm->deferred_asserts[i].location,
                                 &value);
        assert(status.kind == EVAL_STATUS_KIND_OK);

        if (!value.as_u64) {
            fprintf(stderr, FL_Fmt": ERROR: assertion failed\n",
                    FL_Arg(basm->deferred_asserts[i].location));
            exit(1);
        }
    }

    basm->scope = saved_basm_scope;
}

void basm_eval_deferred_operands(Basm *basm)
{
    Scope *saved_basm_scope = basm->scope;
    for (size_t i = 0; i < basm->deferred_operands_size; ++i) {
        assert(basm->deferred_operands[i].scope);
        basm->scope = basm->deferred_operands[i].scope;

        Inst_Addr addr = basm->deferred_operands[i].addr;
        Expr expr = basm->deferred_operands[i].expr;
        File_Location location = basm->deferred_operands[i].location;

        if (expr.kind == EXPR_KIND_BINDING) {
            String_View name = expr.value.as_binding;

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
                fprintf(stderr, FL_Fmt": ERROR: trying to invoke native function from a binding that is %s. Bindings for native functions have to be defined via `%%native` basm directives.\n", FL_Arg(basm->deferred_operands[i].location), binding_kind_as_cstr(binding->kind));
                exit(1);
            }
        }

        Eval_Status status = basm_expr_eval(
                                 basm, expr, location,
                                 &basm->program[addr].operand);
        assert(status.kind == EVAL_STATUS_KIND_OK);
    }

    basm->scope = saved_basm_scope;
}

void basm_eval_deferred_entry(Basm *basm)
{
    Scope *saved_basm_scope = basm->scope;
    if (basm->deferred_entry.binding_name.count > 0) {
        assert(basm->deferred_entry.scope);
        basm->scope = basm->deferred_entry.scope;

        if (basm->has_entry) {
            fprintf(stderr,
                    FL_Fmt": ERROR: entry point has been already set!\n",
                    FL_Arg(basm->deferred_entry.location));
            fprintf(stderr, FL_Fmt": NOTE: the first entry point\n",
                    FL_Arg(basm->entry_location));
            exit(1);

        }

        Binding *binding = basm_resolve_binding(
                               basm,
                               basm->deferred_entry.binding_name);
        if (binding == NULL) {
            fprintf(stderr, FL_Fmt": ERROR: unknown binding `"SV_Fmt"`\n",
                    FL_Arg(basm->deferred_entry.location),
                    SV_Arg(basm->deferred_entry.binding_name));
            exit(1);
        }

        if (binding->kind != BINDING_LABEL) {
            fprintf(stderr, FL_Fmt": ERROR: trying to set a %s as an entry point. Entry point has to be a label.\n", FL_Arg(basm->deferred_entry.location), binding_kind_as_cstr(binding->kind));
            exit(1);
        }

        Word entry = {0};

        Eval_Status status = basm_binding_eval(basm, binding, &entry);
        assert(status.kind == EVAL_STATUS_KIND_OK);

        basm->entry = entry.as_u64;
        basm->has_entry = true;
        basm->entry_location = basm->deferred_entry.location;
    }

    basm->scope = saved_basm_scope;
}

void basm_translate_emit_inst(Basm *basm, Emit_Inst_Statement emit_inst, File_Location location)
{
    assert(basm->program_size < BM_PROGRAM_CAPACITY);
    basm->program[basm->program_size].type = emit_inst.type;

    if (inst_has_operand(emit_inst.type)) {
        basm_push_deferred_operand(basm, basm->program_size, emit_inst.operand, location);
    }

    basm->program_size += 1;
}

void basm_translate_entry(Basm *basm, Entry_Statement entry, File_Location location)
{
    assert(basm->scope);

    if (basm->deferred_entry.binding_name.count > 0) {
        fprintf(stderr,
                FL_Fmt": ERROR: entry point has been already set within the same scope!\n",
                FL_Arg(location));
        fprintf(stderr, FL_Fmt": NOTE: the first entry point\n",
                FL_Arg(basm->deferred_entry.location));
        exit(1);
    }

    if (entry.value.kind != EXPR_KIND_BINDING) {
        fprintf(stderr, FL_Fmt": ERROR: only bindings are allowed to be set as entry points for now.\n",
                FL_Arg(location));
        exit(1);
    }

    String_View label = entry.value.value.as_binding;
    basm->deferred_entry.binding_name = label;
    basm->deferred_entry.location = location;
    basm->deferred_entry.scope = basm->scope;
}

void basm_translate_const(Basm *basm, Const_Statement konst, File_Location location)
{
    basm_bind_expr(basm,
                   konst.name,
                   konst.value,
                   BINDING_CONST,
                   location);
}

void basm_translate_native(Basm *basm, Native_Statement native, File_Location location)
{
    if (native.name.count >= NATIVE_NAME_CAPACITY - 1) {
        fprintf(stderr, FL_Fmt": ERROR: exceed maximum size of the name for a native function. The limit is %zu.\n", FL_Arg(location), (size_t) (NATIVE_NAME_CAPACITY - 1));
        exit(1);
    }

    memset(basm->external_natives[basm->external_natives_size].name,
           0,
           NATIVE_NAME_CAPACITY);

    memcpy(basm->external_natives[basm->external_natives_size].name,
           native.name.data,
           native.name.count);

    basm_bind_value(basm,
                    native.name,
                    word_u64(basm->external_natives_size),
                    BINDING_NATIVE,
                    location);

    basm->external_natives_size += 1;
}

void basm_translate_assert(Basm *basm, Assert_Statement azzert, File_Location location)
{
    assert(basm->scope != NULL);
    basm->deferred_asserts[basm->deferred_asserts_size++] = (Deferred_Assert) {
        .expr = azzert.condition,
        .location = location,
        .scope = basm->scope,
    };
}

void basm_translate_error(Error_Statement error, File_Location location)
{
    fprintf(stderr, FL_Fmt": ERROR: "SV_Fmt"\n",
            FL_Arg(location), SV_Arg(error.message));
    exit(1);
}

void basm_translate_include(Basm *basm, Include_Statement include, File_Location location)
{
    {
        String_View resolved_path = SV_NULL;
        if (basm_resolve_include_file_path(basm, include.path, &resolved_path)) {
            include.path = resolved_path;
        }
    }

    {
        File_Location prev_include_location = basm->include_location;
        basm->include_level += 1;
        basm->include_location = location;
        basm_translate_source_file(basm, include.path);
        basm->include_location = prev_include_location;
        basm->include_level -= 1;
    }
}

void basm_translate_for(Basm *basm, For_Statement phor, File_Location location)
{
    Word from = {0};
    {
        Eval_Status status = basm_expr_eval(basm, phor.from, location, &from);
        if (status.kind == EVAL_STATUS_KIND_DEFERRED) {
            assert(status.deferred_binding);
            assert(status.deferred_binding->kind == BINDING_LABEL);
            assert(status.deferred_binding->status == BINDING_DEFERRED);

            fprintf(stderr, FL_Fmt": ERROR: the %%for block depends on the ambiguous value of a label `"SV_Fmt"` which could be offset by the %%for block itself.\n",
                    FL_Arg(location),
                    SV_Arg(status.deferred_binding->name));
            fprintf(stderr, FL_Fmt": ERROR: the value of label `"SV_Fmt"` is ambiguous, because of the %%for block defined above it.\n",
                    FL_Arg(status.deferred_binding->location),
                    SV_Arg(status.deferred_binding->name));
            fprintf(stderr, "\n    NOTE: To resolve this circular dependency try to define the label before the %%for block that depends on it.\n");
            exit(1);
        }
    }

    Word to = {0};
    {
        Eval_Status status = basm_expr_eval(basm, phor.to, location, &to);
        if (status.kind == EVAL_STATUS_KIND_DEFERRED) {
            assert(status.deferred_binding);
            assert(status.deferred_binding->kind == BINDING_LABEL);
            assert(status.deferred_binding->status == BINDING_DEFERRED);

            fprintf(stderr, FL_Fmt": ERROR: the %%for block depends on the ambiguous value of a label `"SV_Fmt"` which could be offset by the %%for block itself.\n",
                    FL_Arg(location),
                    SV_Arg(status.deferred_binding->name));
            fprintf(stderr, FL_Fmt": ERROR: the value of label `"SV_Fmt"` is ambiguous, because of the %%for block defined above it.\n",
                    FL_Arg(status.deferred_binding->location),
                    SV_Arg(status.deferred_binding->name));
            fprintf(stderr, "\n    NOTE: To resolve this circular dependency try to define the label before the %%for block that depends on it.\n");
            exit(1);
        }
    }

    for (int64_t var_value = from.as_i64;
            var_value <= to.as_i64;
            ++var_value) {
        basm_push_new_scope(basm);
        basm_bind_value(basm, phor.var, word_i64(var_value), BINDING_CONST, location);
        basm_translate_block(basm, phor.body);
        basm_pop_scope(basm);
    }
}

void basm_translate_if(Basm *basm, If_Statement eef, File_Location location)
{
    Word condition = {0};
    Eval_Status status = basm_expr_eval(basm, eef.condition, location, &condition);

    if (status.kind == EVAL_STATUS_KIND_DEFERRED) {
        // TODO(#248): there are no CI tests for compiler errors
        assert(status.deferred_binding);
        assert(status.deferred_binding->kind == BINDING_LABEL);
        assert(status.deferred_binding->status == BINDING_DEFERRED);

        fprintf(stderr, FL_Fmt": ERROR: the %%if block depends on the ambiguous value of a label `"SV_Fmt"` which could be offset by the %%if block itself.\n",
                FL_Arg(location),
                SV_Arg(status.deferred_binding->name));
        fprintf(stderr, FL_Fmt": ERROR: the value of label `"SV_Fmt"` is ambiguous, because of the %%if block defined above it.\n",
                FL_Arg(status.deferred_binding->location),
                SV_Arg(status.deferred_binding->name));
        fprintf(stderr, "\n    NOTE: To resolve this circular dependency try to define the label before the %%if block that depends on it.\n");
        exit(1);
    }

    if (condition.as_u64) {
        basm_push_new_scope(basm);
        basm_translate_block(basm, eef.then);
        basm_pop_scope(basm);
    } else if (eef.elze) {
        basm_push_new_scope(basm);
        basm_translate_block(basm, eef.elze);
        basm_pop_scope(basm);
    }
}

void basm_translate_root_source_file(Basm *basm, String_View input_file_path)
{
    basm_push_new_scope(basm);
    basm_translate_source_file(basm, input_file_path);
    basm_pop_scope(basm);

    basm_eval_deferred_asserts(basm);
    basm_eval_deferred_operands(basm);
    basm_eval_deferred_entry(basm);

    if (!basm->has_entry) {
        fprintf(stderr, SV_Fmt": ERROR: entry point for a BM program is not provided. Use translation directive %%entry to provide the entry point.\n", SV_Arg(input_file_path));
        fprintf(stderr, "  main:\n");
        fprintf(stderr, "     push 69\n");
        fprintf(stderr, "     halt\n");
        fprintf(stderr, "  %%entry main\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "You can also mark an existing label as the entry point like so:\n");
        fprintf(stderr, "  %%entry main:\n");
        fprintf(stderr, "     push 69\n");
        fprintf(stderr, "     halt\n");
        exit(1);
    }
}

void basm_translate_source_file(Basm *basm, String_View input_file_path)
{
    Linizer linizer = {0};
    if (!linizer_from_file(&linizer, &basm->arena, input_file_path)) {
        if (basm->include_level > 0) {
            fprintf(stderr, FL_Fmt": ERROR: could not read file `"SV_Fmt"`: %s\n",
                    FL_Arg(basm->include_location),
                    SV_Arg(input_file_path), strerror(errno));
        } else {
            fprintf(stderr, "ERROR: could not read file `"SV_Fmt"`: %s\n",
                    SV_Arg(input_file_path), strerror(errno));
        }
        exit(1);
    }

    Block_Statement *input_file_block = parse_block_from_lines(&basm->arena, &linizer);
    expect_no_lines(&linizer);
    basm_translate_block(basm, input_file_block);
}

Eval_Status basm_binding_eval(Basm *basm, Binding *binding, Word *output)
{
    switch (binding->status) {
    case BINDING_UNEVALUATED: {
        binding->status = BINDING_EVALUATING;
        Eval_Status status = basm_expr_eval(basm, binding->expr, binding->location, output);
        binding->status = BINDING_EVALUATED;
        binding->value = *output;
        return status;
    }
    break;
    case BINDING_EVALUATING: {
        fprintf(stderr, FL_Fmt": ERROR: cycling binding definition.\n",
                FL_Arg(binding->location));
        exit(1);
    }
    break;
    case BINDING_EVALUATED: {
        *output = binding->value;
        return eval_status_ok();
    }
    break;
    case BINDING_DEFERRED: {
        return eval_status_deferred(binding);
    }
    break;

    default: {
        assert(false && "basm_binding_eval: unreachable");
        exit(1);
    }
    }
}

static Eval_Status basm_binary_op_eval(Basm *basm, Binary_Op *binary_op, File_Location location, Word *output)
{
    Word left = {0};
    {
        Eval_Status status = basm_expr_eval(basm, binary_op->left, location, &left);
        if (status.kind == EVAL_STATUS_KIND_DEFERRED) {
            return status;
        }
    }

    Word right = {0};
    {
        Eval_Status status = basm_expr_eval(basm, binary_op->right, location, &right);
        if (status.kind == EVAL_STATUS_KIND_DEFERRED) {
            return status;
        }
    }

    switch (binary_op->kind) {
    case BINARY_OP_PLUS: {
        // TODO(#183): compile-time sum can only work with integers
        *output = word_u64(left.as_u64 + right.as_u64);
    }
    break;

    case BINARY_OP_MINUS: {
        *output = word_u64(left.as_u64 - right.as_u64);
    }
    break;

    case BINARY_OP_MULT: {
        *output = word_u64(left.as_u64 * right.as_u64);
    }
    break;

    case BINARY_OP_DIV: {
        *output = word_u64(left.as_u64 / right.as_u64);
    }
    break;

    case BINARY_OP_GT: {
        *output = word_u64(left.as_u64 > right.as_u64);
    }
    break;

    case BINARY_OP_LT: {
        *output = word_u64(left.as_u64 < right.as_u64);
    }
    break;

    case BINARY_OP_EQUALS: {
        *output = word_u64(left.as_u64 == right.as_u64);
    }
    break;

    case BINARY_OP_MOD: {
        *output = word_u64(left.as_u64 % right.as_u64);
    }
    break;

    default: {
        assert(false && "basm_binary_op_eval: unreachable");
        exit(1);
    }
    }

    return eval_status_ok();
}

static void funcall_expect_arity(Funcall *funcall, size_t expected_arity, File_Location location)
{
    const size_t actual_arity = funcall_args_len(funcall->args);
    if (actual_arity != expected_arity) {
        fprintf(stderr, FL_Fmt": ERROR: "SV_Fmt"() expects %zu but got %zu",
                FL_Arg(location),
                SV_Arg(funcall->name),
                expected_arity,
                actual_arity);
        exit(1);
    }
}

Eval_Status basm_expr_eval(Basm *basm, Expr expr, File_Location location, Word *output)
{
    switch (expr.kind) {
    case EXPR_KIND_LIT_INT: {
        *output = word_u64(expr.value.as_lit_int);
        return eval_status_ok();
    }
    break;

    case EXPR_KIND_LIT_FLOAT: {
        *output = word_f64(expr.value.as_lit_float);
        return eval_status_ok();
    }
    break;

    case EXPR_KIND_LIT_CHAR: {
        *output = word_u64(lit_char_value(expr.value.as_lit_char));
        return eval_status_ok();
    }
    break;

    case EXPR_KIND_LIT_STR: {
        *output = basm_push_string_to_memory(basm, expr.value.as_lit_str);
        return eval_status_ok();
    }
    break;

    case EXPR_KIND_FUNCALL: {
        if (sv_eq(expr.value.as_funcall->name, sv_from_cstr("len"))) {
            funcall_expect_arity(expr.value.as_funcall, 1, location);

            Word addr = {0};
            {
                Eval_Status status = basm_expr_eval(
                                         basm,
                                         expr.value.as_funcall->args->value,
                                         location,
                                         &addr);
                if (status.kind == EVAL_STATUS_KIND_DEFERRED) {
                    return status;
                }
            }

            Word length = {0};
            if (!basm_string_length_by_addr(basm, addr.as_u64, &length)) {
                fprintf(stderr, FL_Fmt": ERROR: Could not compute the length of string at address %"PRIu64"\n", FL_Arg(location), addr.as_u64);
                exit(1);
            }

            *output = length;
        } else if (sv_eq(expr.value.as_funcall->name, sv_from_cstr("byte_array"))) {
            funcall_expect_arity(expr.value.as_funcall, 2, location);

            Funcall_Arg *args = expr.value.as_funcall->args;
            Word size = {0};
            {
                Eval_Status status = basm_expr_eval(
                                         basm,
                                         args->value,
                                         location,
                                         &size);
                if (status.kind == EVAL_STATUS_KIND_DEFERRED) {
                    return status;
                }
            }
            args = args->next;

            Word value = {0};
            {
                Eval_Status status = basm_expr_eval(basm, args->value, location, &value);
                if (status.kind == EVAL_STATUS_KIND_DEFERRED) {
                    return status;
                }
            }

            *output = basm_push_byte_array_to_memory(basm, size.as_u64, (uint8_t) value.as_u64);
        } else if (sv_eq(expr.value.as_funcall->name, sv_from_cstr("int32"))) {
            Funcall_Arg *args = expr.value.as_funcall->args;

            funcall_expect_arity(expr.value.as_funcall, 1, location);

            Word init_value = {0};
            {
                Eval_Status status = basm_expr_eval(
                                         basm,
                                         args->value,
                                         location,
                                         &init_value);
                if (status.kind == EVAL_STATUS_KIND_DEFERRED) {
                    return status;
                }
            }

            uint32_t byte_array = (uint32_t) init_value.as_u64;
            *output = basm_push_buffer_to_memory(basm, (uint8_t*) &byte_array, sizeof(byte_array));
        } else if (sv_eq(expr.value.as_funcall->name, sv_from_cstr("file"))) {
            funcall_expect_arity(expr.value.as_funcall, 1, location);

            Funcall_Arg *args = expr.value.as_funcall->args;

            if (args->value.kind != EXPR_KIND_LIT_STR) {
                fprintf(stderr, FL_Fmt": ERROR: the first argument of file() is expected to be a string\n",
                        FL_Arg(location));
                exit(1);
            }

            String_View file_path = args->value.value.as_lit_str;
            String_View file_content = {0};
            if (arena_slurp_file(&basm->arena, file_path, &file_content) != 0) {
                fprintf(stderr, FL_Fmt": ERROR: could not read the file `"SV_Fmt"`: %s\n",
                        FL_Arg(location),
                        SV_Arg(file_path),
                        strerror(errno));
                exit(1);
            }

            *output = basm_push_string_to_memory(basm, file_content);
        } else {
            fprintf(stderr,
                    FL_Fmt": ERROR: Unknown translation time function `"SV_Fmt"`\n",
                    FL_Arg(location), SV_Arg(expr.value.as_funcall->name));
            exit(1);
        }
        return eval_status_ok();
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

        return basm_binding_eval(basm, binding, output);
    }
    break;

    case EXPR_KIND_BINARY_OP: {
        return basm_binary_op_eval(basm, expr.value.as_binary_op, location, output);
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

Macrodef *scope_resolve_macrodef(Scope *scope, String_View name)
{
    for (size_t i = 0; i < scope->macrodefs_size; ++i) {
        if (sv_eq(scope->macrodefs[i].name, name)) {
            return &scope->macrodefs[i];
        }
    }

    return NULL;
}

void scope_add_macrodef(Scope *scope, Macrodef macrodef)
{
    Macrodef *existing_macrodef = scope_resolve_macrodef(scope, macrodef.name);

    if (existing_macrodef) {
        fprintf(stderr, FL_Fmt": ERROR: macro with the name `"SV_Fmt"` is already defined\n",
                FL_Arg(macrodef.location), SV_Arg(macrodef.name));
        fprintf(stderr, FL_Fmt": NOTE: the macro is defined here\n",
                FL_Arg(existing_macrodef->location));
        exit(1);
    }

    assert(scope->macrodefs_size < BASM_MACRODEFS_CAPACITY);
    scope->macrodefs[scope->macrodefs_size++] = macrodef;
}

void basm_translate_macrocall_statement(Basm *basm, Macrocall_Statement macrocall, File_Location location)
{
    // TODO(#322): no support for recursive macros
    Macrodef *macrodef = basm_resolve_macrodef(basm, macrocall.name);
    if (!macrodef) {
        fprintf(stderr, FL_Fmt": ERROR: macro `"SV_Fmt"` is not defined\n",
                FL_Arg(location), SV_Arg(macrocall.name));
        exit(1);
    }

    Scope *args_scope = arena_alloc(&basm->arena, sizeof(*args_scope));

    Funcall_Arg *call_args = macrocall.args;
    Fundef_Arg *def_args = macrodef->args;

    size_t arity = 0;
    while (call_args && def_args) {
        Word value = {0};
        Eval_Status status = basm_expr_eval(basm, call_args->value, location, &value);
        if (status.kind == EVAL_STATUS_KIND_DEFERRED) {
            assert(status.deferred_binding->kind == BINDING_LABEL);

            fprintf(stderr, FL_Fmt": ERROR: the macro call `"SV_Fmt"` depends on the ambiguous value of a label `"SV_Fmt"` which could be offset by the macro call itself when it's expanded.\n",
                    FL_Arg(location),
                    SV_Arg(macrocall.name),
                    SV_Arg(status.deferred_binding->name));
            fprintf(stderr, FL_Fmt": ERROR: the value of label `"SV_Fmt"` is ambiguous, because of the macro call `"SV_Fmt"` defined above it.\n",
                    FL_Arg(status.deferred_binding->location),
                    SV_Arg(status.deferred_binding->name),
                    SV_Arg(macrocall.name));
            fprintf(stderr, "\n    NOTE: To resolve this circular dependency try to define the label before the macro call that depends on it.\n");
            exit(1);
        }
        assert(status.kind == EVAL_STATUS_KIND_OK);

        scope_bind_value(args_scope,
                         def_args->name,
                         value,
                         BINDING_CONST,
                         macrodef->location);
        call_args = call_args->next;
        def_args = def_args->next;
        arity += 1;
    }

    if (call_args || def_args) {
        size_t call_arity = arity;
        while (call_args) {
            call_args = call_args->next;
            call_arity += 1;
        }

        size_t def_arity = arity;
        while (def_args) {
            def_args = def_args->next;
            def_arity += 1;
        }

        fprintf(stderr, FL_Fmt": ERROR: provided %zu arguments to the `"SV_Fmt"` macro call\n",
                FL_Arg(location), call_arity, SV_Arg(macrocall.name));
        fprintf(stderr, FL_Fmt": ERROR: but the macro definition of `"SV_Fmt"` expected %zu arguments\n",
                FL_Arg(macrodef->location), SV_Arg(macrodef->name), def_arity);
        exit(1);
    }

    Scope *saved_scope = basm->scope;
    basm->scope = macrodef->scope;
    basm_push_scope(basm, args_scope);
    basm_translate_block(basm, macrodef->body);
    basm->scope = saved_scope;
}

void basm_translate_macrodef_statement(Basm *basm, Macrodef_Statement macrodef_statement, File_Location location)
{
    Macrodef macrodef = {0};
    macrodef.name = macrodef_statement.name;
    macrodef.args = macrodef_statement.args;
    macrodef.body = macrodef_statement.body;
    macrodef.location = location;
    macrodef.scope = basm->scope;
    scope_add_macrodef(basm->scope, macrodef);
}

Macrodef *basm_resolve_macrodef(Basm *basm, String_View name)
{
    for (Scope *scope = basm->scope;
            scope != NULL;
            scope = scope->previous) {
        Macrodef *macrodef = scope_resolve_macrodef(scope, name);
        if (macrodef) {
            return macrodef;
        }
    }

    return NULL;
}
