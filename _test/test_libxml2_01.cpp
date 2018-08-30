/************************************************

 *  Created on: Aug 29, 2018
 *      Author: AlexandruG

 ************************************************/

#include <iostream>
#include <fstream>

#include <tidy.h>
#include <tidybuffio.h>
#include <tidyplatform.h>

#include <libxml/HTMLparser.h>

// this has to be recursive
void extractFromNode(TidyDoc tdoc, TidyNode tnode) {
    printf("called extract for node=%s!\n", tidyNodeGetName(tnode) ? tidyNodeGetName(tnode) : " [value] ");
    TidyNode child;
    for (child = tidyGetChild(tnode); child; child = tidyGetNext(child)) {
        ctmbstr name = tidyNodeGetName(child);
        extractFromNode(tdoc, child);
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
                    if (strcmp(aval, "ctl00_PlaceHolderMain_g_17385422_131b_4c6c_89b4_9d3c87bc221a_ctl01") == 0) {

                        // call here to extract the subnodes and the values
                        // that is the table
                        printf("EVIRKA!\n");
                        extractFromNode(tdoc, tidyGetParent(child));
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
        //            for (attr = tidyAttrFirst(child); attr; attr = tidyAttrNext(attr)) {
        //                printf("%s", tidyAttrName(attr));
        //                tidyAttrValue(attr) ? printf("=\"%s\" ", tidyAttrValue(attr)) : printf(" ffffffffffff ");
        //            }
        //            printf(">\n");
        //        }
        //                } else {
        //            TidyBuffer tbuf;
        //            tidyBufInit(&tbuf);
        //            tidyNodeGetText(tdoc, child, &tbuf);
        //            printf("xxxxxxxxx%*.*s\n", indent, indent + 4, tbuf.bp ? (char*)tbuf.bp : "");
        //            tidyBufFree(&tbuf);
        //        }

        dumpNode(tdoc, child, indent + 4);
    }
}

int main() {
    std::ifstream src;
    src.open("/Users/AlexandruG/eclipse-ws/penetrel/html_documents/get_page_html.dat",
            std::ios_base::binary | std::ios_base::in);

    if (!src) {
        std::cout << "failed to open source file" << std::endl;
        exit(EXIT_FAILURE);
    }

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
