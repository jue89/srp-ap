#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wjelement.h>
#include <getopt.h>
#include "../include/ctrl.h"
#include "../include/init.h"
#include "../include/conf.h"
#include "../include/exec.h"


int main( int argc, char *argv[] ) {

	// Read command line options
	static struct option long_options[] = {
		{ "urlbase",  required_argument, 0, 'b' },
		{ "cacert",   required_argument, 0, 'c' },
		{ "username", required_argument, 0, 'u' },
		{ "password", required_argument, 0, 'p' },
		{ "config",   required_argument, 0, 'C' },
		{ "scripts",  required_argument, 0, 'S' },
		{ "revert",   no_argument,       0, 'r' },
		{ "dryrun",   no_argument,       0, 'n' },
		{ 0, 0, 0, 0}
	};
	int revert = 0;
	int dryrun = 0;
	char *baseUrl = "https://api.13pm.eu/v1.0/";
	char *caCert = "/etc/api/cacert.pem";
	char *username = "";
	char *password = "";
	char *config = "/etc/api";
	char *scripts = "/etc/api";
	int option_index = 0;
	int c;
	while( ( c = getopt_long (argc, argv, "rnb:c:u:p:C:S:", long_options, &option_index) ) != -1 ) {
		switch( c ) {
			case 'r': revert = 1; break;
			case 'n': dryrun = 1; break;
			case 'b': baseUrl = optarg; break;
			case 'c': caCert = optarg; break;
			case 'u': username = optarg; break;
			case 'p': password = optarg; break;
			case 'C': config = optarg; break;
			case 'S': scripts = optarg; break;
		}
	}

	// Revert all configurations?
	if( revert ) {
		// Build path to script
		char scriptPath[128];
		strcpy( scriptPath, scripts );
		strcat( scriptPath, "/revert.sh" );

		return exec_revert( scriptPath );
	}

	// Check if all needed options are present
	if( ! strcmp( username, "" ) || ! strcmp( password, "" ) ) {
		fprintf( stderr, "Username and password are needed!\n" );
		return 1;
	}

	// Init controller access
	CTRL *ctrl = ctrl_init( baseUrl, caCert, username, password );


	// Read local config
	WJElement localConf;
	char localConfPath[128];
	strcpy( localConfPath, config );
	strcat( localConfPath, "/localConf.json" );
	if( init( ctrl, &localConf, localConfPath ) != 0 ) {
		fprintf( stderr, "Local config was not successful!\n" );
		return 1;
	}


	// Dry run won't make any changes to the configuration --> Skip
	if( dryrun ) {
		// Clean up
		WJECloseDocument( localConf );
		ctrl_free( ctrl );

		return 0;
	}

	

	// Fetch configuration
	WJElement conf;
	char confPath[128];
	strcpy( confPath, config );
	strcat( confPath, "/conf.json" );
	if( conf_fetch( ctrl, &localConf, &conf, confPath ) ) {
		// Something has changed

		// Check transmitted config against schema
		char schemaPath[128];
		strcpy( schemaPath, scripts );
		strcat( schemaPath, "/schema.json" );
		if( exec_check( &conf, schemaPath ) != 0 ){
			fprintf( stderr, "Transmitted config does not match schema.\n" );
			return 1;
		}
		
		// Execute revert script to remove older changes
		char scriptPath[128];
		strcpy( scriptPath, scripts );
		strcat( scriptPath, "/revert.sh" );
		if( exec_revert( scriptPath ) != 0 ) {
			fprintf( stderr, "Something went wrong with reverting old configuration.\n" );
			return 1;
		}

		// Execut commit script to apply new configuration
		strcpy( scriptPath, scripts );
		strcat( scriptPath, "/commit.sh" );
		if( exec_commit( &conf, scriptPath ) != 0 ) {
			fprintf( stderr, "Something went wrong with applying new configuration.\n" );
			return 1;
		}

	}


	// Clean up
	WJECloseDocument( localConf );
	WJECloseDocument( conf );
	ctrl_free( ctrl );


	return 0;
}

