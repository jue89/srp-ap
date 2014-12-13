#ifndef CTRL_H
#define CTRL_H

/*
 * CTRL Handle
 */

typedef struct {
	char username[32];
	char password[32];
	char caCert[128];
	char baseUrl[128];
} CTRL;



/* 
 * CTRL INIT FUNCTION
 * Parameter:
 *   baseUrl: base URL for each request
 *   caCert: Path to the CA of the controller
 *   username: What you expact ...
 *   password: ...
 * Returns: CTRL handle
 */

extern CTRL * ctrl_init( const char *baseUrl, const char *caCert, const char *username, const char *password );



/* 
 * CTRL FREE FUNCTION
 * Parameter:
 *   ctrl: Pointer to ctrl handle
 */

extern void ctrl_free( CTRL *ctrl );



/*
 * CTRL GET FUNCTION
 * Parameter:
 *   ctrl: Pointer to ctrl handle
 *   endpoint: Name of endpoint
 *   out: Response object
 * Returns: HTTP code or 0 on error
 */

extern long ctrl_get( const CTRL *ctrl, const char *endpoint, WJElement *out );
extern long ctrl_get_etag( const CTRL *ctrl, const char *endpoint, char *etag, WJElement *out );



/*
 * CTRL POST FUNCTION
 * Parameter:
 *   ctrl: Pointer to ctrl handle
 *   endpoint: Name of endpoint
 *   in: Request object
 *   out: Response object
 * Returns: HTTP code or 0 on error
 */

extern long ctrl_post( const CTRL *ctrl, const char *endpoint, WJElement *in, WJElement *out );



/*
 * CTRL PUT FUNCTION
 * Parameter:
 *   ctrl: Pointer to ctrl handle
 *   endpoint: Name of endpoint
 *   in: Request object
 *   out: Response object
 * Returns: HTTP code or 0 on error
 */

extern long ctrl_put( const CTRL *ctrl, const char *endpoint, WJElement *in, WJElement *out );



/*
 * CTRL DELETE FUNCTION
 * Parameter:
 *   ctrl: Pointer to ctrl handle
 *   endpoint: Name of endpoint
 * Returns: HTTP code or 0 on error
 */

extern long ctrl_delete( const CTRL *ctrl, const char *endpoint );


#endif
