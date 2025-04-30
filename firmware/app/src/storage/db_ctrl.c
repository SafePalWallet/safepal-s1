#define LOG_TAG "db_ctrl"

#include <unistd.h>
#include <strings.h>
#include "common.h"
#include "db_ctrl.h"
#include "sqlite3.h"

#define PRINT_DB_ERROR(x) db_error("db error code=%d msg=%s", sqlite3_errcode(x),sqlite3_errmsg(x));

#define DEBUG_SQL
#define BEGIN_TRANSACTION "begin transaction"
#define COMMIT_TRANSACTION "commit transaction"

typedef struct sqlite3 SQLCon;

SQLCon *mSQLCon;
sqlite3_stmt *mSqlstmt;

static int G_lock = 0;

static inline void
LOCK() {
	while (__sync_lock_test_and_set(&(G_lock), 1)) {}
}

static inline void
UNLOCK() {
	__sync_lock_release(&(G_lock));
}

int dbc_open(const char *db_path) {
	int ret;
	mSQLCon = NULL;
	dbc_close(); //close force
	ret = sqlite3_open(db_path, &mSQLCon);
	db_debug("sqlite3_open: mSQLCon:%p ret:%d", mSQLCon, ret);
	if (SQLITE_OK != ret) {
		PRINT_DB_ERROR(mSQLCon);
		mSQLCon = NULL;
	}
	return ret;
}

int dbc_close() {
	int ret = 0;
	LOCK();
	if (mSQLCon) {
		ret = sqlite3_close(mSQLCon);
		if (SQLITE_OK != ret) {
			PRINT_DB_ERROR(mSQLCon);
		}
		mSQLCon = NULL;
	}
	mSqlstmt = NULL;
	UNLOCK();
	return ret;
}

int dbc_executeSQL(const char *sql) {
	char *errorMsg = NULL;
	int errorCode;
	if (!mSQLCon) {
		db_error("mSQLCon is null");
		return -1;
	}
	LOCK();
	if (!mSQLCon) {
		db_error("mSQLCon is null");
		UNLOCK();
		return -1;
	}
#if 0
	int transaction = 0;
	if (transaction) {
		errorCode = sqlite3_exec(mSQLCon, BEGIN_TRANSACTION, NULL, NULL, &errorMsg);
		if (SQLITE_OK != errorCode) {
			PRINT_DB_ERROR(mSQLCon);
			UNLOCK();
			return errorCode;
		}
	}
#endif
	errorCode = sqlite3_exec(mSQLCon, sql, NULL, NULL, &errorMsg);
#ifdef DEBUG_SQL
	db_msg("sql:%s", sql);
#endif
	if (SQLITE_OK != errorCode) {
		PRINT_DB_ERROR(mSQLCon);
	}
#if 0
	if (transaction) {
		errorCode = sqlite3_exec(mSQLCon, COMMIT_TRANSACTION, NULL, NULL, &errorMsg);
		if (SQLITE_OK != errorCode) {
			PRINT_DB_ERROR(mSQLCon);
		}
	}
#endif
	UNLOCK();
	return errorCode;
}

int dbc_getCount(const char *table, const char *where) {
	int recordNum = -1;
	char sql[128];
	if (where && strlen(where) > 0) {
		snprintf(sql, sizeof(sql), "SELECT COUNT(*) c FROM %s where %s", table, where);
	} else {
		snprintf(sql, sizeof(sql), "SELECT COUNT(*) c FROM %s", table);
	}

	int ret = dbc_execSelectSQL(sql);
	if (ret != 0) {
		return -1;
	}
	ret = dbc_getResult();
	if (ret == 0) {
		recordNum = dbc_getColumnInt(0);
	}
	dbc_endSelectSQL();
	return recordNum;
}

int dbc_execSelectSQL(const char *sql) {
	if (!mSQLCon) {
		db_error("mSQLCon is null");
		return -1;
	}
	const char *errorMsg = NULL;
	LOCK();
	if (!mSQLCon) {
		db_error("mSQLCon is null");
		return -1;
	}
#ifdef DEBUG_SQL
	db_msg("sql:%s", sql);
#endif
	int errorCode = sqlite3_prepare_v2(mSQLCon, sql, -1, &mSqlstmt, &errorMsg);
	if (SQLITE_OK != errorCode) {
		PRINT_DB_ERROR(mSQLCon);
		db_error("error sql:%s msg:%s", sql, errorMsg);
		UNLOCK();
		return -1;
	}
	return 0;
}

int dbc_getResult() {
	if (mSqlstmt == NULL) {
		return -1;
	}
	int errorCode = sqlite3_step(mSqlstmt);
	if (SQLITE_ROW == errorCode) {
		return 0;
	} else if (SQLITE_DONE == errorCode) {
		return SQLITE_DONE;
	} else {
		PRINT_DB_ERROR(mSQLCon);
		return -1;
	}
}

int dbc_getColumnInt(int col) {
	return sqlite3_column_int(mSqlstmt, col);
}

long long dbc_getColumnInt64(int col) {
	return sqlite3_column_int64(mSqlstmt, col);
}

const char *dbc_getColumnText(int col) {
	return (const char *) sqlite3_column_text(mSqlstmt, col);
}

const void *dbc_getColumnBlob(int col, int *size) {
	const void *b = sqlite3_column_blob(mSqlstmt, col);
	if (size) {
		*size = b ? sqlite3_column_bytes(mSqlstmt, col) : 0;
	}
	return b;
}

int dbc_endSelectSQL() {
	if (mSqlstmt != NULL) {
		sqlite3_finalize(mSqlstmt);
		mSqlstmt = NULL;
	}
	UNLOCK();
	return 0;
}

int dbc_prepareSql(const char *sql) {
	return dbc_execSelectSQL(sql);
}

int dbc_bindBlob(int index, const void *blob, int len) {
	return sqlite3_bind_blob(mSqlstmt, index, blob, len, NULL);
}

int dbc_bindText(int index, const char *txt) {
	return sqlite3_bind_text(mSqlstmt, index, txt, -1, NULL);
}

int dbc_bindTextL(int index, const char *txt, int len) {
	return sqlite3_bind_text(mSqlstmt, index, txt, len, NULL);
}

int dbc_execStmt() {
	int ret = dbc_getResult();
	dbc_endSelectSQL();
	return (ret == 0 || ret == SQLITE_DONE) ? 0 : ret;
}
