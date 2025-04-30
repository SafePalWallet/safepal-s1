#ifndef _DBCTRL_H
#define _DBCTRL_H

#ifdef __cplusplus
extern "C" {
#endif

int dbc_open(const char *db_path);

int dbc_close();

int dbc_executeSQL(const char *sql);

int dbc_getCount(const char *table, const char *where);

int dbc_execSelectSQL(const char *sql);

int dbc_getResult();

int dbc_getColumnInt(int col);

long long dbc_getColumnInt64(int col);

const char *dbc_getColumnText(int col);

const void *dbc_getColumnBlob(int col, int *size);

int dbc_endSelectSQL();

int dbc_prepareSql(const char *sql);

int dbc_bindBlob(int index, const void *blob, int len);

int dbc_bindText(int index, const char *txt);

int dbc_bindTextL(int index, const char *txt, int len);

int dbc_execStmt();

#ifdef __cplusplus
}
#endif

#endif
