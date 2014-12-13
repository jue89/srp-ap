#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wjelement.h>
#include "../include/ctrl.h"


static long conf_load( CTRL *ctrl, WJElement *localConf, WJElement *conf ) {
	
	//// Prepare request
	
	// Create a new buffer for ETag. When it changes, this buffer will be used for the new ETag.
	char etag[128];
	strcpy( etag, WJEString( *conf, "etag", WJE_GET, "" ) );

	// Create URL to endpoint
	char endpoint[128] = "aps/";
	strcat( endpoint, WJEString( *localConf, "id", WJE_GET, "" ) );
	strcat( endpoint, "/config" );



	//// LOAD CONFIG

	WJElement res;
	long code = ctrl_get_etag( ctrl, endpoint, etag, &res );


	
	//// Handle response
	
	if( code == 200 ) {
		// Something has changed!
		
		// Save ETag
		WJEString( res, "etag", WJE_SET, etag );

		// Clean old conf
		WJECloseDocument( *conf );

		// Copy new conf
		*conf = WJECopyDocument( NULL, res, NULL, NULL );
	}

	// Clean res
	WJECloseDocument( res );
	

	return code;
	
}

int conf_fetch( CTRL *ctrl, WJElement *localConf, WJElement *conf, const char *path ) {
	
	//// READ CONFIG FILE
	
	// Check if config file exists
	FILE *f = fopen( path, "r" );
	if( f ) {
		// Exists --> Read
		WJReader reader = WJROpenFILEDocument( f, NULL, 0 );
		*conf = WJEOpenDocument( reader, NULL, NULL, NULL );
		WJRCloseDocument( reader );
		fclose( f );
	} else {
		// Nope --> Empty object
		*conf = WJEObject( NULL, NULL, WJE_NEW );
	}


	
	//// Check config file
	
	long code = conf_load( ctrl, localConf, conf );



	//// SAVE CONFIG FILE

	f = fopen( path, "w" );
	WJWriter writer = WJWOpenFILEDocument( TRUE, f );
	WJEWriteDocument( *conf, writer, NULL );
	WJWCloseDocument( writer );
	fclose( f );


	// Code 200 indicates that something has changed
	return code == 200 ? 1 : 0;

}


