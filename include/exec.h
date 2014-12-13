#ifndef EXEC_H
#define EXEC_H

extern int exec_check( WJElement *conf, char *path );
extern int exec_commit( WJElement *conf, char *path );
extern int exec_revert( char *path );

#endif

