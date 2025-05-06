#ifndef __DVM_H_
#define __DVM_H_

#include <stdint.h>
#include <stdlib.h>

#define STACK_SIZE 4096
#define CALL_STACK_SIZE 64
#define HEAP_SIZE 4096 * 4096
#define GLOBALS_SIZE 4096
#define MAX_NATIVE_FUNCS 4096
#define MAX_ERROR_MSG_LEN 256
#define MAX_STRING_LENGTH 1024
#define GC_THRESHOLD ( HEAP_SIZE * 3 / 4 )

typedef enum
{
	OP_NOP,
	OP_HALT,
	OP_JMP,
	OP_JZ,
	OP_JNZ,
	OP_PUSH_INT,
	OP_PUSH_FLOAT,
	OP_PUSH_STRING,
	OP_POP,
	OP_DUP,
	OP_SWAP,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_EQ,
	OP_NEQ,
	OP_GT,
	OP_LT,
	OP_GTE,
	OP_LTE,
	OP_AND,
	OP_OR,
	OP_NOT,
	OP_LOAD,
	OP_STORE,
	OP_CALL,
	OP_RET,
	OP_CALL_NATIVE,
	OP_ALLOC,
	OP_SETFIELD,
	OP_GETFIELD,
	OP_LOAD_GLOBAL,
	OP_STORE_GLOBAL,
	OP_PUSH_ARG,
	OP_SET_ARG
} OpCode;

typedef enum
{
	VAL_INT,
	VAL_FLOAT,
	VAL_PTR,
	VAL_STRING,
	VAL_BOOL,
	VAL_NULL
} ValueType;

typedef struct
{
	ValueType type;
	union
	{
		int32_t i;
		float   f;
		void   *ptr;
		char   *str;
		int     boolean;
	} data;
} Value;

typedef struct
{
	size_t    size;
	uint8_t   marked;
	ValueType type;
} HeapObject;

struct VM;

typedef void ( *NativeFn )( struct VM * );

typedef struct
{
	int return_ip;
	int base_pointer;
	int arg_count;
} CallFrame;

typedef struct VM
{
	int ip;
	int sp;

	Value stack[STACK_SIZE];
	Value globals[GLOBALS_SIZE];

	CallFrame call_stack[CALL_STACK_SIZE];
	int       call_sp;
	int      *code;

	uint8_t heap[HEAP_SIZE];
	int     heap_used;

	NativeFn native_functions[MAX_NATIVE_FUNCS];

	int  running;
	char error_msg[MAX_ERROR_MSG_LEN];

	char **string_table;
	int    string_table_size;

	const char **op_names;
} VM;

VM *vm_create();

void vm_destroy( VM *vm );

void vm_reset( VM *vm );

void vm_register_native( VM *vm, int id, NativeFn fn );

void vm_error( VM *vm, const char *fmt, ... );

int check_stack( VM *vm, int required );

Value make_int( int val );

Value make_float( float val );

Value make_bool( int val );

Value make_null();

Value make_ptr( void *ptr );

Value make_string( VM *vm, const char *str );

const char *get_string_from_table( VM *vm, int index );

const char *pop_string_from_table( VM *vm );

void *gc_alloc( VM *vm, size_t size, ValueType type );

void gc_mark_stack( VM *vm );

void gc_mark_globals( VM *vm );

void gc_sweep( VM *vm );

void gc_mark_and_sweep( VM *vm );

int vm_run( VM *vm );

int vm_load_bytecode( VM *vm, const char *filename );

#endif // __DVM_H_