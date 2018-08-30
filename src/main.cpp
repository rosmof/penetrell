/************************************************

 *  Created on: Aug 24, 2018
 *      Author: AlexandruG

 ************************************************/

#include "request.h"
#include <curl/curl.h>
#include "form_data.h"

int main_about(int argc, char* argv[]) {

    CURL* curl;
    curl = curl_easy_init();
    int has_more_pages = 0, rc = 0, start_index = 290;

    char address[128];

    for (int i = 0; i < 10; i++) {
        bzero(address, 128);
        sprintf(address, address_fmt, start_index + i, start_index + i);

        response_page_details page;
        if ((rc = do_get_request(address, curl, &page)) == PENETREL_CURL_ERROR) {
            // there is an error from curl with the current page;
            // log, skip and move forward
            exit(EXIT_FAILURE);
        }

        if (page.rsp_status_hdr != PENETREL_HTTP_STATUS_OK) {
            // the server returned other status than 200
            // must be interpreted and take actions as necessary
        }

        post_key_list pklist;
        pklist.items = 0;

        printf("******* address = %s\n", address);

        if (!(has_more_pages = get_form_fields(&pklist, page.rsp_page))) {
            // there is only this page so no need to call next page POST
            printf(" -- there is only one page!");
        } else {
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
    }

    curl_easy_cleanup(curl);

    return 0;
}
