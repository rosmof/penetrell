/************************************************

 *  Created on: Aug 24, 2018
 *      Author: AlexandruG

 ************************************************/

#include "request.h"
#include <curl/curl.h>
#include "form_data.h"

int main(int argc, char* argv[]) {

    CURL* curl;
    curl = curl_easy_init();
    int rc = 0, start_index = 290;

    char address[128];

    for (int i = 0; i < 10; i++) {
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
            printf("Header from GET is not OK\n");
            exit(EXIT_FAILURE);
        }

        // get the table rows from the returned page
        table_row_list trlist;
        trlist.n = 0;
        int rc = get_table_data(&trlist, page.rsp_page);

        // print the table list
        for (int i = 0; i < trlist.n; i++) {
            table_row* row = trlist.prow[i];
            printf("judet=%s, localitate=%s, cod=%s, institutie=%s\n", row->judet, row->strada, row->cod,
                    row->institutie);
        }


        // get the post_keys from the returned page
        post_key_list pklist;
        pklist.items = 0;
        bool has_more_pages = get_form_fields(&pklist, page.rsp_page);

        printf("******* address = %s\n", address);

        if (!(has_more_pages)) {
            // there is only this page so no need to call next page POST
            printf(" -- there is only one page!");
        } else {

            //            const char* html = do_post_request(curl, address, &pklist, 12);
            //            if (html) {
            //                table_row_list list;
            //                get_table_data(&list, html);
            //            }

            // there are multiple pages
            // so call POST for next page until you reach the end
            for (int i = 0; i < pklist.items; i++) {
                struct post_key* pk = pklist.keys[i];
                printf(" -- key=%s value=%s\n", pk->key, pk->value);
            }
        }
        printf("**************************************************************\n");
        printf("\n");

        free_pklist(&pklist);
        free(page.rsp_page);
    }

    curl_easy_cleanup(curl);

    return 0;
}
