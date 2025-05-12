#include <stdio.h>

#include "engine.h"
#include "command.h"

void engine_run( int argc, char **argv )
{
	VM *vm = vm_create();

	vm_register_native( vm, 0, native_print );
	vm_register_native( vm, 1, native_concat );
	vm_register_native( vm, 2, native_make_window );
	vm_register_native( vm, 3, native_process_frame );
	vm_register_native( vm, 4, native_clear_color_buffer );
	vm_register_native( vm, 5, native_draw_rect );
	vm_register_native( vm, 6, native_draw_text );
	vm_register_native( vm, 7, native_randint );

	if ( argc < 2 )
	{
		fprintf( stderr, "Usage: %s <bytecode>\n", argv[0] );
		exit( EXIT_FAILURE );
	}

	if ( !vm_load_bytecode( vm, argv[1] ) )
	{
		printf( "Failed to load bytecode\n" );
		vm_destroy( vm );
		exit( EXIT_FAILURE );
	}

	printf( "Running script...\n" );
	vm_run( vm );

	if ( strlen( vm->error_msg ) > 0 )
	{
		printf( "Script ended with error: %s\n", vm->error_msg );
	}
	else
	{
		printf( "Script completed successfully\n" );
	}

	vm_destroy( vm );
}