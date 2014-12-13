#ifndef INIT_H
#define INIT_H

/* 
 * INIT FUNCTION
 * Parameter:
 *   ctrl: CTRL handle
 *   localConf: Local config object
 *   path: Path to JSON file for local config
 * Returns:
 *   0: SUCCESS
 *   1: FAIL
 */

extern int init( CTRL *ctrl, WJElement *localConf, const char *path );

#endif
