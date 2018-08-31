/************************************************

 *  Created on: Aug 29, 2018
 *      Author: AlexandruG

 ************************************************/
#include <unistd.h>

#include <fstream>
#include <iostream>

#include <tidy.h>
#include <tidybuffio.h>
#include <tidyplatform.h>

#include <libxml/HTMLparser.h>

// this has to be recursive
void extractFromNode(TidyDoc tdoc, TidyNode tnode) {

    int index = 0;

    TidyNode cursor;
    for (cursor = tidyGetChild(tnode); cursor; cursor = tidyGetNext(cursor)) {

        TidyNode nd;

        // the values are usually the next node inside the td
        // but sometimes there is an a tag so we have to skip it
        // with the while below
        for (nd = tidyGetChild(cursor); nd; nd = tidyGetNext(nd)) {
            TidyNode valuenode = tidyGetChild(nd);
            ctmbstr value;

            // skip other tags
            while ((value = tidyNodeGetName(valuenode)) != nullptr) {
                valuenode = tidyGetChild(valuenode);
            }

            TidyBuffer buffer;
            tidyBufInit(&buffer);
            tidyNodeGetValue(tdoc, valuenode, &buffer);

            printf("...the value is: %s\n", buffer.bp);
        }
    }
}

void dumpNode(TidyDoc tdoc, TidyNode tnode, int indent) {
    TidyNode child;
    for (child = tidyGetChild(tnode); child; child = tidyGetNext(child)) {
        ctmbstr name = tidyNodeGetName(child);

        if (name) {
            if (strcmp(name, "input") == 0) {
                TidyAttr attr = tidyAttrFirst(child);
                if (strcmp(tidyAttrName(attr), "id") == 0) {
                    const char* aval = tidyAttrValue(attr);
                    if (strcmp(
                            aval,
                            "ctl00_PlaceHolderMain_g_17385422_131b_4c6c_89b4_"
                            "9d3c87bc221a_ctl01") == 0) {

                        // use get child - see the html structure
                        extractFromNode(tdoc, tidyGetNext(child));
                        printf("will exit\n");
                        break;
                    }
                }
            }
        }

        //        if (name) {
        //            TidyAttr attr;
        //            printf("%*.*s%s", indent, indent, "<", name);
        //
        //            for (attr = tidyAttrFirst(child); attr; attr =
        //            tidyAttrNext(attr)) {
        //                printf("%s", tidyAttrName(attr));
        //                tidyAttrValue(attr) ? printf("=\"%s\" ",
        //                tidyAttrValue(attr)) : printf(" ffffffffffff ");
        //            }
        //            printf(">\n");
        //        }
        //                } else {
        //            TidyBuffer tbuf;
        //            tidyBufInit(&tbuf);
        //            tidyNodeGetText(tdoc, child, &tbuf);
        //            printf("xxxxxxxxx%*.*s\n", indent, indent + 4, tbuf.bp ?
        //            (char*)tbuf.bp : ""); tidyBufFree(&tbuf);
        //        }

        dumpNode(tdoc, child, indent + 4);
    }
}

int main() {

    char root[256];
    bzero(root, 256);
    getcwd(root, 256);

    sprintf(root + strlen(root), "%s", "/html_documents/get_page_html.dat");

    std::ifstream src;
    src.open(root, std::ios_base::binary | std::ios_base::in);

    if (!src) {
        std::cout << "failed to open source file" << std::endl;
        exit(EXIT_FAILURE);
    }

    printf("opened file at: %s\n", root);

    src.seekg(0, std::ios_base::end);
    unsigned long len = src.tellg();
    src.seekg(0, std::ios_base::beg);

    std::cout << "message length = " << len << std::endl;

    TidyDoc tdoc;
    TidyBuffer docbuff = {0};
    TidyBuffer errbuff = {0};

    tdoc = tidyCreate();
    tidyOptSetBool(tdoc, TidyForceOutput, yes);
    tidyOptSetInt(tdoc, TidyWrapLen, 4096);
    tidyOptSetBool(tdoc, TidyMakeBare, yes);
    tidySetErrorBuffer(tdoc, &errbuff);
    tidyBufInit(&docbuff);

    std::string line;
    while (std::getline(src, line)) {
        tidyBufAppend(&docbuff, (void*)line.c_str(), line.size());
    }

    int err;
    err = tidyParseBuffer(tdoc, &docbuff);
    if (err >= 0) {
        err = tidyCleanAndRepair(tdoc);
        if (err >= 0) {
            err = tidyRunDiagnostics(tdoc);
            if (err >= 0) {
                dumpNode(tdoc, tidyGetRoot(tdoc), 0);
                fprintf(stderr, "%s\n", errbuff.bp);
            }
        }
    }

    // printf("%s\n", docbuff.bp);

    tidyBufFree(&docbuff);
    tidyBufFree(&errbuff);
    tidyRelease(tdoc);
}
