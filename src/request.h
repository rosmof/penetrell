/************************************************

 *  Created on: Aug 25, 2018
 *      Author: AlexandruG

 ************************************************/

#ifndef SRC_REQUEST_H_
#define SRC_REQUEST_H_

#include "parse.h"
#include <curl/curl.h>
#include <curl/easy.h>

struct response_page_details {
    char* rsp_page;
    int rsp_status_hdr;
    size_t rsp_size_hdr;
    size_t rsp_size_recv;
};

/**
 * This function is set as callback function to CURL to be called when the response
 * from the server is available; multiple calls for one response may happen but that
 * is behind this procedure, inside CURL.
 *  */
size_t content_callback(void* block, size_t size, size_t nmemb, void* userp);

/**
 * This function is set as callback function to CURL to be called as soon as the
 * headers from the server are sent back to CURL; for each header, this function
 * is called;
 *  */
size_t header_callback(char* header, size_t size, size_t nitems, void* userdata);



int do_get_request(const char* address, CURL* curl, response_page_details* page);

/**
 * This function creates the post request in order to get another page from the server
 * */
const char* do_post_request(CURL* curl, const char* address, post_key_list* pklist, size_t gotn);

/**
 * This function sets all the post fields for a POST request. For EVENTARGUMENT field
 * it increments the next value depending on the gotn value, where gotn is the number
 * of items collected so far.
 *
 * @param pklist the list with pointers to the key-value mapping that is stored in
 * post_key structure;
 *
 * @return an URL-ENCODED null-terminated string that represents the post form value
 *
 *  */
const char* set_post_fields(CURL* curl, struct post_key_list* pklist, size_t gotn);

#endif
