#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"

VM *vm_create()
{
	VM *vm = (VM *)malloc( sizeof( VM ) );
	memset( vm, 0, sizeof( VM ) );
	vm->sp        = -1;
	vm->call_sp   = 0;
	vm->heap_used = 0;
	vm->running   = 0;

	for ( int i = 0; i < STACK_SIZE; i++ )
	{
		vm->stack[i] = make_null();
	}

	for ( int i = 0; i < GLOBALS_SIZE; i++ )
	{
		vm->globals[i] = make_null();
	}

	vm->op_names = (const char **)malloc( sizeof( char * ) * 37 );

	vm->op_names[OP_HALT]         = "HALT";
	vm->op_names[OP_NOP]          = "NOP";
	vm->op_names[OP_PUSH_INT]     = "PUSH_INT";
	vm->op_names[OP_PUSH_FLOAT]   = "PUSH_FLOAT";
	vm->op_names[OP_PUSH_STRING]  = "PUSH_STRING";
	vm->op_names[OP_POP]          = "POP";
	vm->op_names[OP_DUP]          = "DUP";
	vm->op_names[OP_SWAP]         = "SWAP";
	vm->op_names[OP_ADD]          = "ADD";
	vm->op_names[OP_SUB]          = "SUB";
	vm->op_names[OP_MUL]          = "MUL";
	vm->op_names[OP_DIV]          = "DIV";
	vm->op_names[OP_MOD]          = "MOD";
	vm->op_names[OP_EQ]           = "EQ";
	vm->op_names[OP_NEQ]          = "NEQ";
	vm->op_names[OP_GT]           = "GT";
	vm->op_names[OP_LT]           = "LT";
	vm->op_names[OP_GTE]          = "GTE";
	vm->op_names[OP_LTE]          = "LTE";
	vm->op_names[OP_AND]          = "AND";
	vm->op_names[OP_OR]           = "OR";
	vm->op_names[OP_NOT]          = "NOT";
	vm->op_names[OP_JMP]          = "JMP";
	vm->op_names[OP_JZ]           = "JZ";
	vm->op_names[OP_JNZ]          = "JNZ";
	vm->op_names[OP_LOAD]         = "LOAD";
	vm->op_names[OP_STORE]        = "STORE";
	vm->op_names[OP_LOAD_GLOBAL]  = "LOAD_GLOBAL";
	vm->op_names[OP_STORE_GLOBAL] = "STORE_GLOBAL";
	vm->op_names[OP_CALL]         = "CALL";
	vm->op_names[OP_RET]          = "RET";
	vm->op_names[OP_CALL_NATIVE]  = "CALL_NATIVE";
	vm->op_names[OP_ALLOC]        = "ALLOC";
	vm->op_names[OP_SETFIELD]     = "SETFIELD";
	vm->op_names[OP_GETFIELD]     = "GETFIELD";
	vm->op_names[OP_PUSH_ARG]     = "PUSH_ARG";
	vm->op_names[OP_SET_ARG]      = "SET_ARG";

	vm->string_table      = NULL;
	vm->string_table_size = 0;

	return vm;
}

void vm_destroy( VM *vm )
{
	free( vm );
}

void vm_reset( VM *vm )
{
	vm->sp      = 0;
	vm->ip      = 0;
	vm->running = 1;
}

void vm_register_native( VM *vm, int id, NativeFn fn )
{
	if ( id >= 0 && id < MAX_NATIVE_FUNCS )
	{
		vm->native_functions[id] = fn;
	}
	else
	{
		vm_error( vm, "Native function ID out of range: %d", id );
	}
}

void vm_error( VM *vm, const char *fmt, ... )
{
	va_list args;
	va_start( args, fmt );
	vsnprintf( vm->error_msg, MAX_ERROR_MSG_LEN, fmt, args );
	va_end( args );

	printf( "VM Error: %s\n", vm->error_msg );
	vm->running = 0;
}

int vm_check_stack( VM *vm, int required )
{
	if ( vm->sp + required >= STACK_SIZE )
	{
		vm_error( vm, "Stack overflow" );
		return 0;
	}
	if ( vm->sp + 1 - required < 0 )
	{
		vm_error( vm, "Stack underflow" );
		return 0;
	}
	return 1;
}

Value make_int( int val )
{
	Value v;
	v.type   = VAL_INT;
	v.data.i = val;
	return v;
}

Value make_float( float val )
{
	Value v;
	v.type   = VAL_FLOAT;
	v.data.f = val;
	return v;
}

Value make_bool( int val )
{
	Value v;
	v.type         = VAL_BOOL;
	v.data.boolean = val ? 1 : 0;
	return v;
}

Value make_null()
{
	Value v;
	v.type     = VAL_NULL;
	v.data.ptr = NULL;
	return v;
}

Value make_ptr( void *ptr )
{
	Value v;
	v.type     = VAL_PTR;
	v.data.ptr = ptr;
	return v;
}

Value make_string( VM *vm, const char *str )
{
	size_t len = strlen( str ) + 1;
	if ( len > MAX_STRING_LENGTH )
	{
		vm_error( vm, "String too long" );
		return make_null();
	}

	char *heap_str = gc_alloc( vm, len, VAL_STRING );
	if ( heap_str )
	{
		strcpy( heap_str, str );
		Value v;
		v.type     = VAL_STRING;
		v.data.str = heap_str;
		return v;
	}

	return make_null();
}

const char *get_string_from_table( VM *vm, int index )
{
	if ( index >= 0 && index < vm->string_table_size )
	{
		return vm->string_table[index];
	}
	return NULL;
}

const char *pop_string_from_table( VM *vm )
{
	if ( vm->string_table_size <= 0 )
	{
		return NULL;
	}

	return vm->string_table[--vm->string_table_size];
}

void *gc_alloc( VM *vm, size_t size, ValueType type )
{
	size_t total_size = size + sizeof( HeapObject );
	if ( vm->heap_used + total_size > GC_THRESHOLD )
	{
		gc_mark_and_sweep( vm );
	}

	if ( vm->heap_used + total_size > HEAP_SIZE )
	{
		vm_error( vm, "Out of memory" );
		return NULL;
	}

	HeapObject *obj = (HeapObject *)&vm->heap[vm->heap_used];
	obj->size       = size;
	obj->marked     = 0;
	obj->type       = type;

	void *data = (void *)( (uint8_t *)obj + sizeof( HeapObject ) );
	vm->heap_used += total_size;

	return data;
}

void gc_mark_stack( VM *vm )
{
	for ( int i = 0; i <= vm->sp; i++ )
	{
		Value val = vm->stack[i];
		if ( val.type == VAL_PTR || val.type == VAL_STRING )
		{
			if ( val.data.ptr != NULL )
			{
				HeapObject *obj = (HeapObject *)( (uint8_t *)val.data.ptr - sizeof( HeapObject ) );
				obj->marked     = 1;
			}
		}
	}
}

void gc_mark_globals( VM *vm )
{
	for ( int i = 0; i < GLOBALS_SIZE; i++ )
	{
		Value val = vm->globals[i];
		if ( val.type == VAL_PTR || val.type == VAL_STRING )
		{
			if ( val.data.ptr != NULL )
			{
				HeapObject *obj = (HeapObject *)( (uint8_t *)val.data.ptr - sizeof( HeapObject ) );
				obj->marked     = 1;
			}
		}
	}
}

void gc_sweep( VM *vm )
{
	uint8_t *new_heap      = (uint8_t *)malloc( HEAP_SIZE );
	size_t   new_heap_used = 0;

	uint8_t *ptr = vm->heap;
	while ( ptr < vm->heap + vm->heap_used )
	{
		HeapObject *obj        = (HeapObject *)ptr;
		size_t      total_size = obj->size + sizeof( HeapObject );

		if ( obj->marked )
		{
			memcpy( new_heap + new_heap_used, ptr, total_size );
			obj->marked = 0;
			new_heap_used += total_size;
		}

		ptr += total_size;
	}

	for ( int i = 0; i <= vm->sp; i++ )
	{
		Value *val = &vm->stack[i];
		if ( val->type == VAL_PTR || val->type == VAL_STRING )
		{
			if ( val->data.ptr != NULL )
			{
				uint8_t *old_ptr = (uint8_t *)val->data.ptr;
				size_t   offset  = ( old_ptr - vm->heap ) - sizeof( HeapObject );
				val->data.ptr    = new_heap + offset + sizeof( HeapObject );
			}
		}
	}

	for ( int i = 0; i < GLOBALS_SIZE; i++ )
	{
		Value *val = &vm->globals[i];
		if ( val->type == VAL_PTR || val->type == VAL_STRING )
		{
			if ( val->data.ptr != NULL )
			{
				uint8_t *old_ptr = (uint8_t *)val->data.ptr;
				size_t   offset  = ( old_ptr - vm->heap ) - sizeof( HeapObject );
				val->data.ptr    = new_heap + offset + sizeof( HeapObject );
			}
		}
	}

#if 0
	free(vm->heap);
	vm->heap[0] = new_heap;
	vm->heap_used = new_heap_used;
#endif
}

void gc_mark_and_sweep( VM *vm )
{
	gc_mark_stack( vm );
	gc_mark_globals( vm );
	gc_sweep( vm );
}

int vm_run( VM *vm )
{
	vm->running = 1;
	while ( vm->running )
	{
		int opcode = vm->code[vm->ip++];
		switch ( opcode )
		{
		case OP_HALT:
			vm->running = 0;
			break;

		case OP_NOP:
			break;

		case OP_PUSH_INT:
			if ( !vm_check_stack( vm, -1 ) )
				return 0;
			vm->stack[++vm->sp] = make_int( vm->code[vm->ip++] );
			break;

		case OP_PUSH_FLOAT:
		{
			if ( !vm_check_stack( vm, -1 ) )
			{
				return 0;
			}
			int   raw_bytes     = vm->code[vm->ip++];
			float fval          = *( (float *)&raw_bytes );
			vm->stack[++vm->sp] = make_float( fval );
			break;
		}

		case OP_PUSH_STRING:
		{
			if ( !vm_check_stack( vm, -1 ) )
			{
				return 0;
			}

			int         str_index = vm->code[vm->ip++];
			const char *str       = get_string_from_table( vm, str_index );
			if ( str == NULL )
			{
				vm_error( vm, "Invalid string table index: %d", str_index );
				return 0;
			}

			vm->stack[++vm->sp] = make_string( vm, str );
		}
		break;

		case OP_POP:
			if ( !vm_check_stack( vm, 1 ) )
			{
				return 0;
			}
			vm->sp--;
			break;

		case OP_DUP:
			if ( !vm_check_stack( vm, -1 ) )
			{
				return 0;
			}
			vm->stack[vm->sp + 1] = vm->stack[vm->sp];
			vm->sp++;
			break;

		case OP_SWAP:
		{
			if ( !vm_check_stack( vm, 2 ) )
			{
				return 0;
			}
			Value a               = vm->stack[vm->sp];
			Value b               = vm->stack[vm->sp - 1];
			vm->stack[vm->sp]     = b;
			vm->stack[vm->sp - 1] = a;
			break;
		}

		case OP_ADD:
		{
			if ( !vm_check_stack( vm, 2 ) )
			{
				return 0;
			}
			Value b = vm->stack[vm->sp--];
			Value a = vm->stack[vm->sp];

			if ( a.type == VAL_INT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_int( a.data.i + b.data.i );
			}
			else if ( a.type == VAL_FLOAT && b.type == VAL_FLOAT )
			{
				vm->stack[vm->sp] = make_float( a.data.f + b.data.f );
			}
			else if ( a.type == VAL_INT && b.type == VAL_FLOAT )
			{
				vm->stack[vm->sp] = make_float( a.data.i + b.data.f );
			}
			else if ( a.type == VAL_FLOAT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_float( a.data.f + b.data.i );
			}
			else if ( a.type == VAL_STRING && b.type == VAL_STRING )
			{
				size_t len_a  = strlen( a.data.str );
				size_t len_b  = strlen( b.data.str );
				char  *result = gc_alloc( vm, len_a + len_b + 1, VAL_STRING );
				if ( result )
				{
					strcpy( result, a.data.str );
					strcat( result, b.data.str );
					vm->stack[vm->sp].data.str = result;
				}
				else
				{
					vm_error( vm, "String concatenation failed" );
					return 0;
				}
			}
			else
			{
				vm_error( vm, "Type mismatch for ADD operation" );
				return 0;
			}
			break;
		}

		case OP_SUB:
		{
			if ( !vm_check_stack( vm, 2 ) )
			{
				return 0;
			}
			Value b = vm->stack[vm->sp--];
			Value a = vm->stack[vm->sp];

			if ( a.type == VAL_INT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_int( a.data.i - b.data.i );
			}
			else if ( a.type == VAL_FLOAT && b.type == VAL_FLOAT )
			{
				vm->stack[vm->sp] = make_float( a.data.f - b.data.f );
			}
			else if ( a.type == VAL_INT && b.type == VAL_FLOAT )
			{
				vm->stack[vm->sp] = make_float( a.data.i - b.data.f );
			}
			else if ( a.type == VAL_FLOAT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_float( a.data.f - b.data.i );
			}
			else
			{
				vm_error( vm, "Type mismatch for SUB operation" );
				return 0;
			}
			break;
		}

		case OP_MUL:
		{
			if ( !vm_check_stack( vm, 2 ) )
			{
				return 0;
			}
			Value b = vm->stack[vm->sp--];
			Value a = vm->stack[vm->sp];

			if ( a.type == VAL_INT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_int( a.data.i * b.data.i );
			}
			else if ( a.type == VAL_FLOAT && b.type == VAL_FLOAT )
			{
				vm->stack[vm->sp] = make_float( a.data.f * b.data.f );
			}
			else if ( a.type == VAL_INT && b.type == VAL_FLOAT )
			{
				vm->stack[vm->sp] = make_float( a.data.i * b.data.f );
			}
			else if ( a.type == VAL_FLOAT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_float( a.data.f * b.data.i );
			}
			else
			{
				vm_error( vm, "Type mismatch for MUL operation" );
				return 0;
			}
			break;
		}

		case OP_DIV:
		{
			if ( !vm_check_stack( vm, 2 ) )
			{
				return 0;
			}
			Value b = vm->stack[vm->sp--];
			Value a = vm->stack[vm->sp];
			if ( ( b.type == VAL_INT && b.data.i == 0 ) || ( b.type == VAL_FLOAT && b.data.f == 0.0f ) )
			{
				vm_error( vm, "Division by zero" );
				return 0;
			}

			if ( a.type == VAL_INT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_int( a.data.i / b.data.i );
			}
			else if ( a.type == VAL_FLOAT && b.type == VAL_FLOAT )
			{
				vm->stack[vm->sp] = make_float( a.data.f / b.data.f );
			}
			else if ( a.type == VAL_INT && b.type == VAL_FLOAT )
			{
				vm->stack[vm->sp] = make_float( a.data.i / b.data.f );
			}
			else if ( a.type == VAL_FLOAT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_float( a.data.f / b.data.i );
			}
			else
			{
				vm_error( vm, "Type mismatch for DIV operation" );
				return 0;
			}
			break;
		}

		case OP_MOD:
		{
			if ( !vm_check_stack( vm, 2 ) )
			{
				return 0;
			}
			Value b = vm->stack[vm->sp--];
			Value a = vm->stack[vm->sp];

			if ( a.type == VAL_INT && b.type == VAL_INT )
			{
				if ( b.data.i == 0 )
				{
					vm_error( vm, "Modulo by zero" );
					return 0;
				}
				vm->stack[vm->sp] = make_int( a.data.i % b.data.i );
			}
			else
			{
				vm_error( vm, "MOD operation only supports integers" );
				return 0;
			}
			break;
		}

		case OP_EQ:
		{
			if ( !vm_check_stack( vm, 2 ) )
			{
				return 0;
			}
			Value b = vm->stack[vm->sp--];
			Value a = vm->stack[vm->sp];

			if ( a.type != b.type )
			{
				vm->stack[vm->sp] = make_bool( 0 );
			}
			else
			{
				switch ( a.type )
				{
				case VAL_INT:
					vm->stack[vm->sp] = make_bool( a.data.i == b.data.i );
					break;
				case VAL_FLOAT:
					vm->stack[vm->sp] = make_bool( a.data.f == b.data.f );
					break;
				case VAL_PTR:
					vm->stack[vm->sp] = make_bool( a.data.ptr == b.data.ptr );
					break;
				case VAL_STRING:
					vm->stack[vm->sp] = make_bool( strcmp( a.data.str, b.data.str ) == 0 );
					break;
				case VAL_BOOL:
					vm->stack[vm->sp] = make_bool( a.data.boolean == b.data.boolean );
					break;
				case VAL_NULL:
					vm->stack[vm->sp] = make_bool( 1 ); // null == null
					break;
				}
			}
			break;
		}

		case OP_NEQ:
		{
			if ( !vm_check_stack( vm, 2 ) )
				return 0;
			Value b = vm->stack[vm->sp--];
			Value a = vm->stack[vm->sp];

			if ( a.type != b.type )
			{
				vm->stack[vm->sp] = make_bool( 1 );
			}
			else
			{
				switch ( a.type )
				{
				case VAL_INT:
					vm->stack[vm->sp] = make_bool( a.data.i != b.data.i );
					break;
				case VAL_FLOAT:
					vm->stack[vm->sp] = make_bool( a.data.f != b.data.f );
					break;
				case VAL_PTR:
					vm->stack[vm->sp] = make_bool( a.data.ptr != b.data.ptr );
					break;
				case VAL_STRING:
					vm->stack[vm->sp] = make_bool( strcmp( a.data.str, b.data.str ) != 0 );
					break;
				case VAL_BOOL:
					vm->stack[vm->sp] = make_bool( a.data.boolean != b.data.boolean );
					break;
				case VAL_NULL:
					vm->stack[vm->sp] = make_bool( 0 ); // null != null is false
					break;
				}
			}
			break;
		}

		case OP_GT:
		{
			if ( !vm_check_stack( vm, 2 ) )
				return 0;
			Value b = vm->stack[vm->sp--];
			Value a = vm->stack[vm->sp];

			if ( a.type == VAL_INT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_bool( a.data.i > b.data.i );
			}
			else if ( a.type == VAL_FLOAT && b.type == VAL_FLOAT )
			{
				vm->stack[vm->sp] = make_bool( a.data.f > b.data.f );
			}
			else if ( a.type == VAL_INT && b.type == VAL_FLOAT )
			{
				vm->stack[vm->sp] = make_bool( a.data.i > b.data.f );
			}
			else if ( a.type == VAL_FLOAT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_bool( a.data.f > b.data.i );
			}
			else if ( a.type == VAL_STRING && b.type == VAL_STRING )
			{
				vm->stack[vm->sp] = make_bool( strcmp( a.data.str, b.data.str ) > 0 );
			}
			else
			{
				vm_error( vm, "Type mismatch for GT operation" );
				return 0;
			}
			break;
		}

		case OP_LT:
		{
			if ( !vm_check_stack( vm, 2 ) )
				return 0;
			Value b = vm->stack[vm->sp--];
			Value a = vm->stack[vm->sp];

			if ( a.type == VAL_INT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_bool( a.data.i < b.data.i );
			}
			else if ( a.type == VAL_FLOAT && b.type == VAL_FLOAT )
			{
				vm->stack[vm->sp] = make_bool( a.data.f < b.data.f );
			}
			else if ( a.type == VAL_INT && b.type == VAL_FLOAT )
			{
				vm->stack[vm->sp] = make_bool( a.data.i < b.data.f );
			}
			else if ( a.type == VAL_FLOAT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_bool( a.data.f < b.data.i );
			}
			else if ( a.type == VAL_STRING && b.type == VAL_STRING )
			{
				vm->stack[vm->sp] = make_bool( strcmp( a.data.str, b.data.str ) < 0 );
			}
			else
			{
				vm_error( vm, "Type mismatch for LT operation" );
				return 0;
			}
			break;
		}

		case OP_GTE:
		{
			if ( !vm_check_stack( vm, 2 ) )
				return 0;
			Value b = vm->stack[vm->sp--];
			Value a = vm->stack[vm->sp];

			if ( a.type == VAL_INT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_bool( a.data.i >= b.data.i );
			}
			else if ( a.type == VAL_FLOAT && b.type == VAL_FLOAT )
			{
				vm->stack[vm->sp] = make_bool( a.data.f >= b.data.f );
			}
			else if ( a.type == VAL_INT && b.type == VAL_FLOAT )
			{
				vm->stack[vm->sp] = make_bool( a.data.i >= b.data.f );
			}
			else if ( a.type == VAL_FLOAT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_bool( a.data.f >= b.data.i );
			}
			else if ( a.type == VAL_STRING && b.type == VAL_STRING )
			{
				vm->stack[vm->sp] = make_bool( strcmp( a.data.str, b.data.str ) >= 0 );
			}
			else
			{
				vm_error( vm, "Type mismatch for GTE operation" );
				return 0;
			}
			break;
		}

		case OP_LTE:
		{
			if ( !vm_check_stack( vm, 2 ) )
				return 0;
			Value b = vm->stack[vm->sp--];
			Value a = vm->stack[vm->sp];

			if ( a.type == VAL_INT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_bool( a.data.i <= b.data.i );
			}
			else if ( a.type == VAL_FLOAT && b.type == VAL_FLOAT )
			{
				vm->stack[vm->sp] = make_bool( a.data.f <= b.data.f );
			}
			else if ( a.type == VAL_INT && b.type == VAL_FLOAT )
			{
				vm->stack[vm->sp] = make_bool( a.data.i <= b.data.f );
			}
			else if ( a.type == VAL_FLOAT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_bool( a.data.f <= b.data.i );
			}
			else if ( a.type == VAL_STRING && b.type == VAL_STRING )
			{
				vm->stack[vm->sp] = make_bool( strcmp( a.data.str, b.data.str ) <= 0 );
			}
			else
			{
				vm_error( vm, "Type mismatch for LTE operation" );
				return 0;
			}
			break;
		}

		case OP_AND:
		{
			if ( !vm_check_stack( vm, 2 ) )
				return 0;
			Value b = vm->stack[vm->sp--];
			Value a = vm->stack[vm->sp];

			if ( a.type == VAL_BOOL && b.type == VAL_BOOL )
			{
				vm->stack[vm->sp] = make_bool( a.data.boolean && b.data.boolean );
			}
			else if ( a.type == VAL_INT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_bool( a.data.i && b.data.i );
			}
			else
			{
				vm_error( vm, "Type mismatch for AND operation" );
				return 0;
			}
			break;
		}

		case OP_OR:
		{
			if ( !vm_check_stack( vm, 2 ) )
				return 0;
			Value b = vm->stack[vm->sp--];
			Value a = vm->stack[vm->sp];

			if ( a.type == VAL_BOOL && b.type == VAL_BOOL )
			{
				vm->stack[vm->sp] = make_bool( a.data.boolean || b.data.boolean );
			}
			else if ( a.type == VAL_INT && b.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_bool( a.data.i || b.data.i );
			}
			else
			{
				vm_error( vm, "Type mismatch for OR operation" );
				return 0;
			}
			break;
		}

		case OP_NOT:
		{
			if ( !vm_check_stack( vm, 1 ) )
				return 0;
			Value a = vm->stack[vm->sp];

			if ( a.type == VAL_BOOL )
			{
				vm->stack[vm->sp] = make_bool( !a.data.boolean );
			}
			else if ( a.type == VAL_INT )
			{
				vm->stack[vm->sp] = make_bool( !a.data.i );
			}
			else
			{
				vm_error( vm, "Type mismatch for NOT operation" );
				return 0;
			}
			break;
		}

		case OP_JMP:
			vm->ip = vm->code[vm->ip];
			break;

		case OP_JZ:
		{
			if ( !vm_check_stack( vm, 1 ) )
				return 0;
			Value val     = vm->stack[vm->sp--];
			int   is_zero = 0;

			if ( val.type == VAL_INT )
			{
				is_zero = ( val.data.i == 0 );
			}
			else if ( val.type == VAL_BOOL )
			{
				is_zero = ( val.data.boolean == 0 );
			}
			else
			{
				vm_error( vm, "Type mismatch for JZ operation" );
				return 0;
			}

			if ( is_zero )
				vm->ip = vm->code[vm->ip];
			else
				vm->ip++;
			break;
		}

		case OP_JNZ:
		{
			if ( !vm_check_stack( vm, 1 ) )
				return 0;
			Value val         = vm->stack[vm->sp--];
			int   is_not_zero = 0;

			if ( val.type == VAL_INT )
			{
				is_not_zero = ( val.data.i != 0 );
			}
			else if ( val.type == VAL_BOOL )
			{
				is_not_zero = ( val.data.boolean != 0 );
			}
			else
			{
				vm_error( vm, "Type mismatch for JNZ operation" );
				return 0;
			}

			if ( is_not_zero )
				vm->ip = vm->code[vm->ip];
			else
				vm->ip++;
			break;
		}

		case OP_LOAD:
		{
			if ( !vm_check_stack( vm, -1 ) )
				return 0;
			int offset = vm->code[vm->ip++];

			if ( vm->call_sp <= 0 )
			{
				vm_error( vm, "LOAD outside of a function" );
				return 0;
			}

			vm->stack[++vm->sp] = vm->stack[vm->call_stack[vm->call_sp - 1].base_pointer + offset];
			break;
		}

		case OP_STORE:
		{
			if ( !vm_check_stack( vm, 1 ) )
				return 0;
			int offset = vm->code[vm->ip++];

			if ( vm->call_sp <= 0 )
			{
				vm_error( vm, "STORE outside of a function" );
				return 0;
			}

			vm->stack[vm->call_stack[vm->call_sp - 1].base_pointer + offset] = vm->stack[vm->sp--];
			break;
		}

		case OP_LOAD_GLOBAL:
		{
			if ( !vm_check_stack( vm, -1 ) )
				return 0;
			int index = vm->code[vm->ip++];

			if ( index < 0 || index >= GLOBALS_SIZE )
			{
				vm_error( vm, "Global index out of bounds: %d", index );
				return 0;
			}

			vm->stack[++vm->sp] = vm->globals[index];
			break;
		}

		case OP_STORE_GLOBAL:
		{
			if ( !vm_check_stack( vm, 1 ) )
				return 0;
			int index = vm->code[vm->ip++];

			if ( index < 0 || index >= GLOBALS_SIZE )
			{
				vm_error( vm, "Global index out of bounds: %d", index );
				return 0;
			}

			vm->globals[index] = vm->stack[vm->sp--];
			break;
		}

		case OP_CALL:
		{
			int addr      = vm->code[vm->ip++];
			int arg_count = vm->code[vm->ip++];

			if ( vm->call_sp >= CALL_STACK_SIZE )
			{
				vm_error( vm, "Call stack overflow" );
				return 0;
			}

			vm->call_stack[vm->call_sp++] = (CallFrame){ vm->ip, vm->sp - arg_count, arg_count };
			vm->ip                        = addr;
			break;
		}

		case OP_RET:
		{
			if ( vm->call_sp <= 0 )
			{
				vm_error( vm, "RET without CALL" );
				return 0;
			}

			CallFrame frame   = vm->call_stack[--vm->call_sp];
			Value     ret_val = make_null();
			if ( vm->sp > frame.base_pointer )
			{
				ret_val = vm->stack[vm->sp];
			}

			vm->sp = frame.base_pointer;
			if ( ret_val.type != VAL_NULL )
			{
				vm->stack[++vm->sp] = ret_val;
			}

			vm->ip = frame.return_ip;
			break;
		}

		case OP_CALL_NATIVE:
		{
			int func_id   = vm->code[vm->ip++];
			int arg_count = vm->code[vm->ip++];

			if ( func_id < 0 || func_id >= MAX_NATIVE_FUNCS )
			{
				vm_error( vm, "Invalid native function ID: %d", func_id );
				return 0;
			}

			if ( !vm->native_functions[func_id] )
			{
				vm_error( vm, "Native function not registered: %d", func_id );
				return 0;
			}

			if ( vm->call_sp >= CALL_STACK_SIZE )
			{
				vm_error( vm, "Call stack overflow" );
				return 0;
			}

			vm->call_stack[vm->call_sp++] = (CallFrame){ vm->ip, vm->sp - arg_count, arg_count };
			vm->native_functions[func_id]( vm );
			CallFrame frame = vm->call_stack[--vm->call_sp];

			if ( vm->sp > frame.base_pointer )
			{
				Value ret_val       = vm->stack[vm->sp];
				vm->sp              = frame.base_pointer;
				vm->stack[++vm->sp] = ret_val;
			}
			else
			{
				vm->sp = frame.base_pointer;
			}
			break;
		}

		case OP_ALLOC:
		{
			if ( !vm_check_stack( vm, -1 ) )
				return 0;
			int   size = vm->code[vm->ip++];
			void *mem  = gc_alloc( vm, size * sizeof( Value ), VAL_PTR );

			if ( !mem )
			{
				vm_error( vm, "Memory allocation failed" );
				return 0;
			}

			for ( int i = 0; i < size; i++ )
			{
				( (Value *)mem )[i] = make_null();
			}

			vm->stack[++vm->sp] = make_ptr( mem );
			break;
		}

		case OP_SETFIELD:
		{
			if ( !vm_check_stack( vm, 2 ) )
				return 0;
			int   index = vm->code[vm->ip++];
			Value value = vm->stack[vm->sp--];
			Value obj   = vm->stack[vm->sp--];

			if ( obj.type != VAL_PTR || obj.data.ptr == NULL )
			{
				vm_error( vm, "SETFIELD on non-object" );
				return 0;
			}

			( (Value *)obj.data.ptr )[index] = value;
			break;
		}

		case OP_GETFIELD:
		{
			if ( !vm_check_stack( vm, 1 ) )
				return 0;
			int   index = vm->code[vm->ip++];
			Value obj   = vm->stack[vm->sp];

			if ( obj.type != VAL_PTR || obj.data.ptr == NULL )
			{
				vm_error( vm, "GETFIELD on non-object" );
				return 0;
			}

			vm->stack[vm->sp] = ( (Value *)obj.data.ptr )[index];
			break;
		}

		case OP_PUSH_ARG:
		{
			if ( !vm_check_stack( vm, -1 ) )
				return 0;
			int arg_index = vm->code[vm->ip++];

			if ( vm->call_sp <= 0 )
			{
				vm_error( vm, "Cannot access arguments outside function" );
				return 0;
			}

			CallFrame *frame = &vm->call_stack[vm->call_sp - 1];
			if ( arg_index < 0 || arg_index >= frame->arg_count )
			{
				vm_error( vm, "Argument index out of bounds: %d", arg_index );
				return 0;
			}

			vm->stack[++vm->sp] = vm->stack[frame->base_pointer - frame->arg_count + arg_index];
			break;
		}

		case OP_SET_ARG:
		{
			if ( !vm_check_stack( vm, 1 ) )
				return 0;
			int arg_index = vm->code[vm->ip++];

			if ( vm->call_sp <= 0 )
			{
				vm_error( vm, "Cannot set arguments outside function" );
				return 0;
			}

			CallFrame *frame = &vm->call_stack[vm->call_sp - 1];
			if ( arg_index < 0 || arg_index >= frame->arg_count )
			{
				vm_error( vm, "Argument index out of bounds: %d", arg_index );
				return 0;
			}

			vm->stack[frame->base_pointer - frame->arg_count + arg_index] = vm->stack[vm->sp--];
			break;
		}

		default:
			vm_error( vm, "Unknown opcode: %d", opcode );
			return 0;
		}
	}

	return 1;
}

int vm_load_bytecode( VM *vm, const char *filename )
{
	FILE *fp = fopen( filename, "rb" );
	if ( !fp )
	{
		fprintf( stderr, "Failed to open bytecode file: %s\n", filename );
		return 0;
	}

	int code_size;
	if ( fread( &code_size, sizeof( int ), 1, fp ) != 1 )
	{
		fprintf( stderr, "Failed to read bytecode size\n" );
		fclose( fp );
		return 0;
	}

	vm->code = (int *)malloc( code_size * sizeof( int ) );
	if ( !vm->code )
	{
		fprintf( stderr, "Failed to allocate memory for bytecode\n" );
		fclose( fp );
		return 0;
	}

	if ( fread( vm->code, sizeof( int ), code_size, fp ) != code_size )
	{
		fprintf( stderr, "Failed to read bytecode\n" );
		free( vm->code );
		vm->code = NULL;
		fclose( fp );
		return 0;
	}

	int string_table_size;
	if ( fread( &string_table_size, sizeof( int ), 1, fp ) != 1 )
	{
		fprintf( stderr, "Failed to read string table size\n" );
		free( vm->code );
		vm->code = NULL;
		fclose( fp );
		return 0;
	}

	vm->string_table = (char **)malloc( string_table_size * sizeof( char * ) );
	if ( !vm->string_table )
	{
		fprintf( stderr, "Failed to allocate memory for string table\n" );
		free( vm->code );
		vm->code = NULL;
		fclose( fp );
		return 0;
	}

	for ( int i = 0; i < string_table_size; i++ )
	{
		int string_len;
		if ( fread( &string_len, sizeof( int ), 1, fp ) != 1 )
		{
			fprintf( stderr, "Failed to read string length\n" );
			free( vm->code );
			vm->code = NULL;
			free( vm->string_table );
			vm->string_table = NULL;
			fclose( fp );
			return 0;
		}

		vm->string_table[i] = (char *)malloc( string_len );
		if ( fread( vm->string_table[i], 1, string_len, fp ) != string_len )
		{
			fprintf( stderr, "Failed to read string\n" );
			free( vm->code );
			vm->code = NULL;
			free( vm->string_table );
			vm->string_table = NULL;
			fclose( fp );
			return 0;
		}

		vm->string_table_size++;
	}

	fclose( fp );
	return 1;
}
