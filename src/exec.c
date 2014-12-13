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
	char *cmd = calloc( 2048, sizeof( char * ) );
	int cmdc = 0;

	// Iterate through all config fields
	WJElement item = WJEChild( *conf, "config", WJE_GET )->child;
	do {
		int i;

		// Copy the name and make it uppercase
		for( i = 0; item->name[i] && cmdc < 2048-4; i++ ) {
			cmd[cmdc++] = toupper( item->name[i] );
		}
		
		// Delimiter
		cmd[cmdc++] = '=';
		cmd[cmdc++] = '"';

		// Value
		char *value = WJEString( item, "", WJE_GET, NULL );
		for( i = 0; value[i] && cmdc < 2048-4; i++ ) {
			cmd[cmdc++] = value[i];
		}

		// Finalise
		cmd[cmdc++] = '"';
		cmd[cmdc++] = ' ';

	} while( (item = item->next) );

	// Add path
	strncat( cmd, path, 2048 - cmdc );

	// Execute
	int ret = system( cmd );

	// Clean up
	free( cmd );

	return ret;

}

int exec_revert( char *path ) {

	// Execute
	int ret = system( path );

	return ret;

}

