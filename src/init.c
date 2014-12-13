#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wjelement.h>
#include "../include/ctrl.h"
#include "../include/init.h"


// Will create a new key pair
static int init_create_keys( WJElement *localConf ) {
	char privateKey[65];
	char publicKey[65];

	// Call fastd and read input into Buffers
	FILE *fastd = popen("fastd --generate-key 2> /dev/null", "r");
	fscanf( fastd, "Secret: %64s\n", privateKey );
	fscanf( fastd, "Public: %64s\n", publicKey );
	pclose( fastd );

	// Save buffers into config object
	WJEString( *localConf, "privateKey", WJE_SET, privateKey );
	WJEString( *localConf, "publicKey", WJE_SET, publicKey );

	return 0;
}

// Will check if AP is still registered at the controller
static int init_check_ap( CTRL *ctrl, WJElement *localConf ) {
	// Get id
	char *id = WJEString( *localConf, "id", WJE_GET, "" );

	// Create endpoint link
	char endpoint[128] = "aps/";
	strcat( endpoint, id );

	// Request
	WJElement res;
	long code = ctrl_get( ctrl, endpoint, &res );
	WJECloseDocument( res );
	if( code == 404 ) {
		// Not found --> Delete id from config object
		WJECloseDocument( WJEGet( *localConf, "id", NULL ) );
		return 0;
	} else if( code == 200 ) {
		// Found
		return 0;
	}

	// Error occured
	return 1;
}

// Register an new AP
static int init_register_ap( CTRL *ctrl, WJElement *localConf ) {
	// Get public key
	char *publicKey = WJEString( *localConf, "publicKey", WJE_GET, "" );

	// Create request
	WJElement req = WJEObject( NULL, NULL, WJE_NEW );
	WJEObject( req, "aps", WJE_NEW );
	WJEString( req, "aps.public_key", WJE_SET, publicKey );

	// Create res
	WJElement res;

	// Request
	long code = ctrl_post( ctrl, "aps", &req, &res );
	
	// Something went wrong!
	if( code != 201 ) {
		WJECloseDocument( req );
		WJECloseDocument( res );
		return 1;
	}

	// Interprete request
	char *id = WJEString( res, "aps.id", WJE_GET, "" );

	// Update local config
	WJEString( *localConf, "id", WJE_SET, id );

	// Clean up
	WJECloseDocument( req );
	WJECloseDocument( res );

	return 0;
}

int init( CTRL *ctrl, WJElement *localConf, const char *path ) {
	
	//// READ CONFIG FILE
	
	// Check if config file exists
	FILE *f = fopen( path, "r" );
	if( f ) {
		// Exists --> Read
		WJReader reader = WJROpenFILEDocument( f, NULL, 0 );
		*localConf = WJEOpenDocument( reader, NULL, NULL, NULL );
		WJRCloseDocument( reader );
		fclose( f );
	} else {
		// Nope --> Empty object
		*localConf = WJEObject( NULL, NULL, WJE_NEW );
	}


	
	//// Check config file
	
	// When no private key is set generate a new key pair
	if( WJEString( *localConf, "privateKey", WJE_GET, "" )[0] == 0 ) {
		init_create_keys( localConf );
	}

	// When ID is set, check if it is still valid
	if( WJEString( *localConf, "id", WJE_GET, "" )[0] != 0 ) {
		if( init_check_ap( ctrl, localConf ) ) return 1;
	}

	// When no ID is set, register a new AP
	if( WJEString( *localConf, "id", WJE_GET, "" )[0] == 0 ) {
		if( init_register_ap( ctrl, localConf ) ) return 1;
	}



	//// SAVE CONFIG FILE

	f = fopen( path, "w" );
	WJWriter writer = WJWOpenFILEDocument( TRUE, f );
	WJEWriteDocument( *localConf, writer, NULL );
	WJWCloseDocument( writer );
	fclose( f );


	return 0;

}
