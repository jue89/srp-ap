#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <wjelement.h>
#include "../include/ctrl.h"

 
/*
 * BUFFER FUNCTIONS
 */

// Buffer for receiving and sending data
struct Data {
	char *memory;
	size_t size;
	size_t sent;
};

// Allocate a new buffer
struct Data * ctrl_alloc_buffer( size_t size ) {
	struct Data *buffer = malloc( sizeof( struct Data ) );

	buffer->memory = malloc( size + 1 );
	buffer->sent = 0;
	buffer->size = size;

	return buffer;
}

// Free a buffer
static void ctrl_free_buffer( struct Data *buffer ) {
	free( buffer->memory );
	free( buffer );
}


// Initialises ctrl
CTRL * ctrl_init( const char *baseUrl, const char *caCert, const char *username, const char *password ) {
	
	// Create a new handle
	CTRL *ctrl = malloc( sizeof( CTRL ) );
	
	// Copy configuration
	strncpy( ctrl->baseUrl, baseUrl, 128 );
	strncpy( ctrl->caCert, caCert, 128 );
	strncpy( ctrl->username, username, 32 );
	strncpy( ctrl->password, password, 32 );

	// Return the handle
	return ctrl;

}


// Frees ctrl
void ctrl_free( CTRL *ctrl ) {
	free( ctrl );
}


// Callback for request body
static size_t ctrl_callback_read( void *dest, size_t size, size_t nmemb, void *handle ) {
	struct Data *src = (struct Data *) handle;
	size_t bytes = size * nmemb;
	size_t left = src->size - src->sent;

	if( bytes > left ) bytes = left;
	if( bytes == 0 ) return 0;

	memcpy( dest, &(src->memory[src->sent]), bytes );
	src->sent += bytes;
	
	return bytes;
}


// Callback for response body
static size_t ctrl_callback_write( void *contents, size_t size, size_t nmemb, void *handle ) {
	size_t bytes = size * nmemb;
	struct Data *data = (struct Data *) handle;

	// Make some space for received data
	data->memory = realloc( data->memory, data->size + bytes + 1);
	if( data->memory == NULL ) {
		// We're out of memory
		fprintf( stderr, "not enough memory (realloc returned NULL)\n" );
		return 0;
	}

	// Copy data from receive buffer to memory
	memcpy( &(data->memory[data->size]), contents, bytes );
	data->size += bytes;
	data->memory[data->size] = 0;

	return bytes;

}


// Callback for response header
static size_t ctrl_callback_header( char *buffer, size_t size, size_t nitems, void *handle ) {
	size_t bytes = size * nitems;
	WJElement *data = (WJElement *) handle;

	char *pos;
	char field[128], value[128];

	// Find delimiter
	pos = strstr( buffer, ":" );
	// When not found: skip header
	if( pos == NULL ) return bytes;

	// Length of field
	size_t len = (size_t)( pos - buffer );

	// Copy and terminate field
	memcpy( field, buffer, len );
	field[len] = 0;

	// Skip space when present
	if( pos[1] == ' ' ) pos+=2;

	// Length of value
	len = bytes - len - 4;

	// Copy and terminate value
	memcpy( value, pos, len );
	value[len] = 0;

	// Set JSON object
	WJEObject( *data, "headers[$]", WJE_NEW);
	WJEString( *data, "headers[-1].field", WJE_SET, field );
	WJEString( *data, "headers[-1].value", WJE_SET, value );


	return bytes;

}


// Callback for JSON stringify
static size_t ctrl_callback_json( char *contents, size_t len, void *handle ) {

	struct Data *data = (struct Data *) handle;

	// Make some space for received data
	data->memory = realloc( data->memory, data->size + len + 1);
	if( data->memory == NULL ) {
		// We're out of memory
		fprintf( stderr, "not enough memory (realloc returned NULL)\n" );
		return 0;
	}

	// Copy data from receive buffer to memory
	memcpy( &(data->memory[data->size]), contents, len );
	data->size += len;
	data->memory[data->size] = 0;

	return len;
}


// Generic request function
static long ctrl_request( const CTRL *ctrl, const char *endpoint, const char *method,
                          WJElement *reqHeader, WJElement *in,
                          WJElement *resHeader, WJElement *out ) {

	// Init cURL
	CURL *curl;
	curl_global_init( CURL_GLOBAL_DEFAULT );

	// Create easy cURL handle and return failure when something went wrong
	curl = curl_easy_init();
	if( ! curl ) return 0;


	//// PREPARE REQUEST

	// Ensure that the servers certificate is checked
	curl_easy_setopt( curl, CURLOPT_CAINFO, ctrl->caCert );

	// Set request method
	curl_easy_setopt( curl, CURLOPT_CUSTOMREQUEST, method );

	// Set URL
	char url[256];
	strcpy( url, ctrl->baseUrl );
	strcat( url, endpoint );
	curl_easy_setopt( curl, CURLOPT_URL, url );

	// Auth
	curl_easy_setopt( curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC );
	curl_easy_setopt( curl, CURLOPT_USERNAME, ctrl->username );
	curl_easy_setopt( curl, CURLOPT_PASSWORD, ctrl->password );
	
	// Set buffers for response
	struct Data *receiveBuffer = ctrl_alloc_buffer( 0 );
	curl_easy_setopt( curl, CURLOPT_WRITEDATA, (void *) receiveBuffer );
	curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, ctrl_callback_write );

	// Prepare header
	struct curl_slist *h = NULL;
	if( reqHeader ) {
		WJElement item = NULL;
		while( ( item = _WJEObject( *reqHeader, "headers[]", WJE_GET, &item ) ) ) {
			char line[128];
			strcpy( line, WJEString( item, "field", WJE_GET, "" ) );
			strcat( line, ": " );
			strcat( line, WJEString( item, "value", WJE_GET, "" ) );
			h = curl_slist_append( h, line );
		}
	}

	// Set buffers and headers for request, when needed
	struct Data *sendBuffer = ctrl_alloc_buffer( 0 );
	if( in != NULL ) {
		// Convert JSON
		WJWriter writer = _WJWOpenDocument( FALSE, ctrl_callback_json, sendBuffer, 0 );
		WJEWriteDocument( *in, writer, NULL );
		WJWCloseDocument( writer );

		// Set content type header
		h = curl_slist_append( h, "Content-Type: application/vnd.api+json" );

		// Make sure that data is sent by read callback ( POSTFIELDS -> NULL )
		curl_easy_setopt( curl, CURLOPT_POST, TRUE );
		curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE, sendBuffer->size );
		curl_easy_setopt( curl, CURLOPT_POSTFIELDS, NULL );

		// Set read callback
		curl_easy_setopt( curl, CURLOPT_READDATA, (void *) sendBuffer );
		curl_easy_setopt( curl, CURLOPT_READFUNCTION, ctrl_callback_read );
	}

	// Set header
	curl_easy_setopt( curl, CURLOPT_HTTPHEADER, h );

	// Response header
	if( resHeader ) {
		*resHeader = WJEObject( NULL, NULL, WJE_NEW );
		WJEObject( *resHeader, "headers", WJE_NEW );
		curl_easy_setopt( curl, CURLOPT_HEADERDATA, (void *) resHeader );
		curl_easy_setopt( curl, CURLOPT_HEADERFUNCTION, ctrl_callback_header );
	}

	//curl_easy_setopt( curl, CURLOPT_VERBOSE, TRUE );
	
	
	//// AND HERE WE GO
	CURLcode res;
	res = curl_easy_perform( curl );
	if( res != CURLE_OK ) {
		fprintf( stderr, "%s\n", curl_easy_strerror( res ) );
		return 0;
	}


	//// CLEAN UP
	
	// Response code
	long code;
	curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &code);

	// Read JSON
	WJReader reader = WJROpenMemDocument( receiveBuffer->memory, NULL, 0 );
	*out = WJEOpenDocument( reader, NULL, NULL, NULL );
	WJRCloseDocument( reader );

	// Cleaning up
	ctrl_free_buffer( sendBuffer );
	ctrl_free_buffer( receiveBuffer );
	curl_slist_free_all( h );
	curl_easy_cleanup( curl );
	
	// When unauth --> generic error
	if( code == 401 ) fprintf( stderr, "Wrong credentials!\n" );

	return code;
	
}

long ctrl_get( const CTRL *ctrl, const char *endpoint, WJElement *out ) {
	return ctrl_request( ctrl, endpoint, "GET", NULL, NULL, NULL, out );
}

long ctrl_get_etag( const CTRL *ctrl, const char *endpoint, char *etag, WJElement *out ) {
	WJElement req = WJEObject( NULL, NULL, WJE_NEW );
	WJEObject( req, "headers", WJE_NEW );
	WJEObject( req, "headers[$]", WJE_NEW);
	WJEString( req, "headers[-1].field", WJE_SET, "If-None-Match" );
	WJEString( req, "headers[-1].value", WJE_SET, etag );

	WJElement res;

	long code = ctrl_request( ctrl, endpoint, "GET", &req, NULL, &res, out );

	WJElement item = NULL;
	while( ( item = _WJEObject( res, "headers[]", WJE_GET, &item ) ) ) {
		if( strcmp( WJEString( item, "field", WJE_GET, "" ), "ETag" ) == 0 ) {
			strcpy( etag, WJEString( item, "value", WJE_GET, "" ) );
		}
	}

	WJECloseDocument( req );
	WJECloseDocument( res );

	return code;
}

long ctrl_post( const CTRL *ctrl, const char *endpoint, WJElement *in, WJElement *out ) {
	return ctrl_request( ctrl, endpoint, "POST", NULL, in, NULL, out );
}

long ctrl_put( const CTRL *ctrl, const char *endpoint, WJElement *in, WJElement *out ) {
	return ctrl_request( ctrl, endpoint, "PUT", NULL, in, NULL, out );
}

long ctrl_delete( const CTRL *ctrl, const char *endpoint ) {
	WJElement out;

	long code = ctrl_request( ctrl, endpoint, "DELETE", NULL, NULL, NULL, &out );

	WJECloseDocument( out );

	return code;
}

