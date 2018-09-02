/************************************************

 *  Created on: Aug 25, 2018
 *      Author: AlexandruG

 ************************************************/

#ifndef SRC_PARSE_H_
#define SRC_PARSE_H_

#include <map>
#include <unistd.h>
#include <tidy.h>

#include "form_data.h"

struct post_key {
    char key[96];
    char value[5012];
};

struct post_key_list {
    post_key** keys;
    size_t items;
};

struct memory_block {
    char* buffer;
    size_t size;
};

struct header_block {
    int http_status;
    size_t response_len;
};

struct href_next {
    char id[128];
    char next_row[3];
    const char* next_row_fmt = "dvt_firstrow={%s}\0";
};

/**
 * Structure used to save a row from the parsed table
 **/
typedef struct {
    char judet[64];
    char strada[256];
    char cod[8];
    char institutie[128];
} table_row;

typedef struct {
    table_row** prow;
    size_t n;
} table_row_list;

static const char* href_start_marker = "javascript: __doPostBack\0"; // used to parse the next_href

/***************************************************************************************************************
 **
 **					GENERIC GET DATA SECTION OF FILE
 **/
/***************************************************************************************************************/

int add_post_key(post_key_list* pklist, post_key* pk, size_t size);

void free_pklist(post_key_list* pklist);

void update_eventargument(post_key_list* pklist, size_t gotn);

/**
 * Parses all the keys that are inside the GET HTML response.
 *
 * Returns true if there are more records, meaning that succesive POST must be submitted
 * and false if the page has all the fields (less than 30 as i observed)
 *
 *  */
bool get_form_fields(post_key_list* pklist, const char* html);

/**
 * Parses the HTML with libtidy and extracts the data from the table into
 * the table_row_list structure that is passed as pointer;
 *
 * @return 1 if there are more pages; 0 if this is the last page; -1 on error.
 *
 **/
int get_table_data(table_row_list* prlist, const char* html);

void free_table_data(table_row_list* trlist);

/***************************************************************************************************************
 **
 **					PARSE SECTION OF FILE
 **/
/***************************************************************************************************************/

enum parse_option { HREF_NEXT = 1, EVENT_VALIDATION = 2, REQUEST_DIGEST = 3 };

typedef post_key* (*parse_func)(const char* raw_html, void* parse_object_type, char* separator, size_t* result_size);

post_key* parse(parse_option popt, const char* html, void* ptype, char* separator, size_t* len);

/**
 * This is the main function for parsing form attributes from the html response.
 * Initially the parse function was used as a generic means to achieve bla bla,
 * but i changed it; all the attributes are marked with id="" and so finding is
 * relative easy;
 *
 *  */
post_key* parse_form_field(const char* html, const char* fieldname);

/**
 * Some generic function that parses the raw html and gets the value between the
 * separator. The value is returned in the buffer pointer.
 *  */
void get_value(const char* html, const char* start_criteria, const char* separator, const char* end, char* buffer,
        size_t bufflen);

/**
 * Extracts the id for the post that is included in next button.
 * href_next::id is filled with the id (between ' ' in source code) and the id
 * is the next number
 *  */
post_key* parse_next_href(const char* html, void* pnext, char* separator, size_t* len);

int parse_html_table(TidyDoc tdoc, table_row_list* prlist, const char* html);

void find_starting_node_post(TidyDoc tdoc, TidyNode node);

/**
 * Function used to check if there are more pages
 *
 * @return true if there are, false otherwise
 *  */
bool has_next(const char* html);

/**********************************************************************************************************************
 ** Actually iterates through the table tag and extracts the table values to the
 ** table_row_list pointer structure.
 **
 ** @param tdoc, the TidyDocument;
 **
 ** @param ref, the starting tag to search forward from;
 **
 ** @param prlist, the pointer to the list of table rows that will be extracted from the page
 **
 ** @return true on success, false on fail;
 ***********************************************************************************************************************/
bool extract_html_table(TidyDoc tdoc, TidyNode ref, table_row_list* prlist);

bool is_th_tag(TidyNode node);

static std::map<parse_option, parse_func> parse_func_repository = {{parse_option::HREF_NEXT, parse_next_href}};

#define FUNCTION_REPOSITORY_PARSE parse_func_repository

/***************************************************************************************************************
 **
 **					CONVERSION SECTION OF FILE
 **/
/***************************************************************************************************************/

enum conversion_option { HREFNEXT_TO_POSTKEY = 1, EVENTVALID_TO_POSTKEY = 2, REQUESTDIGEST_TO_POSTKEY = 3 };

/**
 * Typedef function definition for generic conversion mechanism.
 *  */
typedef post_key* (*conversion_func)(const void* argument, size_t* len);

/**
 * Generic function that must be used for conversion. Depending on the
 * conversion option the correct conversion function will be called from the
 * function repository.
 *  */
post_key* convert(const conversion_option copt, void* src, size_t* len);

/**
 * Converts a href_next parsed object to post_key so that it will be easily
 *inserted in the post request.
 *
 * @param len is a pointer that will be filled by the function with the number
 *of elements.
 *
 * @return a list of post_key objects; the length of the list will be set
 *through len argument.
 **/
post_key* convert_hrefnext_to_postkey(const void* pnext, size_t* len);

/**
 * This is the conversion function repository. The conversion steps, from a raw
 * object parsed from the html page and a post_key will be done: a) call convert
 * generic function with the correct option; b) inside the convert function,
 * call call_conversion_function with the conversion option; c) the
 * call_conversion_function will get the function from this map;
 *  */
static std::map<conversion_option, conversion_func> convert_func_repository = {
        {conversion_option::HREFNEXT_TO_POSTKEY, convert_hrefnext_to_postkey}};

#define FUNCTION_REPOSITORY_CONVERT convert_func_repository

#endif
