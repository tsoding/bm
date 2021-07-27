#ifndef BASM_H_
#define BASM_H_

#include "./sv.h"
#include "./arena.h"
#include "./bm.h"
#include "./expr.h"
#include "./statement.h"
#include "./types.h"
#include "./target.h"

#define BASM_BINDINGS_CAPACITY 1024
#define BASM_MACRODEFS_CAPACITY 1024
#define BASM_DEFERRED_OPERANDS_CAPACITY 1024
#define BASM_DEFERRED_ASSERTS_CAPACITY 1024
#define BASM_STRING_LENGTHS_CAPACITY 1024
#define BASM_MAX_INCLUDE_LEVEL 69
#define BASM_INCLUDE_PATHS_CAPACITY 1024

typedef enum {
    OS_TARGET_LINUX = 0,
    OS_TARGET_FREEBSD,
    OS_TARGET_WINDOWS,
    OS_TARGET_MACOS,
} OS_Target;

typedef enum {
    BINDING_UNEVALUATED = 0,
    BINDING_EVALUATING,
    BINDING_EVALUATED,
    BINDING_DEFERRED,
} Binding_Status;

const char *binding_status_as_cstr(Binding_Status status);

typedef struct {
    Type type;
    String_View name;
    Word value;
    Expr expr;
    Binding_Status status;
    File_Location location;
} Binding;

typedef struct {
    Inst_Addr addr;
    uint64_t length;
} String_Length;

typedef enum {
    EVAL_STATUS_OK = 0,
    EVAL_STATUS_DEFERRED
} Eval_Status;

typedef struct {
    Eval_Status status;
    Binding *deferred_binding;
    Word value;
    Type type;
} Eval_Result;

Eval_Result eval_result_ok(Word value, Type type);
Eval_Result eval_result_deferred(Binding *deferred_binding);

typedef struct Scope Scope;

typedef struct {
    String_View name;
    Fundef_Arg *args;
    Block_Statement *body;
    File_Location location;
    Scope *scope;
} Macrodef;

struct Scope {
    Scope *previous;

    Binding bindings[BASM_BINDINGS_CAPACITY];
    size_t bindings_size;

    Macrodef macrodefs[BASM_MACRODEFS_CAPACITY];
    size_t macrodefs_size;
};

typedef struct {
    Inst_Addr addr;
    Expr expr;
    File_Location location;
    Scope *scope;
} Deferred_Operand;

typedef struct {
    Expr expr;
    File_Location location;
    Scope *scope;
} Deferred_Assert;

typedef struct {
    String_View binding_name;
    File_Location location;
    Scope *scope;
} Deferred_Entry;

typedef struct {
    Scope *scope;
    Scope *global_scope;

    Inst program[BM_PROGRAM_CAPACITY];
    File_Location program_locations[BM_PROGRAM_CAPACITY];
    Type program_operand_types[BM_PROGRAM_CAPACITY];
    uint64_t program_size;

    Deferred_Operand deferred_operands[BASM_DEFERRED_OPERANDS_CAPACITY];
    size_t deferred_operands_size;

    Deferred_Assert deferred_asserts[BASM_DEFERRED_ASSERTS_CAPACITY];
    size_t deferred_asserts_size;

    Deferred_Entry deferred_entry;

    Inst_Addr entry;
    bool has_entry;
    File_Location entry_location;

    String_Length string_lengths[BASM_STRING_LENGTHS_CAPACITY];
    size_t string_lengths_size;

    uint8_t memory[BM_MEMORY_CAPACITY];
    size_t memory_base;
    size_t memory_size;
    size_t memory_capacity;

    External_Native external_natives[BM_EXTERNAL_NATIVES_CAPACITY];
    size_t external_natives_size;

    Arena arena;

    size_t include_level;
    File_Location include_location;

    String_View include_paths[BASM_INCLUDE_PATHS_CAPACITY];
    size_t include_paths_size;
} Basm;

Macrodef *scope_resolve_macrodef(Scope *scope, String_View name);
void scope_add_macrodef(Scope *scope, Macrodef macrodef);

Binding *scope_resolve_binding(Scope *scope, String_View name);
void scope_bind_value(Scope *scope, String_View name, Word value, Type type, File_Location location);
void scope_defer_binding(Scope *scope, String_View name, Type type, File_Location location);
void scope_bind_expr(Scope *scope, String_View name, Expr expr, File_Location location);

void basm_push_scope(Basm *basm, Scope *scope);
void basm_push_new_scope(Basm *basm);
void basm_pop_scope(Basm *basm);

void basm_eval_deferred_asserts(Basm *basm);
void basm_eval_deferred_operands(Basm *basm);
void basm_eval_deferred_entry(Basm *basm);
Binding *basm_resolve_binding(Basm *basm, String_View name);
void basm_defer_binding(Basm *basm, String_View name, Type type, File_Location location);
void basm_bind_expr(Basm *basm, String_View name, Expr expr, File_Location location);
void basm_bind_value(Basm *basm, String_View name, Word value, Type type, File_Location location);
void basm_push_deferred_operand(Basm *basm, Inst_Addr addr, Expr expr, File_Location location);
void basm_save_to_bm(const Basm *basm, Bm *bm);
void basm_save_to_file_as_target(Basm *basm, const char *output_file_path, Target target);
void basm_save_to_file_as_bm(Basm *basm, const char *output_file_path);
void basm_save_to_file_as_nasm_sysv_x86_64(Basm *basm, OS_Target os_target, const char *output_file_path);
void basm_save_to_file_as_gas_arm64(Basm *basm, OS_Target os_target, const char *output_file_path);
Word basm_push_string_to_memory(Basm *basm, String_View sv);
Word basm_push_byte_array_to_memory(Basm *basm, uint64_t size, uint8_t value);
Word basm_push_buffer_to_memory(Basm *basm, uint8_t *buffer, uint64_t buffer_size);
bool basm_string_length_by_addr(Basm *basm, Inst_Addr addr, Word *length);
Eval_Result basm_expr_eval(Basm *basm, Expr expr, File_Location location);
Eval_Result basm_binding_eval(Basm *basm, Binding *binding);
void basm_push_include_path(Basm *basm, String_View path);
bool basm_resolve_include_file_path(Basm *basm,
                                    String_View file_path,
                                    String_View *resolved_path);
Macrodef *basm_resolve_macrodef(Basm *basm, String_View name);
void basm_translate_macrocall_statement(Basm *basm, Macrocall_Statement macrocall, File_Location location);
void basm_translate_macrodef_statement(Basm *basm, Macrodef_Statement macrodef, File_Location location);
void basm_translate_base_statement(Basm *basm, Base_Statement base_statement, File_Location location);
void basm_translate_block_statement(Basm *basm, Block_Statement *block);
void basm_translate_const_statement(Basm *basm, Const_Statement konst, File_Location location);
void basm_translate_native_statement(Basm *basm, Native_Statement native, File_Location location);
void basm_translate_if_statement(Basm *basm, If_Statement eef, File_Location location);
void basm_translate_include_statement(Basm *basm, Include_Statement include, File_Location location);
void basm_translate_assert_statement(Basm *basm, Assert_Statement azzert, File_Location location);
void basm_translate_error_statement(Error_Statement error, File_Location location);
void basm_translate_entry_statement(Basm *basm, Entry_Statement entry, File_Location location);
void basm_translate_emit_inst_statement(Basm *basm, Emit_Inst_Statement emit_inst, File_Location location);
void basm_translate_for_statement(Basm *basm, For_Statement phor, File_Location location);
void basm_translate_source_file(Basm *basm, String_View input_file_path);
void basm_translate_root_source_file(Basm *basm, String_View input_file_path);
void funcall_expect_arity(Funcall *funcall, size_t expected_arity, File_Location location);
Native_ID basm_push_external_native(Basm *basm, String_View native_name);
Inst_Addr basm_push_inst(Basm *basm, Inst_Type inst_type, Word inst_operand);

#endif // BASM_H_
