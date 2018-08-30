/************************************************

 *  Created on: Aug 25, 2018
 *      Author: AlexandruG

 ************************************************/
#include <iostream>
#include <curl/curl.h>
#include <curl/easy.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include "parse.h"
#include "form_data.h"

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
            free(pklist->keys);
            i++;
            continue;
        } else {
            free(pklist->keys[i]);
        }
        pklist->items--;
    }
}

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
