/************************************************

 *  Created on: Aug 24, 2018
 *      Author: AlexandruG

 ************************************************/

#include "request.h"
#include <curl/curl.h>
#include "form_data.h"
#include "persistence.h"

void print_table_row(table_row_list* list) {
    for (int i = 0; i < list->n; i++) {
        table_row* row = list->prow[i];
        printf("judet=%32s, localitate=%64s, cod=%8s, institutie=%s\n", row->judet, row->strada, row->cod,
                row->institutie);
    }
}

void print_pklist(post_key_list* list) {
    for (int i = 0; i < list->items; i++) {
        struct post_key* pk = list->keys[i];
        printf(" -- key=%s value=%s\n", pk->key, pk->value);
    }
}

int main(int argc, char* argv[]) {

    if (initialize_database()) {
        printf("adad");
    }

    exit(0);

    CURL* curl;
    curl = curl_easy_init();
    int rc = 0, start_index = 290;

    char address[128];

    for (int i = 0; i < 1; i++) {
        bzero(address, 128);
        sprintf(address, address_fmt, start_index + i, start_index + i);

        // get the GET page response
        response_page_details page;
        if ((rc = do_get_request(address, curl, &page)) == PENETREL_CURL_ERROR) {
            // there is an error from curl with the current page;
            // log, skip and move forward
            exit(EXIT_FAILURE);
        }

        if (page.rsp_status_hdr != PENETREL_HTTP_STATUS_OK) {
            // the server returned other status than 200
            // must be interpreted and take actions as necessary
            printf("Header from GET is not OK for %s [%d]\n", address, page.rsp_status_hdr);
            exit(EXIT_FAILURE);
        }

        // get the table rows from the returned page
        table_row_list trlist;
        trlist.n = 0;
        if (get_table_data(&trlist, page.rsp_page) == 0) {
            printf("could not get details for %s; will skip this one\n", address);
            continue;
        }

        // here the data must be saved to db
        print_table_row(&trlist);
        free_table_data(&trlist);

        // get the post_keys from the returned page
        post_key_list pklist;
        pklist.items = 0;
        bool has_more_pages = get_form_fields(&pklist, page.rsp_page);

        if (has_more_pages) {
            const char* html = nullptr;
            int next = 31; //

            // get all the post pages
            do {
                html = do_post_request(curl, address, &pklist, next);
                if (html) {
                    table_row_list list;
                    if (get_table_data(&list, html) == 0) {
                        printf("Failed to get table data from POST response\n");
                        exit(EXIT_FAILURE);
                    }
                    print_table_row(&list);
                    next += list.n;
                    free_table_data(&list);
                } else {
                    printf("There is no POST reply from server\n");
                    exit(EXIT_FAILURE);
                }
            } while (has_next(html));
        }

        // cleanup
        free_pklist(&pklist);
        free(page.rsp_page);
    }

    curl_easy_cleanup(curl);

    return 0;
}
