/************************************************

 *  Created on: Sep 3, 2018
 *      Author: AlexandruG

 ************************************************/

#ifndef SRC_ARGUMENTS_H_
#define SRC_ARGUMENTS_H_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct _input_arguments {
    int start;
    int n;

} inargs;

void print_error() {
    printf("error: invalid input parameters\n");
    printf("usage: ./penetrell -s [index] -n [for], meaning:\n");
    printf("   -- s [index], the index should be taken from portal.just.ro "
           "(http://portal.just.ro/2/SitePages/acasa_default.aspx?id_inst=2)\n");
    printf("   -- n [for] , the number of increments to get from start\n");
    printf("try again!");
}

inargs* handle_input(int argc, char* argv[]) {
    int opt = 0, sval = 0, nval = 0;
    while ((opt = getopt(argc, argv, "s:n:")) != -1) {
        switch (opt) {
            case 's':
                sval = atoi(optarg);
                break;
            case 'n':
                nval = atoi(optarg);
                break;
            default:
                print_error();
                return nullptr;
        }
    }

    if (sval == 0 || nval == 0) {
	printf("inide err\n");
        print_error();
        return nullptr;
    }

    inargs* ina = (inargs*)calloc(1, sizeof(inargs));
    ina->start = sval;
    ina->n = nval;

    return ina;
}

#endif
