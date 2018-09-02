/************************************************

 *  Created on: Sep 2, 2018
 *      Author: AlexandruG

 ************************************************/

#ifndef SRC_PERSISTENCE_H_
#define SRC_PERSISTENCE_H_

#include "parse.h"

class sqlite3; // forward declaration

/**
 * Structure to be passed to sql callback function
 *  */
typedef struct {
    const char* qtype;
    int res;
} sqlarg;

static sqlite3* dbx = nullptr;

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

#endif
