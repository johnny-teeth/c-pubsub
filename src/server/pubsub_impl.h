#pragma once

struct subscription {
  char *addr;
  int port;
  char *type;
  char *originator;
  char *org;
  char *contents;
};


int * join_1_svc(talk *, struct svc_req *);
int * leave_1_svc(talk *, struct svc_req *);
int * subscribe_1_svc(sub *, struct svc_req *);
int * unsubscribe_1_svc(sub *, struct svc_req *);
int * publish_1_svc(pub *, struct svc_req *);
int * ping_1_svc(void *, struct svc_req *);

