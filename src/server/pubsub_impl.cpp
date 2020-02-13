#include <common/pubsub.h>
#include <server/pubsub_impl.h>
#include <common/common.h>
#include <common/udp_server.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <server/common.h>
#include <arpa/inet.h>
#include <thread>

using namespace std;

map< string, vector<subscription *> > sub_list;
int num_clients = 0;

/* Convert msgqueue to char * */ 
char * concat_msg(struct msgqueue * msg, size_t artlen, int flag) { 
    char * arti = (char *)malloc(artlen + 3); 
    if ( msg->type != NULL ){ 
        sprintf(arti, "%s;", msg->type); 
        if ( msg->originator != NULL ) { 
            strcat(arti, msg->originator); 
        } 
        strcat(arti, ";"); 
        if ( msg->org != NULL ) { 
            strcat(arti, msg->org); 
        } 
        strcat(arti, ";"); 
 
    } else if ( msg->originator != NULL ) { 
        sprintf(arti,";%s;", msg->originator); 
        if ( msg->org != NULL ) { 
            strcat(arti, msg->org); 
        } 
        strcat(arti, ";"); 
 
    } else { 
        sprintf(arti, ";;%s;", msg->org); 
    } 
 
    if (flag) { 
        strcat(arti, msg->contents); 
    } 
    cout << "Constructed article string: ";
    cout << arti << endl;

    return arti; 
}

/* Register client by its port and ip. Return 0 if port/ip already exists */
int *join_1_svc(talk *t, struct svc_req *) {

    static int result;
    stringstream ss;
    ss << t->port;
    string key = ss.str() + t->addr;

    // Add pairing if it doesn't already exist
    if(sub_list.count(key) > 0) {
	result = 0;
    } else {
        cout << "Adding: " << t->addr << "  " << t->port << endl;
        sub_list.insert(pair<string, vector<subscription *> >(key, vector<subscription *>()));
	num_clients++;
	result = 1;
    }
    return &result;
}

/* Remove the port/ip pairing from the list */
int *leave_1_svc(talk *t, struct svc_req *) {
    static int result = 1;
    stringstream ss;
    ss << t->port;
    string key = ss.str() + t->addr;

    // If pairing exists, remove it
    if(sub_list.count(key) <= 0) {
	result = 0;
    }else {
	cout << "Removing: " << t->addr << "  " << ss.str() << endl;
	sub_list.erase(key);
	num_clients--;
	result = 1;
	globalUdpServer->send(t->addr, atoi(ss.str().c_str()), "leave", strlen("leave")); 
    }

    return &result;
}

/* Add a subscription for the client/ip */
/* Returns 0 if port/ip pairing doesn't exist */
int *subscribe_1_svc(sub *s, struct svc_req *) {
    static int result; 
    struct msgqueue * msg = new struct msgqueue();
    struct subscription *subs = new struct subscription();
    
    cout << "Received Subscription: " << s->art << endl;

    stringstream ss;
    ss << s->port;
    string key = ss.str() + s->addr;
    parse_response(msg, s->art, 0);

    subs->addr = copyString(s->addr);
    subs->port = s->port;

    if(msg->type != NULL){
        fprintf(stdout, "type->%s ",msg->type);
        subs->type = msg->type;
    } else {
        fprintf(stdout, "type->(null) ");
	subs->type = NULL;
    }
    if(msg->org != NULL) {
        fprintf(stdout, "org->%s ",msg->org);
        subs->org = msg->org;
    } else {
        fprintf(stdout, "org->(null) ");
	subs->org = NULL;
    }
    if(subs->originator != NULL) {
	fprintf(stdout, "orig->%s ",msg->originator);
        subs->originator = msg->originator;
    } else {
        fprintf(stdout, "orig->(null) ");
	subs->originator = NULL;
    }
    cout << endl;

    // If pairing exists, add subscription
    if(sub_list.count(key) <= 0){ 
	result = 0;
    } else {
	sub_list[key].push_back(subs);
	result = 1;
    }

    return &result;
}

/* Returns 0 when port/ip pairing or subscription doesn't exist. Deletes
   multiple subscriptsion if they are the same. All other errors can be handled
   by the client */
int *unsubscribe_1_svc(sub *s, struct svc_req *) {
    static int result;
    struct msgqueue msg;

    stringstream ss;
    ss << s->port;
    string key = ss.str() + s->addr;
    parse_response(&msg, s->art, 0);

    // Check if ip/port pairing exists
    if(sub_list.count(key) <= 0) { 
	result = 0;
    } else {
	// Search for subscription
	vector<subscription *> connection = sub_list[key];
	for(unsigned int it = 0; it < connection.size(); it++) {
	    size_t len = 0;
            struct msgqueue msgsub;

            if(connection[it]->type != NULL) {
		len += strlen(connection[it]->type);
                msgsub.type = connection[it]->type;
            } else {
		msgsub.type = NULL;
	    }

	    if(connection[it]->originator != NULL){
		len += strlen(connection[it]->originator);
		msgsub.originator = connection[it]->originator;
	    } else {
		msgsub.originator = NULL;
	    }

	    if(connection[it]->org != NULL) {
		len += strlen(connection[it]->org);
		msgsub.org = connection[it]->org;
	    } else {
		msgsub.org = NULL;
	    }

            char * substr = concat_msg(&msgsub, len, 0);

            if(strcmp(substr, s->art) == 0) {
		cout << "Deleting subscription: " << substr << endl;
                connection.erase(connection.begin() + it);
                sub_list.erase(key);
		sub_list.insert(make_pair(key, connection));
		result = 1;
		break;
            }
	}
    }
    return &result;
}

/* Perform search on subset of clients from map */
void scan_client(char * art, vector<string> &clients, int start, int doesinc) {
    int finish;
    int success;
    struct msgqueue msg;
    unsigned int total = clients.size();
    int shared = total / 4;

    if (doesinc) {
        finish = start + 1;
    } else {
	if (start == 3) {
	    finish = total;
	} else {
	    finish = start + shared;
	}
    }

    parse_response(&msg, art, 1);

    for (int i = start; i < finish; i++) {
        vector<subscription *> subs = sub_list[clients[i]];
        for (unsigned int j = 0; j < subs.size(); j++) {
            char * type, * orig, * org;
            int findt = 0, findori = 0, findorg = 0;
            int matcht = 0, matchori = 0, matchorg = 0;

            if(subs[j]->type != NULL) {
                type = subs[j]->type;
		findt = 1;
            }
            if(subs[j]->originator != NULL) {
                orig = subs[j]->originator;
                findori = 1;
            }
            if(subs[j]->org != NULL) {
                org = subs[j]->org;
		findorg = 1;
            }

            if (findt && strcmp(type, "(null)") != 0) {
	        if( msg.type != NULL ) {
	    	    if ( strcmp(msg.type, type) == 0) {
                       matcht = 1;
	    	    } else {
	    	        continue;
	    	    }
	        } else {
	           continue;
                }
            } else {
	        matcht = 1;
            }

            if (findori && strcmp(orig, "(null)") != 0) {
	        if(msg.originator != NULL) {
	    	    if(strcmp(msg.originator, orig) == 0){
	    	        matchori = 1;
	    	    } else {
	    	        continue;
	    	    }
	        } else {
	    	    continue;
	        }
            } else {
	        matchori = 1;
            }

            if (findorg && strcmp(org, "(null)") != 0) {
	        if(msg.org != NULL) {
	    	    if(strcmp(msg.org, org) == 0) {
	    	        matchorg = 1;
	    	    } else {
	    	        continue;
                    }
	        } else {
	    	    continue;
	        }   
            } else {
	        matchorg = 1;
	    }
            if (matcht && matchori && matchorg) {
                success = globalUdpServer->send(subs[j]->addr, subs[j]->port, art, strlen(art));
                if (success) {
                    break;
                } else {
                    fprintf(stderr, "Error sending message: %s %s %d\n",art,subs[j]->addr,subs[j]->port);
                }
            }
        }
    }
}

/* Determines how many threads & how many clients per thread are search */
void search_clients(char * msg) {
    int thread_num;
    int ltefour = 0;
    map<string, vector<subscription *>>::iterator it;
    vector<string> clients;
    for(it = sub_list.begin(); it != sub_list.end(); ++it){
        clients.push_back(it->first);
    }

    int thread_cli = clients.size() / 4;
    if (thread_cli == 0) {
        thread_num = clients.size() % 4;
	ltefour = 1;
    } else {
	thread_num = 4;
	ltefour = 0;
    }
    

    cout << "Threads: " << thread_num << " Size: " << clients.size() << endl;
 
    thread pool[thread_num];
    for( int i = 0; i < thread_num; i++ ){
        cout << "Starting search thread " << i << endl;
	pool[i] = thread(scan_client, msg, ref(clients), i, ltefour);
    }

    for( int i = 0; i < thread_num; i++) {
	pool[i].join();
    }
}

/* Pass article string to threaded search handling */
int * publish_1_svc(pub *p, struct svc_req *) {
    static int result;
    cout << "Received Publishing Article: " << p->art << endl;
    search_clients(p->art);
    result = 1;
    return &result;
}

int * ping_1_svc(void *, struct svc_req *) {
     static int result = 1;
     cout << "Received ping" << endl;
     return &result;
}

