#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wjelement.h>
#include <ctype.h>
#include <unistd.h>
#include "../include/exec.h"


void exec_err_callback( void *client, const char *format, ... ) {
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

int exec_check( WJElement *conf, char *path ) {
	
	// Load schema
	FILE *f = fopen( path, "r" );
	WJReader reader = WJROpenFILEDocument( f, NULL, 0 );
	WJElement schema = WJEOpenDocument( reader, NULL, NULL, NULL );
	WJRCloseDocument( reader );
	fclose( f );

	// Check schema
	int ret;
	if( ! WJESchemaValidate(
		schema,
		WJEGet( *conf, "config", NULL),
		exec_err_callback,
		NULL, NULL, NULL
	) ) {
		ret = 1;
	} else {
		ret = 0;
	}

	WJECloseDocument( schema );

	return ret;
}

int exec_commit( WJElement *conf, char *path ) {

	// Build environment array
	char **env = malloc( sizeof( char * ) );
	int envc = 0;

	// Iterate through all config fields
	WJElement item = WJEChild( *conf, "config", WJE_GET )->child;
	do {
		int i;

		// Create string buffer
		env[envc] = calloc( 128, sizeof( char ) );

		// Copy the name and make it uppercase
		for( i = 0; item->name[i]; i++ ) {
			env[envc][i] = toupper( item->name[i] );
		}
		
		// Delimiter
		env[envc][i++] = '=';
		env[envc][i++] = '\0';

		// Value
		strncat( env[envc], WJEString( item, "", WJE_GET, NULL ), 128 - i );

		// Create a new element
		envc++;
		env = realloc( env, sizeof( char * ) * (envc+1) );

	} while( (item = item->next) );
	// Terminate array with null pointer
	env[envc] = (char*) 0;

	// Command array
	char *cmd[] = { "sh", path, (char *) 0 };

	// Execute
	int ret = execve( "/bin/sh", cmd, env );

	// Clean up
	while( envc-- ) {
		free( env[ envc ] );
	}
	free( env );

	return ret;

}

int exec_revert( char *path ) {

	// Command array
	char *cmd[] = { "sh", path, (char *) 0 };

	// Execute
	int ret = execv( "/bin/sh", cmd );

	return ret;

}

