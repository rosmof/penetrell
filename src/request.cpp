/************************************************

 *  Created on: Aug 25, 2018
 *      Author: AlexandruG

 ************************************************/

#include "request.h"

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

size_t content_callback(void* block, size_t size, size_t nmemb, void* userp) {

    size_t rsz = size * nmemb;
    struct memory_block* pmb = (struct memory_block*)userp;

    /* increase the buffer */
    pmb->buffer = (char*)realloc(pmb->buffer, pmb->size + rsz + 1);
    if (pmb->buffer == nullptr) {
        printf("failed to get new memory; will close\n");
        return 0;
    }

    /* realloc keeps the original data, so only the new block must be copied */
    memcpy(&pmb->buffer[pmb->size], block, rsz);

    /* update memory_block size */
    pmb->size += rsz;

    /* null terminated string */
    pmb->buffer[pmb->size] = 0;

    return rsz;
}

size_t header_callback(char* header, size_t size, size_t nitems, void* userdata) {
    char* found = nullptr;

    if ((found = strstr(header, PENETREL_HEADER_STATUS)) != 0) { // search for status header
        sscanf(header, "%*s %d", &((header_block*)userdata)->http_status);

    } else if ((found = strstr(header, PENETREL_HEADER_LENGTH)) != 0) { // search for content length header
        sscanf(found, "%*s %lu", &((header_block*)userdata)->response_len);
    }

    return size * nitems;
}

/**
 * First GET request to a page.
 *
 * @return 0 if success or -1 if failed; it can fail only from CURL; other failures
 * will be interpreted based in the status from header.
 **/
int do_get_request(const char* address, CURL* c, response_page_details* page) {

    int rc = 0;

    struct memory_block mb;
    mb.buffer = (char*)malloc(1);
    mb.size = 0;

    struct header_block hb;
    hb.http_status = 0;
    hb.response_len = 0;

    curl_easy_reset(c);

    if (c) {
        CURLcode res;
        curl_easy_setopt(c, CURLOPT_URL, address);
        curl_easy_setopt(c, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(c, CURLOPT_USERAGENT,
                "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_3) AppleWebKit/537.36 (KHTML, like Gecko) "
                "Chrome/68.0.3440.106 Safari/537.36");
        curl_easy_setopt(c, CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, content_callback);
        curl_easy_setopt(c, CURLOPT_WRITEDATA, (void*)&mb);
        curl_easy_setopt(c, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(c, CURLOPT_HEADERDATA, &hb);

        printf("before perform\n");
        res = curl_easy_perform(c);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return -1;
        }

        page->rsp_page = mb.buffer;
        page->rsp_size_recv = mb.size;
        page->rsp_size_hdr = hb.response_len;
        page->rsp_status_hdr = hb.http_status;

    } else {
        printf("failed to get CURL initialized\n");
        rc = -1;
    }

    return rc;
};

const char* do_post_request(CURL* curl, const char* address, post_key_list* pklist, size_t gotn) {

    char err_buffer[CURL_ERROR_SIZE];

    struct memory_block response_page;
    response_page.buffer = (char*)malloc(1);
    response_page.size = 0;

    struct header_block hb;
    hb.http_status = 0;
    hb.response_len = 0;

    CURLcode code;

    curl_easy_reset(curl);

    curl_easy_setopt(curl, CURLOPT_URL, address);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err_buffer);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,
            "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_3) AppleWebKit/537.36 (KHTML, like Gecko) "
            "Chrome/68.0.3440.106 Safari/537.36");

    const char* post_data = set_post_fields(curl, pklist, gotn);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_REFERER, address);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, content_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_page);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &hb);

    // custom AJAX header
    struct curl_slist* list = nullptr;
    list = curl_slist_append(list, "X-MicrosoftAjax: Delta=true");
    list = curl_slist_append(list, "Origin: http://portal.just.ro");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    if ((code = curl_easy_perform(curl)) != CURLE_OK) {
        printf("failed to post the data -> %s , %s \n", curl_easy_strerror(code), err_buffer);
        return nullptr;
    }

    curl_slist_free_all(list);
    delete post_data;

    return response_page.buffer;
}

/**
 * Creates a string formed by key=value linked with the & char. Keys and Values are
 * converted to URL-Encoded with curl_easy_escape;
 *
 * @return allocated char with the content of the post data; the result must be freed
 * after being used;
 *  */
const char* set_post_fields(CURL* curl, struct post_key_list* pklist, size_t gotn) {

    std::string post_data;

    update_eventargument(pklist, gotn);

    for (int i = 0; i < pklist->items; i++) {
        struct post_key* pk = pklist->keys[i];
        post_data.append(curl_easy_escape(curl, pk->key, strlen(pk->key)))
                .append("=")
                .append(curl_easy_escape(curl, pk->value, strlen(pk->value)))
                .append("&");
    }

    char* result = (char*)calloc(post_data.size(), sizeof(unsigned char));
    strcpy(result, post_data.c_str());

    return result;
}
