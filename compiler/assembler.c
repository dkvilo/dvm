//
// note (david) this is WIP and syntax will be changed
//
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../vm/vm.h"

#define MAX_LABELS 256
#define MAX_LINE_LENGTH 256
#define MAX_BYTECODE_SIZE 10000
#define MAX_STRINGS 256

typedef struct
{
	char name[64];
	int  address;
} Label;

typedef struct
{
	char strings[MAX_STRINGS][MAX_STRING_LENGTH];
	int  count;
} StringTable;

typedef struct
{
	Label       labels[MAX_LABELS];
	int         label_count;
	int         bytecode[MAX_BYTECODE_SIZE];
	int         bytecode_size;
	int         pass;
	StringTable string_table;
} Assembler;

void asm_init( Assembler *a )
{
	a->label_count        = 0;
	a->bytecode_size      = 0;
	a->pass               = 1;
	a->string_table.count = 0;
}

int asm_find_label( Assembler *a, const char *name )
{
	for ( int i = 0; i < a->label_count; i++ )
	{
		if ( strcmp( a->labels[i].name, name ) == 0 )
		{
			return a->labels[i].address;
		}
	}
	return -1;
}

void asm_add_label( Assembler *a, const char *name, int address )
{
	if ( a->label_count >= MAX_LABELS )
	{
		fprintf( stderr, "Too many labels\n" );
		exit( 1 );
	}

	strncpy( a->labels[a->label_count].name, name, 63 );
	a->labels[a->label_count].name[63] = '\0';
	a->labels[a->label_count].address  = address;
	a->label_count++;
}

int asm_add_string( Assembler *a, const char *str )
{
	if ( a->string_table.count >= MAX_STRINGS )
	{
		fprintf( stderr, "Too many strings\n" );
		exit( 1 );
	}

	for ( int i = 0; i < a->string_table.count; i++ )
	{
		if ( strcmp( a->string_table.strings[i], str ) == 0 )
		{
			return i;
		}
	}

	strncpy( a->string_table.strings[a->string_table.count], str, MAX_STRING_LENGTH - 1 );
	a->string_table.strings[a->string_table.count][MAX_STRING_LENGTH - 1] = '\0';

	int index = a->string_table.count++;
	return index;
}

void asm_emit( Assembler *a, int value )
{
	if ( a->bytecode_size >= MAX_BYTECODE_SIZE )
	{
		fprintf( stderr, "Bytecode buffer overflow\n" );
		exit( 1 );
	}

	if ( a->pass == 2 )
	{
		a->bytecode[a->bytecode_size] = value;
	}
	a->bytecode_size++;
}

int asm_emit_string_in_code( Assembler *a, const char *str )
{
	int addr = a->bytecode_size;
	for ( int i = 0; str[i] != '\0'; i++ )
	{
		asm_emit( a, (int)(unsigned char)str[i] );
	}
	asm_emit( a, 0 );
	return addr;
}

int asm_parse_opcode( const char *name )
{
	if ( strcmp( name, "NOP" ) == 0 )
		return OP_NOP;
	if ( strcmp( name, "HALT" ) == 0 )
		return OP_HALT;
	if ( strcmp( name, "JMP" ) == 0 )
		return OP_JMP;
	if ( strcmp( name, "JZ" ) == 0 )
		return OP_JZ;
	if ( strcmp( name, "JNZ" ) == 0 )
		return OP_JNZ;
	if ( strcmp( name, "PUSH_INT" ) == 0 )
		return OP_PUSH_INT;
	if ( strcmp( name, "PUSH_FLOAT" ) == 0 )
		return OP_PUSH_FLOAT;
	if ( strcmp( name, "PUSH_STRING" ) == 0 )
		return OP_PUSH_STRING;
	if ( strcmp( name, "POP" ) == 0 )
		return OP_POP;
	if ( strcmp( name, "DUP" ) == 0 )
		return OP_DUP;
	if ( strcmp( name, "SWAP" ) == 0 )
		return OP_SWAP;
	if ( strcmp( name, "ADD" ) == 0 )
		return OP_ADD;
	if ( strcmp( name, "SUB" ) == 0 )
		return OP_SUB;
	if ( strcmp( name, "MUL" ) == 0 )
		return OP_MUL;
	if ( strcmp( name, "DIV" ) == 0 )
		return OP_DIV;
	if ( strcmp( name, "MOD" ) == 0 )
		return OP_MOD;
	if ( strcmp( name, "EQ" ) == 0 )
		return OP_EQ;
	if ( strcmp( name, "NEQ" ) == 0 )
		return OP_NEQ;
	if ( strcmp( name, "GT" ) == 0 )
		return OP_GT;
	if ( strcmp( name, "LT" ) == 0 )
		return OP_LT;
	if ( strcmp( name, "GTE" ) == 0 )
		return OP_GTE;
	if ( strcmp( name, "LTE" ) == 0 )
		return OP_LTE;
	if ( strcmp( name, "AND" ) == 0 )
		return OP_AND;
	if ( strcmp( name, "OR" ) == 0 )
		return OP_OR;
	if ( strcmp( name, "NOT" ) == 0 )
		return OP_NOT;
	if ( strcmp( name, "LOAD" ) == 0 )
		return OP_LOAD;
	if ( strcmp( name, "STORE" ) == 0 )
		return OP_STORE;
	if ( strcmp( name, "CALL" ) == 0 )
		return OP_CALL;
	if ( strcmp( name, "RET" ) == 0 )
		return OP_RET;
	if ( strcmp( name, "CALL_NATIVE" ) == 0 )
		return OP_CALL_NATIVE;
	if ( strcmp( name, "ALLOC" ) == 0 )
		return OP_ALLOC;
	if ( strcmp( name, "SETFIELD" ) == 0 )
		return OP_SETFIELD;
	if ( strcmp( name, "GETFIELD" ) == 0 )
		return OP_GETFIELD;
	if ( strcmp( name, "LOAD_GLOBAL" ) == 0 )
		return OP_LOAD_GLOBAL;
	if ( strcmp( name, "STORE_GLOBAL" ) == 0 )
		return OP_STORE_GLOBAL;
	if ( strcmp( name, "PUSH_ARG" ) == 0 )
		return OP_PUSH_ARG;
	if ( strcmp( name, "SET_ARG" ) == 0 )
		return OP_SET_ARG;

	fprintf( stderr, "Unknown opcode: %s\n", name );
	return -1;
}

char *parse_string_literal( char *str )
{
	static char buffer[MAX_STRING_LENGTH];
	int         len = strlen( str );

	if ( len >= 2 && str[0] == '"' && str[len - 1] == '"' )
	{
		str[len - 1] = '\0';
		str++;
	}

	int j = 0;
	for ( int i = 0; str[i] && j < MAX_STRING_LENGTH - 1; i++ )
	{
		if ( str[i] == '\\' && str[i + 1] )
		{
			i++;
			switch ( str[i] )
			{
			case 'n':
				buffer[j++] = '\n';
				break;
			case 't':
				buffer[j++] = '\t';
				break;
			case 'r':
				buffer[j++] = '\r';
				break;
			case '0':
				buffer[j++] = '\0';
				break;
			case '\\':
				buffer[j++] = '\\';
				break;
			case '"':
				buffer[j++] = '"';
				break;
			default:
				buffer[j++] = str[i];
				break;
			}
		}
		else
		{
			buffer[j++] = str[i];
		}
	}
	buffer[j] = '\0';
	return buffer;
}

void asm_process_line( Assembler *a, char *line )
{
	char *comment = strchr( line, ';' );
	if ( comment )
		*comment = '\0';

	while ( *line && isspace( *line ) )
		line++;
	if ( *line == '\0' )
		return;

	if ( line[0] == ':' )
	{
		char label[64];
		sscanf( line + 1, "%63s", label );

		if ( a->pass == 1 )
		{
			asm_add_label( a, label, a->bytecode_size );
		}
		return;
	}

	char opcode_str[32];
	char arg1[64] = "";
	char arg2[64] = "";

	// note (david) PUSH_STRING might have space in the middle
	if ( strncmp( line, "PUSH_STRING", 11 ) == 0 && isspace( line[11] ) )
	{
		strcpy( opcode_str, "PUSH_STRING" );

		char *str_start = line + 11;
		while ( *str_start && isspace( *str_start ) )
			str_start++;

		if ( *str_start == '"' )
		{
			char *str_end = str_start + 1;
			while ( *str_end && *str_end != '"' )
			{
				if ( *str_end == '\\' && *( str_end + 1 ) )
					str_end++;
				str_end++;
			}
			if ( *str_end == '"' )
			{
				str_end++;
				size_t len = str_end - str_start;
				if ( len > sizeof( arg1 ) - 1 )
					len = sizeof( arg1 ) - 1;
				strncpy( arg1, str_start, len );
				arg1[len] = '\0';
			}
		}
		else
		{
			strncpy( arg1, str_start, sizeof( arg1 ) - 1 );
			arg1[sizeof( arg1 ) - 1] = '\0';
		}
	}
	else
	{
		int num_args = sscanf( line, "%31s %63s %63s", opcode_str, arg1, arg2 );
		if ( num_args < 1 )
			return;
	}

	int opcode = asm_parse_opcode( opcode_str );
	if ( opcode < 0 )
		return;

	asm_emit( a, opcode );

	switch ( opcode )
	{
	case OP_PUSH_INT:
	case OP_LOAD:
	case OP_STORE:
	case OP_LOAD_GLOBAL:
	case OP_STORE_GLOBAL:
	case OP_ALLOC:
	case OP_PUSH_ARG:
	case OP_SET_ARG:
	case OP_GETFIELD:
	case OP_SETFIELD:
		if ( arg1[0] != '\0' )
		{
			asm_emit( a, atoi( arg1 ) );
		}
		else
		{
			fprintf( stderr, "Missing argument for %s\n", opcode_str );
		}
		break;

	case OP_PUSH_FLOAT:
		if ( arg1[0] != '\0' )
		{
			float f       = atof( arg1 );
			int  *int_rep = (int *)&f;
			asm_emit( a, *int_rep );
		}
		else
		{
			fprintf( stderr, "Missing argument for %s\n", opcode_str );
		}
		break;

	case OP_PUSH_STRING:
		if ( arg1[0] != '\0' )
		{
			// note (david) come back to this and decide how do we want to process strings
			char *parsed  = parse_string_literal( arg1 );
			int   str_idx = asm_add_string( a, parsed );
			asm_emit( a, str_idx );
			// char* parsed = parse_string_literal(arg1);
			// int addr = asm_emit_string_in_code(a, parsed);
			// asm_emit(a, addr);
		}
		else
		{
			fprintf( stderr, "Missing string for PUSH_STRING\n" );
		}
		break;

	case OP_JMP:
	case OP_JZ:
	case OP_JNZ:
		if ( arg1[0] != '\0' )
		{
			if ( a->pass == 1 )
			{
				asm_emit( a, 0 );
			}
			else
			{
				int addr = asm_find_label( a, arg1 );
				if ( addr < 0 )
				{
					fprintf( stderr, "Unknown label: %s\n", arg1 );
					asm_emit( a, 0 );
				}
				else
				{
					asm_emit( a, addr );
				}
			}
		}
		else
		{
			fprintf( stderr, "Missing label for %s\n", opcode_str );
		}
		break;

	case OP_CALL:
		// Label reference + arg count
		if ( arg1[0] != '\0' && arg2[0] != '\0' )
		{
			if ( a->pass == 1 )
			{
				asm_emit( a, 0 );            // temp fop address
				asm_emit( a, atoi( arg2 ) ); // arg count
			}
			else
			{
				int addr = asm_find_label( a, arg1 );
				if ( addr < 0 )
				{
					fprintf( stderr, "Unknown function: %s\n", arg1 );
					asm_emit( a, 0 );
				}
				else
				{
					asm_emit( a, addr );
				}
				asm_emit( a, atoi( arg2 ) ); // arg count
			}
		}
		else
		{
			fprintf( stderr, "Missing arguments for CALL\n" );
		}
		break;

	case OP_CALL_NATIVE:
		if ( arg1[0] != '\0' && arg2[0] != '\0' )
		{
			asm_emit( a, atoi( arg1 ) ); // native function ID
			asm_emit( a, atoi( arg2 ) ); // arg count
		}
		else
		{
			fprintf( stderr, "Missing arguments for CALL_NATIVE\n" );
		}
		break;
	}
}

int asm_assemble_file( Assembler *a, const char *filename )
{
	FILE *fp = fopen( filename, "r" );
	if ( !fp )
	{
		fprintf( stderr, "Cannot open file: %s\n", filename );
		return 0;
	}

	char line[MAX_LINE_LENGTH];

	// collect labels
	a->pass          = 1;
	a->bytecode_size = 0;

	while ( fgets( line, MAX_LINE_LENGTH, fp ) )
	{
		asm_process_line( a, line );
	}

	rewind( fp );
	a->pass          = 2;
	a->bytecode_size = 0;

	while ( fgets( line, MAX_LINE_LENGTH, fp ) )
	{
		asm_process_line( a, line );
	}

	fclose( fp );
	return 1;
}

int asm_write_bytecode( Assembler *a, const char *filename )
{
	FILE *fp = fopen( filename, "wb" );
	if ( !fp )
	{
		fprintf( stderr, "Cannot create output file: %s\n", filename );
		return 0;
	}

	// bytecode size
	fwrite( &a->bytecode_size, sizeof( int ), 1, fp );

	// bytecode
	fwrite( a->bytecode, sizeof( int ), a->bytecode_size, fp );

	// string table size
	fwrite( &a->string_table.count, sizeof( int ), 1, fp );

	// strings
	for ( int i = 0; i < a->string_table.count; i++ )
	{
		int len = strlen( a->string_table.strings[i] ) + 1;
		fwrite( &len, sizeof( int ), 1, fp );
		fwrite( a->string_table.strings[i], 1, len, fp );
	}

	fclose( fp );
	return 1;
}

void asm_print_bytecode( Assembler *a )
{
	printf( "Bytecode size: %ld bytes\n", a->bytecode_size * sizeof( int ) );
	for ( int i = 0; i < a->bytecode_size; i++ )
	{
		printf( "%04d: %d", i, a->bytecode[i] );

		if ( i == 0 || a->bytecode[i - 1] != OP_PUSH_INT && a->bytecode[i - 1] != OP_PUSH_FLOAT &&
		                   a->bytecode[i - 1] != OP_PUSH_STRING && a->bytecode[i - 1] != OP_LOAD &&
		                   a->bytecode[i - 1] != OP_STORE && a->bytecode[i - 1] != OP_JMP &&
		                   a->bytecode[i - 1] != OP_JZ && a->bytecode[i - 1] != OP_JNZ &&
		                   a->bytecode[i - 1] != OP_CALL && a->bytecode[i - 1] != OP_CALL_NATIVE &&
		                   a->bytecode[i - 1] != OP_ALLOC && a->bytecode[i - 1] != OP_GETFIELD &&
		                   a->bytecode[i - 1] != OP_SETFIELD && a->bytecode[i - 1] != OP_LOAD_GLOBAL &&
		                   a->bytecode[i - 1] != OP_STORE_GLOBAL && a->bytecode[i - 1] != OP_PUSH_ARG &&
		                   a->bytecode[i - 1] != OP_SET_ARG )
		{

			switch ( a->bytecode[i] )
			{
			case OP_NOP:
				printf( " (NOP)" );
				break;
			case OP_HALT:
				printf( " (HALT)" );
				break;
			case OP_JMP:
				printf( " (JMP)" );
				break;
			case OP_JZ:
				printf( " (JZ)" );
				break;
			case OP_JNZ:
				printf( " (JNZ)" );
				break;
			case OP_PUSH_INT:
				printf( " (PUSH_INT)" );
				break;
			case OP_PUSH_FLOAT:
				printf( " (PUSH_FLOAT)" );
				break;
			case OP_PUSH_STRING:
				printf( " (PUSH_STRING)" );
				break;
			case OP_POP:
				printf( " (POP)" );
				break;
			case OP_DUP:
				printf( " (DUP)" );
				break;
			case OP_SWAP:
				printf( " (SWAP)" );
				break;
			case OP_ADD:
				printf( " (ADD)" );
				break;
			case OP_SUB:
				printf( " (SUB)" );
				break;
			case OP_MUL:
				printf( " (MUL)" );
				break;
			case OP_DIV:
				printf( " (DIV)" );
				break;
			case OP_MOD:
				printf( " (MOD)" );
				break;
			case OP_EQ:
				printf( " (EQ)" );
				break;
			case OP_NEQ:
				printf( " (NEQ)" );
				break;
			case OP_GT:
				printf( " (GT)" );
				break;
			case OP_LT:
				printf( " (LT)" );
				break;
			case OP_GTE:
				printf( " (GTE)" );
				break;
			case OP_LTE:
				printf( " (LTE)" );
				break;
			case OP_AND:
				printf( " (AND)" );
				break;
			case OP_OR:
				printf( " (OR)" );
				break;
			case OP_NOT:
				printf( " (NOT)" );
				break;
			case OP_LOAD:
				printf( " (LOAD)" );
				break;
			case OP_STORE:
				printf( " (STORE)" );
				break;
			case OP_CALL:
				printf( " (CALL)" );
				break;
			case OP_RET:
				printf( " (RET)" );
				break;
			case OP_CALL_NATIVE:
				printf( " (CALL_NATIVE)" );
				break;
			case OP_ALLOC:
				printf( " (ALLOC)" );
				break;
			case OP_SETFIELD:
				printf( " (SETFIELD)" );
				break;
			case OP_GETFIELD:
				printf( " (GETFIELD)" );
				break;
			case OP_LOAD_GLOBAL:
				printf( " (LOAD_GLOBAL)" );
				break;
			case OP_STORE_GLOBAL:
				printf( " (STORE_GLOBAL)" );
				break;
			case OP_PUSH_ARG:
				printf( " (PUSH_ARG)" );
				break;
			case OP_SET_ARG:
				printf( " (SET_ARG)" );
				break;
			}
		}

		if ( i > 0 && a->bytecode[i - 1] == OP_PUSH_STRING && a->bytecode[i] < a->string_table.count )
		{
			printf( " (\"%s\")", a->string_table.strings[a->bytecode[i]] );
		}

		printf( "\n" );
	}

	printf( "\nString table (%d entries):\n", a->string_table.count );
	for ( int i = 0; i < a->string_table.count; i++ )
	{
		printf( "[%d] \"%s\"\n", i, a->string_table.strings[i] );
	}

	printf( "\nLabels (%d entries):\n", a->label_count );
	for ( int i = 0; i < a->label_count; i++ )
	{
		printf( "[%d] %s -> %d\n", i, a->labels[i].name, a->labels[i].address );
	}
}

int main( int argc, char **argv )
{
	if ( argc < 3 )
	{
		fprintf( stderr, "Usage: %s <input.asm> <output.bin> [-d]\n", argv[0] );
		return 1;
	}

	const char *input_file  = argv[1];
	const char *output_file = argv[2];
	int         debug_mode  = ( argc > 3 && strcmp( argv[3], "-d" ) == 0 );

	Assembler assembler;
	asm_init( &assembler );

	if ( !asm_assemble_file( &assembler, input_file ) )
	{
		fprintf( stderr, "Assembly failed\n" );
		return 1;
	}

	if ( debug_mode )
	{
		asm_print_bytecode( &assembler );
	}

	if ( !asm_write_bytecode( &assembler, output_file ) )
	{
		fprintf( stderr, "Failed to write output file\n" );
		return 1;
	}

	printf( "Assembly successful: %d bytecode instructions\n", assembler.bytecode_size );
	return 0;
}