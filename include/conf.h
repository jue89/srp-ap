#ifndef CONF_H
#define CONF_H

/* 
 * CONFIG FETCH FUNCTION
 * Parameter:
 *   ctrl: CTRL handle
 *   localConf: Local config object
 *   conf: Config object
 *   path: Path to JSON file for local config
 * Returns:
 *   0: No config changed
 *   1: Changed
 */

extern int conf_fetch( CTRL *ctrl, WJElement *localConf, WJElement *conf, const char *path );

#endif
