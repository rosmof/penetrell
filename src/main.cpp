/************************************************

 *  Created on: Aug 24, 2018
 *      Author: AlexandruG

 ************************************************/

#include "request.h"
#include <curl/curl.h>
#include <stdlib.h>

#include "persistence.h"
#include "arguments.h"

/***********************************************************************************
 ** The purpose of this is to collect the tables from portal.just.ro that otherwise
 ** are available only by submitting POST requests for each 30 result set;
 **
 ** The main URL is hard coded inside the source code but indexes may be passed to
 ** GET different pages. The subsequent POST are done automatically if there are more
 ** rows.
 **
 ** To run the program one must use 2 parameters:
 ** 	a) -s , the index to start penetrell from
 ** 	b) -n , the n number to increment -s inside a for
 **
 ** Data is saved in /database/penetrel.db
 **
 ************************************************************************************/
int main(int argc, char* argv[]) {

    inargs* ar = handle_input(argc, argv);
    if (!ar) {
        exit(EXIT_FAILURE);
    }

    if (!initialize_database()) {
        printf("failed to initiate database context\n");
        exit(EXIT_FAILURE);
    }

    CURL* curl;
    curl = curl_easy_init();
    int rc = 0, start_index = ar->start;

    char address[128];

    for (int i = 0; i < ar->n; i++) {
        bzero(address, 128);
        sprintf(address, address_fmt, start_index + i, start_index + i);

        int next = 1;

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
            continue;
        }

        // get the table rows from the returned page
        table_row_list trlist;
        trlist.n = 0;
        if (get_table_data(&trlist, page.rsp_page) == 0) {
            printf("could not get details for %s; will skip this one\n", address);
            continue;
        }
        next += trlist.n;

        // here the data must be saved to db
        save_tablerows(&trlist, start_index + i, true);

        // get the post_keys from the returned page
        post_key_list pklist;
        pklist.items = 0;
        bool has_more_pages = get_form_fields(&pklist, page.rsp_page);

        // dig for subsequent pages
        if (has_more_pages) {
            const char* html = nullptr;

            do {
                html = do_post_request(curl, address, &pklist, next);
                if (html) {
                    table_row_list list;
                    if (get_table_data(&list, html) == 0) {
                        printf("Failed to get table data from POST response\n");
                        exit(EXIT_FAILURE);
                    }

                    // for some other time:
                    // if there is an error while saving the data  should
                    // retry for some time and the, if still not ok, break
                    save_tablerows(&list, start_index + i, false);
                    next += list.n;

                    free_table_data(&list);
                } else {
                    printf("There is no POST reply from server\n");
                    exit(EXIT_FAILURE);
                }
            } while (has_next(html));
        }

        printf("total elements for %s = %d\n", address, next);

        free_pklist(&pklist);     // cleanup post_key list from GET
        free_table_data(&trlist); // cleanup table_row list from GET
        free(page.rsp_page);      // cleanup html  page from GET
    }

    curl_easy_cleanup(curl);
    free(ar);

    return 0;
}
