#ifndef _COMMON_H_
#define _COMMON_H_

#include <stddef.h>

struct msgqueue{
    char * type;
    char * originator;
    char * org;
    char * contents;
};

int parse_response(struct msgqueue * msg, char * response, int flags);

char * copyString(char * str);

void parse_and_print_list(const char *list);

#endif /* _COMMON_H_ */
