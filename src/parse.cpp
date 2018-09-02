/************************************************

 *  Created on: Aug 25, 2018
 *      Author: AlexandruG

 ************************************************/
#include <iostream>
#include <curl/curl.h>
#include <curl/easy.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include <tidybuffio.h>
#include <tidyplatform.h>

#include "parse.h"
#include "form_data.h"

static volatile bool isfound = false; // used to mark that the starting tag was found
static TidyNode found = nullptr;      // this is the found tag

/**
 * Adds pointers to a list of post_keys and updates the size attribute
 * to always know how many pointers are associated with the list.
 **/
int add_post_key(post_key_list* pklist, post_key* pk, size_t size) {

    if (pklist->items == 0) { // list not initialized
        pklist->keys = (struct post_key**)calloc(size, sizeof(struct post_key*));
        if (!pklist->keys) {
            printf("failed to get memory\n");
            return 0;
        }

    } else { // list initialized, need more space
        pklist->keys = (struct post_key**)realloc(pklist->keys, (pklist->items + size) * sizeof(struct post_key*));
    }

    // assign the pointers to the list
    for (int i = 0; i < size; i++) {
        pklist->keys[pklist->items + i] = &pk[i];
    }

    // increase the number of elements
    pklist->items += size;

    return 1;
}

/**
 * Free has a little trick, in the way that the EVENTTARGET and EVENTARGUMENT are allocated
 * with calloc and size 2; so some attention must be payed when freeing the pointers.
 **/
void free_pklist(post_key_list* pklist) {
    for (int i = 0; i < pklist->items; i++) {
        if (strncmp(pklist->keys[i]->key, "__EVENTTARGET", 13) == 0) {
            // there are 2 pointers allocated
            delete[] pklist->keys;
            i++;
            continue;
        } else {
            delete pklist->keys[i];
        }
        pklist->items--;
    }
}

/**
 * I only have to updata the EVENTARGUMENT for values above 31; The first complete
 * value is parsed from the GET response, by method
 *
 * bool get_form_fields(post_key_list* pklist, const char* html) in parse.h
 *
 *  */
void update_eventargument(post_key_list* pklist, size_t gotn) {
    if (gotn > 30) {
        for (int i = 0; i < pklist->items; i++) {
            if (strncmp(pklist->keys[i]->key, "__EVENTARGUMENT", 13) == 0) {
                char replace[32];
                bzero(replace, 32);
                sprintf(replace, nextrow_fmt, gotn);
                bzero(pklist->keys[i]->value, 5012);
                strcpy(pklist->keys[i]->value, replace);
                printf("new value: %s\n", pklist->keys[i]->value);
            }
        }
    }
}

/**
 * All the keys that must be parsed from the HTTP GET response are hard coded in the
 * formfields array; This method iterates through all the values in the array and parses
 * the values from the HTML.
 *
 * The keys will be allocated in the pklist that is submitted as argument in the function
 * signature.
 *
 * @param pklist, a pointer to the list of keys that will be allocated by the function;
 *
 * @param html, a pointer to the raw content of the HTML page
 *
 * @return if there are more pages returns true; otherwise false
 **/
bool get_form_fields(post_key_list* pklist, const char* html) {
    const char** pcurr = &formfields[0];

    while (*pcurr) {

        // the script manager value does not change and i don't feel like digging for it
        if (strstr(*pcurr, "SPECIAL_SCRIPT_MANAGER") != 0) {
            struct post_key* pk = (struct post_key*)calloc(1, sizeof(struct post_key));
            memcpy(pk->key, script_manager_key, strlen(script_manager_key));
            memcpy(pk->value, script_manager_value, strlen(script_manager_value));
            add_post_key(pklist, pk, 1);

            pcurr++;
            continue;
        }

        // handle next elements case from the list
        if (strstr(*pcurr, "SPECIAL_NEXT_ELEMENTS") != 0) {
            href_next next;
            bzero(next.id, sizeof(next.id));
            bzero(next.next_row, sizeof(next.next_row));
            size_t len;
            struct post_key* pk = parse(parse_option::HREF_NEXT, html, &next, (char*)&separator_nextkey, &len);

            // if the page has less than 30 records there is no next button
            // so there is no need to continue with the keys
            if (!pk) {
                return false;
            }

            add_post_key(pklist, pk, len);
            pcurr++;
            continue;
        }

        // NULL_HARDED_GROUP is a set of form attributes that have empty values;
        // Only the keys must be included in the POST form
        if (strstr(*pcurr, "NULL_HARDCODED_GROUP") != 0) {
            const char** pt = &hardcoded_fields[0];
            while (*pt) {
                struct post_key* k = (struct post_key*)calloc(1, sizeof(struct post_key));
                memcpy(k->key, *pt, strlen(*pt));
                add_post_key(pklist, k, 1);

                pt++;
            }
            pcurr++;
            continue;
        }

        if (strstr(*pcurr, "SPECIAL_ASYNCPOST") != 0) {
            struct post_key* k = (struct post_key*)calloc(1, sizeof(struct post_key));
            memcpy(k->key, "__ASYNCPOST", 11);
            memcpy(k->value, "true", 4);
            add_post_key(pklist, k, 1);

            pcurr++;
            continue;
        }

        struct post_key* pk = parse_form_field(html, *pcurr);
        add_post_key(pklist, pk, 1);
        pcurr++;
    }

    return true;
}

/**
 * Parses the html passed as argument and extracts the table with values
 * that is further copied to prlist; prlist is allocated on the heap, so
 * you must free the memory for each pointer.
 *
 * Returns 0 if something failed or 1 if everything is ok;
 *
 *  */
int get_table_data(table_row_list* prlist, const char* html) {

    TidyDoc tdoc;
    TidyBuffer tbuff = {0};
    TidyBuffer terr = {0};

    tdoc = tidyCreate();
    tidyOptSetBool(tdoc, TidyForceOutput, yes);
    tidyOptSetInt(tdoc, TidyWrapLen, 4096);
    tidySetErrorBuffer(tdoc, &terr);
    tidyBufInit(&tbuff);

    tidyBufAppend(&tbuff, (void*)html, strlen(html));

    int err = tidyParseString(tdoc, html);
    if (err >= 0) {
        err = tidyCleanAndRepair(tdoc);
        if (err >= 0) {
            err = tidyRunDiagnostics(tdoc);
        }
    }

    return parse_html_table(tdoc, prlist, html);
}

int parse_html_table(TidyDoc tdoc, table_row_list* prlist, const char* html) {
    find_starting_node_post(tdoc, tidyGetRoot(tdoc));

    // i couldn't make find_starting_node_post to return
    // a value or set a parameter (have no ideea why)
    // so the last resort was a static TidyNode found
    // that is used to check if find returns a value
    // or not
    if (found == nullptr) {
        printf("Starting tag not found!\n");
        printf("html: \n%s\n", html);
        return 0;
    }

    if (!(extract_html_table(tdoc, tidyGetNext(found), prlist))) {
        printf("failed to extract the table from the page!\n");
        return 0;
    }

    return 1;
}

void find_starting_node_post(TidyDoc tdoc, TidyNode current) {
    found = nullptr;
    isfound = false;
    TidyNode cursor;

    for (cursor = tidyGetChild(current); cursor; cursor = tidyGetNext(cursor)) {
        ctmbstr name = tidyNodeGetName(cursor);

        if (name) {
            if (strcmp(name, PENETREL_POST_START_TAG) == 0) {
                TidyAttr attr = tidyAttrFirst(cursor);
                if (strcmp(tidyAttrName(attr), PENETREL_POST_START_ID) == 0) {
                    const char* aval = tidyAttrValue(attr);
                    if (strcmp(aval, PENETREL_POST_START_VALUE) == 0) {
                        found = cursor;
                        isfound = true;
                        break;
                    }
                }
            }
        }

        if (!isfound) {
            find_starting_node_post(tdoc, cursor);
        }
    }
}

typedef enum { row_judet = 0, row_strada = 1, row_cod = 2, row_institutie = 3 } row_fields;

bool extract_html_table(TidyDoc tdoc, TidyNode ref, table_row_list* prlist) {

    TidyNode cursor;
    table_row* tr;

    for (cursor = tidyGetChild(ref); cursor; cursor = tidyGetNext(cursor)) {
        bool isth = false;
        TidyNode nd;
        tr = (table_row*)calloc(1, sizeof(table_row));

        // the table_row_list index that is incremented with each entry
        int index = 0;

        // the values are usually the next node inside the td
        // but sometimes there is an a tag so we have to skip it
        // with the while below
        for (nd = tidyGetChild(cursor); nd; nd = tidyGetNext(nd)) {
            isth = is_th_tag(nd);
            if (isth) {
                continue;
            }

            TidyNode valuenode = tidyGetChild(nd);
            ctmbstr value;

            int tdcounter = 0;

            // skip other tags and use a counter to stop;
            while (((value = tidyNodeGetName(valuenode)) != nullptr) && tdcounter++ < 5) {
                valuenode = tidyGetChild(valuenode);
            }

            if (tdcounter == 4) { // this means that the value tag was not found
                printf("skipped too many tags; probably bad format\n");
                return 0;
            }

            TidyBuffer buffer;
            tidyBufInit(&buffer);
            tidyNodeGetValue(tdoc, valuenode, &buffer);

            switch (index) {
                case row_judet:
                    memcpy(tr->judet, buffer.bp, strlen((char*)buffer.bp));
                    break;
                case row_strada:
                    memcpy(tr->strada, buffer.bp, strlen((char*)buffer.bp));
                    break;
                case row_cod:
                    memcpy(tr->cod, buffer.bp, strlen((char*)buffer.bp));
                    break;
                case row_institutie:
                    memcpy(tr->institutie, buffer.bp, strlen((char*)buffer.bp));
                    break;
                default:
                    printf("bad index at values\n");
                    return 0;
            }

            if (prlist->n == 0) {
                prlist->prow = (table_row**)calloc(1, sizeof(table_row*));
            } else {
                prlist->prow = (table_row**)realloc(prlist->prow, (prlist->n + 1) * sizeof(table_row*));
            }

            if (!(prlist->prow)) {
                printf("Could not assign new memory to table_row while parsing the html\n");
                return 0;
            }
            prlist->prow[prlist->n] = tr;
            index++;
        }

        if (!isth) {
            prlist->n++;
        }
    }

    return true;
}

bool is_th_tag(TidyNode node) {

    bool result = false;

    ctmbstr name = tidyNodeGetName(node);
    if (strcmp(name, "th") == 0) {
        result = true;
    }

    return result;
}

bool has_next(const char* html) {
    if (strstr(html, "next") != nullptr) {
        return true;
    }

    return false;
}

void free_table_data(table_row_list* trlist) {
    int n = trlist->n;
    for (int i = 0; i < n; i++) {
        free(trlist->prow[i]);
        trlist->n--;
    }

    delete[] trlist->prow;
}

/**
 * Creates a post_key structure from a key and value based on the fieldname passed
 * as argument.
 *
 * It calls the get_value general function.
 *  */
post_key* parse_form_field(const char* html, const char* fieldname) {
    struct post_key* key = (struct post_key*)calloc(1, sizeof(struct post_key));

    char start_marker[64];
    bzero(start_marker, 64);
    sprintf(start_marker, formfield_start_fmt, fieldname);

    get_value(html, start_marker, &separator_quotes, &separator_quotes, key->value, sizeof(key->value));
    memcpy(key->key, fieldname, strlen(fieldname));

    return key;
}

post_key* parse(parse_option popt, const char* html, void* ptype, char* separator, size_t* len) {
    return (*FUNCTION_REPOSITORY_PARSE[popt])(html, ptype, separator, len);
}

void get_value(const char* html, const char* start_criteria, const char* separator, const char* end, char* buffer,
        size_t bufflen) {
    char* start = nullptr;
    if ((start = strstr((char*)html, start_criteria)) == nullptr) {
        printf("failed to find [%s] inside raw html; will exit\n", start_criteria);
        exit(EXIT_FAILURE);
    }

    start += strlen(start_criteria);

    int to_save = 0, isend = 0, counter = 0;
    while (!isend) {
        char c = 0;
        if ((c = *start++) == *separator) {
            if (!to_save) {
                to_save = 1;
                continue; // do not include separator in buffer
            } else {
                break; // this means i got the separator for the second time
            }
        }

        // there might be 2 types of end characters; if the end char is the same with
        // the starting one, then this situation is handled above with the break;
        // Otherwise, if the char is different, then this is the way to break the while
        if (c == *end) {
            break;
        }

        if (to_save) {
            if (counter < bufflen) {
                memcpy(&buffer[counter++], &c, 1);
            } else {
                printf("the value length is larger than the buffer size (%lu)\n", bufflen);
                exit(EXIT_FAILURE);
            }
        }

        // just for safety
        if (counter > bufflen) {
            printf("something went wrong: could not find the end; will exit\n");
            exit(EXIT_FAILURE);
        }
    }

    buffer[counter] = 0;
}

post_key* parse_next_href(const char* html, void* pnext, char* separator, size_t* len) {
    struct href_next* p = (struct href_next*)pnext;

    char* start = nullptr;
    if ((start = strstr((char*)html, href_start_marker)) == nullptr) {
        printf("next href marker not present in source code\n");
        return 0;
    }

    int to_save = 0, iseof = 0, idx = 0;
    while (!iseof) {
        char c;
        if ((c = *start++) == *separator) {
            if (!to_save) {
                to_save = 1;
                continue;
            } else {
                break;
            }
        }

        if (to_save) {
            if (idx < sizeof(p->id)) {
                // replace $ with _
                memcpy(&p->id[idx++], &c, 1);
            } else {
                printf("buffer is full\n");
                break;
            }
        }
    }

    to_save = idx = 0;
    while (true) {
        char c = 0;
        if ((c = *start++) == '{') {
            to_save = 1;
            continue;
        }

        if (c == '}') {
            break;
        }

        if (to_save) {
            if (!isdigit(c)) {
                printf("bad format %c!\n", c);
                return 0;
            }
            memcpy(&p->next_row[idx++], &c, 1);
        }
    }

    return convert(conversion_option::HREFNEXT_TO_POSTKEY, pnext, len);
}

post_key* convert(const conversion_option copt, void* src, size_t* len) {
    return (*FUNCTION_REPOSITORY_CONVERT[copt])(src, len);
}

/**
 * len will be filled with the number of items in post_key list.
 *  */
post_key* convert_hrefnext_to_postkey(const void* pnext, size_t* len) {

    struct href_next* p = (struct href_next*)pnext;

    post_key* pkey = (post_key*)calloc(2, sizeof(struct post_key));
    if (pkey) {
        memcpy(pkey[0].key, "__EVENTTARGET", 13);
        memcpy(pkey[0].value, p->id, strlen(p->id));

        memcpy(pkey[1].key, "__EVENTARGUMENT", 15);
        sprintf(pkey[1].value, p->next_row_fmt, p->next_row);

        *len = 2;
    } else {
        printf("failed to get new memory\n");
    }

    return pkey;
}
