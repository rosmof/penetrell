/************************************************

 *  Created on: Sep 2, 2018
 *      Author: AlexandruG

 ************************************************/

#include "persistence.h"

#include <sqlite3.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
//#include "parse.h"

const char* colname_list[] = {"query_id", "judet", "localitate", "cod_postal", "institutia", "created"};

const char* penetrel_sql_createtable = "CREATE TABLE `index` ( `id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                                       "`query_id` INTEGER NOT NULL, `judet` TEXT NOT NULL, `localitate` TEXT NOT "
                                       "NULL, `cod_postal` TEXT NOT NULL, `institutia` TEXT NOT NULL, `created` "
                                       "NUMERIC NOT NULL )";

const char* penetrel_sql_insert =
        "INSERT INTO "
        "'index' ('query_id','judet','localitate','cod_postal','institutia','created') values "
        "(?, ?, ?, ?, ?,?);";

constexpr const char* penetrel_sql_delete = "DELETE FROM 'index' where query_id = %d;";
const int delen = strlen(penetrel_sql_delete);

const char* penetrel_sql_checktable =
        "select count(type) as result from sqlite_master where type='table' and name='index'";

static int sql_callback(void* type, int argc, char** argv, char** colname) {
    sqlarg* a = (sqlarg*)type;

    if (strcmp(a->qtype, "table_exists") == 0) {
        if (argc == 1) {
            if (strcmp(colname[0], "result") == 0) {
                a->res = atoi(argv[0]);
            }
        }
    }

    return 0;
}

bool initialize_database() {
    char root[256];
    bzero(root, 256);
    getcwd(root, 256);
    sprintf(root + strlen(root), "%s", "/database/penetrell.db");

    return get_dbx(root) && create_table();
}

sqlite3* get_dbx(const char* name) {
    if (!objdb.dbx) {
        create_dbdir(&name);
        int err = 0;
        if ((err = sqlite3_open_v2(name, &objdb.dbx, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr))
                != SQLITE_OK) {
            printf("failed to open database [%s]\n", sqlite3_errstr(err));
            return nullptr;
        }
    }

    return objdb.dbx;
}

bool database_exists(const char* name) {
    struct stat def;
    return stat(name, &def) == 0;
}

bool create_dbdir(const char** name) {
    char* find = (char*)strstr(*name, "penetrell.db");
    --find; // escape '/'

    char* res = (char*)calloc(strlen(*name) - strlen(find) + 1, sizeof(char));
    memcpy(res, *name, strlen(*name) - strlen(find));

    errno = 0;
    if (mkdir(res, S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
        if (errno != EEXIST) {
            printf("failed to create database directory [%s] [%d]\n", strerror(errno), errno);
        }
    }

    return true;
}

bool create_table() {
    int rc = 0;
    if (!table_exists()) {
        if ((rc = sqlite3_exec(objdb.dbx, penetrel_sql_createtable, nullptr, nullptr, nullptr)) != SQLITE_OK) {
            printf("failed to create sql table [%s]\n", sqlite3_errstr(rc));
            return false;
        }
    }

    return true;
}

bool table_exists() {
    int rc = 0;
    sqlarg arg;
    arg.qtype = "table_exists";
    if ((rc = sqlite3_exec(objdb.dbx, penetrel_sql_checktable, sql_callback, (void*)&arg, nullptr)) != SQLITE_OK) {
        printf("failed to execute check table sql [%s]\n", sqlite3_errstr(rc));
        return false;
    }

    return arg.res;
}

int save_tablerows(table_row_list* list, int idx, bool delete_before) {
    if (delete_before)
        delete_foridx(idx);

    return save_tablerows_exec(list, idx);
}

int save_tablerows_exec(table_row_list* list, int idx) {

    if (!objdb.stm) {
        int rc;
        if ((rc = sqlite3_prepare_v2(objdb.dbx, penetrel_sql_insert, strlen(penetrel_sql_insert), &objdb.stm, nullptr))
                != SQLITE_OK) {
            printf("failed to prepare statement [%s]\n", sqlite3_errstr(rc));
            return -1;
        }
    }

    sql_begintx();

    table_row* prow;
    for (int i = 0; i < list->n; i++) {
        prow = list->prow[i];
        printf("current: %s %s %s %s\n", prow->cod, prow->institutie, prow->judet, prow->strada);
        sqlite3_bind_int64(objdb.stm, 1, idx);
        sqlite3_bind_text(objdb.stm, 2, prow->judet, strlen(prow->judet), nullptr);
        sqlite3_bind_text(objdb.stm, 3, prow->strada, strlen(prow->strada), nullptr);
        sqlite3_bind_text(objdb.stm, 4, prow->cod, strlen(prow->cod), nullptr);
        sqlite3_bind_text(objdb.stm, 5, prow->institutie, strlen(prow->institutie), nullptr);
        sqlite3_bind_int64(objdb.stm, 6, time(0));

        int rc;
        if ((rc = sqlite3_step(objdb.stm)) != SQLITE_OK) {
            if (rc != SQLITE_DONE) {
                printf("rc = %d %s\n", rc, sqlite3_errstr(rc));
                sql_rollback();
            }
        }

        if ((rc = sqlite3_clear_bindings(objdb.stm)) != SQLITE_OK) {
            printf("failed to clear binding; will rollbak\n");
            sql_rollback();
        }

        sqlite3_reset(objdb.stm);
    }

    sql_endtx();

    return list->n;
}

bool sql_exec(const char* cmd) {
    int rc = 0;
    if ((rc = sqlite3_exec(objdb.dbx, cmd, sql_callback, nullptr, nullptr)) != 0) {
        printf("failed to %s transaction [%d] [%s]\n", cmd, rc, sqlite3_errstr(rc));
        return false;
    }

    return true;
}

bool delete_foridx(int idx) {
    char cmd[delen + 1];
    bzero(cmd, delen + 1);
    sprintf(cmd, penetrel_sql_delete, idx);

    return sql_exec(cmd);
}

bool sql_begintx() {
    return sql_exec("BEGIN TRANSACTION;");
}

bool sql_endtx() {
    return sql_exec("END TRANSACTION;");
}

bool sql_commit() {
    return sql_exec("COMMIT;");
}

bool sql_rollback() {
    return sql_exec("ROLLBACK;");
}
