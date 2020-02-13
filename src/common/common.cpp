#ifndef __COMMON_COMMON_H__
#define __COMMON_COMMON_H__

#include "common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Convert char * to msgqueue */
int parse_response(struct msgqueue * msg, char * response, int flags) {
    msg->type = NULL, msg->originator = NULL, msg->org = NULL, msg->contents = NULL;
    unsigned int i = 0, cln_cnt = 0, last_cln = 0;
    size_t resp_len = strlen(response);

    while(i < resp_len){
        if (response[i] == ';') {

            if (cln_cnt == 0 && i > 0){
                msg->type = (char *) malloc(i+1);
                strncpy(msg->type, response, i);
		msg->type[i] = '\0';
                fprintf(stdout, "Type: %s ", msg->type);
            } else if (cln_cnt == 1 && (i - last_cln) > 1) {
                msg->originator = (char *)malloc(i - (last_cln + 1));
                strncpy(msg->originator, response + (last_cln + 1), i - (last_cln + 1));
		msg->originator[i - (last_cln + 1)] = '\0';
                fprintf(stdout, "Originator: %s ", msg->originator);
            } else if (cln_cnt == 2 && (i - last_cln) > 1) {
                msg->org = (char *) malloc(i - (last_cln + 1));
                strncpy(msg->org, response + (last_cln + 1), i - (last_cln + 1));
		msg->org[i - (last_cln + 1)] = '\0';
                fprintf(stdout, "Organization: %s ", msg->org);
            }

            last_cln = i;
            cln_cnt++;
        }

        if ( cln_cnt == 3 ) {
	    break;
        }

        i++;
    }

    if(flags == 1) {
        msg->contents = (char *)malloc(resp_len - (last_cln + 1));
        strncpy(msg->contents, response + (last_cln + 1), resp_len - (last_cln + 1));
	msg->contents[resp_len - (last_cln + 1)] = '\0';
        fprintf(stdout, "Contents: %s\n", msg->contents);
    } 

    return 0;
}

char * copyString(char * str) {
	unsigned int len = strlen(str);
	char * ret = (char *)malloc(len+1);

	memcpy(ret, str, len);

	ret[len] = '\0';
	return ret;
}

void parse_and_print_list(const char *list) {
  char *ip = NULL;
  char *progID = NULL;
  char *version = NULL;

  int i = 0, set = 0, start_char = 0;
  int done = 0;
  while(!done) {
    set = 0;
    while(set < 3) {
      if(list[start_char + i] == ';') {
	if(set == 0) {
	  ip = (char *)malloc(i+1);
	  strncpy(ip, list + start_char, i);
	  ip[i] = '\0';
	} else if(set == 1) {
	  progID = (char *)malloc(i+1);
	  strncpy(progID, list + start_char, i);
	  progID[i] = '\0';
	} else {
	  version = (char *)malloc(i+1);
	  strncpy(version, list + start_char, i);
	  version[i] = '\0';
	}
	++set;
	start_char += i + 1;
	i = 0;
      } else if(list[start_char + i] == '\0') {
	version = (char *)malloc(i);
	strncpy(version, list + start_char, i);
	++set;
	done = 1;
      }
      ++i;
    }
    fprintf(stdout, "IP: %s, ProgramID: %s, Version: %s\n", ip, progID, version);
    free(ip);
    free(progID);
    free(version);
  }
}

#endif /* __COMMON_COMMON_H__ */
