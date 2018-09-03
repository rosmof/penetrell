/************************************************

 *  Created on: Sep 2, 2018
 *      Author: AlexandruG

 ************************************************/

#ifndef SRC_PERSISTENCE_H_
#define SRC_PERSISTENCE_H_

#include "parse.h"

// struct _table_row_list;                 // forward declaration (1)
// typedef table_row_list _table_row_list; // forward declaration (2)
struct sqlite3;      // forward declaration
struct sqlite3_stmt; // forward declaration

/**
 * Structure to be passed to sql callback function
 **/
typedef struct {
    const char* qtype;
    int res;
} sqlarg;

typedef struct _database_objects {
    sqlite3* dbx = nullptr;
    sqlite3_stmt* stm = nullptr;

} dbxobj;

static dbxobj objdb;

// static sqlite3* dbx = nullptr;

/**
 * Unified method to initialize the database, including creation and
 * table definition.
 **/
bool initialize_database();

/**
 * Checks if the database exists;
 *  */
bool database_exists(const char* name);

/**
 * Creates the database dir
 *  */
bool create_dbdir(const char** name);

/**
 * Checks if the table with results exists;
 *  */
bool table_exists();

/**
 * Creates the default index table;
 *  */
bool create_table();

/**
 * Returns the static sqlite3* pointer;
 *  */
sqlite3* get_dbx(const char* name);

/**
 * Prior to saving, this function also checks if the data is recorded
 *  */
int save_tablerows(table_row_list* list, int idx, bool delete_before);

/**
 * Execution for save_tablerow function
 **/
int save_tablerows_exec(table_row_list* list, int idx);

/**
 * Deletes all the records for an idx; this is done before any
 * insertion;
 *  */
bool delete_foridx(int idx);

bool sql_begintx();

bool sql_endtx();

bool sql_commit();

bool sql_rollback();

#endif
