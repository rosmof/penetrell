/************************************************

 *  Created on: Sep 2, 2018
 *      Author: AlexandruG

 ************************************************/

#include "persistence.h"

#include <sqlite3.h>
#include <stdio.h>
#include <sys/stat.h>

const char* penetrel_sql_createtable = "CREATE TABLE `index` ( `id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                                       "`query_id` INTEGER NOT NULL, `judet` TEXT NOT NULL, `localitate` TEXT NOT "
                                       "NULL, `cod_postal` TEXT NOT NULL, `institutia` TEXT NOT NULL, `created` "
                                       "NUMERIC NOT NULL )";

const char* penetrel_sql_checktable =
        "select count(type) as result from sqlite_master where type='table' and name='index'";

static int sql_callback(void* type, int argc, char** argv, char** colname) {
    sqlarg* a = (sqlarg*)type;

    if (strcmp(a->qtype, "table_exists") == 0) {
        printf("correct parameter\n");
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
    if (!dbx) {
        int err = 0;
        if ((err = sqlite3_open_v2(name, &dbx, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr)) != SQLITE_OK) {
            printf("failed to open database [%s]\n", sqlite3_errstr(err));
            return nullptr;
        }
    }

    return dbx;
}

bool database_exists(const char* name) {
    struct stat def;
    return stat(name, &def) == 0;
}

bool create_table() {
    int rc = 0;
    if (!table_exists()) {
        if ((rc = sqlite3_exec(dbx, penetrel_sql_createtable, nullptr, nullptr, nullptr)) != SQLITE_OK) {
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
    if ((rc = sqlite3_exec(dbx, penetrel_sql_checktable, sql_callback, (void*)&arg, nullptr)) != SQLITE_OK) {
        printf("failed to execute check table sql [%s]\n", sqlite3_errstr(rc));
        return false;
    }

    return arg.res;
}
