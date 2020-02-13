/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

/* Local Headers */
#include "common/pubsub.h"
#include "common/udp_server.h"

/* C++ Headers */
#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>
#include <mutex>

/* C Headers */
#include <sys/select.h>
#include <unistd.h>
#include <arpa/inet.h>

/* Preprocessor */
#define MAXMSG 120

/* Struct Definitions */
struct msgqueue {
    char * type;
    char * originator;
    char * org;
    char * contents;
};

/* Global Vars */
// -- RPC
CLIENT *clnt;
// -- Synchronization
std::thread udpthread;
std::mutex locker;
struct msgqueue * msgs[10];
int has_subs = 0, has_msg = 0;
int finished = 0;
// -- Networking
int sock; 
struct sockaddr_in loc;

/* Forward Declarations */
void print_actions(struct talk * tk);
char * concat_msg(struct msgqueue * msg, size_t artlen, int flag);
int parse_response(struct msgqueue * msg, const char * response, size_t nlen, struct talk * tk, int flag);
void check_msgs();

/* Thread Function */
void gather_msgs(struct talk * tk) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in rem;

    if(bind(sock, (struct sockaddr*)&loc, sizeof(loc)) < 0)
	fprintf(stdout, "Bind error\n");

    socklen_t rlen = sizeof(rem);
    char buf[MAXMSG];

    while(!finished) {
        struct msgqueue * msg = (struct msgqueue *)malloc(sizeof(msgqueue));
        ssize_t nbytes = recvfrom(sock, buf, MAXMSG, 0, (struct sockaddr*)&rem, &rlen);
        buf[nbytes] = '\0';

        if(strcmp(buf, "leave") == 0) {
	    free(msg);
	    break;
        }

        parse_response(msg, buf, nbytes, tk, 1);

        locker.lock();
        if (has_msg == 10) {
	    locker.unlock();
	    check_msgs();
	    locker.lock();
        }

	msgs[has_msg] = msg;
        has_msg++;
        locker.unlock();

	memset(buf, 0, MAXMSG);
    }
} /* gather_msgs */

/* Handles Synchronization Between Printing */
void check_msgs() {
    int i = 0;
    locker.lock();
    if (has_msg > 0) {   
	fprintf(stdout, "#-------------- ARTICLES --------------#\n");
        while(i < has_msg){
	    fprintf(stdout, "Type: ");
	    if (msgs[i]->type == NULL) {
		fprintf(stdout, "(not specified)\n");
	    } else {
		fprintf(stdout, "%s\n", msgs[i]->type);
		free(msgs[i]->type);
	    }
	    
	    fprintf(stdout, "Originator: ");
	    if (msgs[i]->originator == NULL) {
		fprintf(stdout, "(not specified)\n");
            } else {
		fprintf(stdout, "%s\n", msgs[i]->originator);
                free(msgs[i]->originator);
	    }

	    fprintf(stdout, "Organization: ");
	    if (msgs[i]->org == NULL) {
		fprintf(stdout, "(not specified)\n");
	    } else {
		fprintf(stdout, "%s\n", msgs[i]->org);
                free(msgs[i]->org);
	    }

	    fprintf(stdout, "--> %s\n", msgs[i]->contents);
            fprintf(stdout, "#---------------------------------------#\n");
            free(msgs[i]->contents);
	    free(msgs[i]);
            i++;
	} 
    } else {
	fprintf(stdout, "\n[%lu] No new messages received\n\n", (unsigned long)time(NULL));
    }
    has_msg = 0;
    locker.unlock();
} /* check_msgs */

/* Turns char * into struct msgqueue */
int parse_response(struct msgqueue * msg, const char * response, size_t nlen,  struct talk * tk, int flag) {
    msg->type = NULL, msg->originator = NULL, msg->org = NULL, msg->contents = NULL;
    unsigned int i = 0, cln_cnt = 0, last_cln = 0;
    
    while(i < nlen){
        if (response[i] == ';') {

            if (cln_cnt == 0 && i > 0){
                msg->type = (char *)malloc(i);
                strncpy(msg->type, response, i);
                msg->type[i] = '\0';
            } else if (cln_cnt == 1 && (i - last_cln) > 1) {
                msg->originator = (char *)malloc(i - (last_cln + 1));
                strncpy(msg->originator, response + (last_cln + 1), i - (last_cln + 1));
                msg->originator[i - (last_cln + 1)] = '\0';
            } else if (cln_cnt == 2 && (i - last_cln) > 1) {
                msg->org = (char *)malloc(i - (last_cln + 1));
                strncpy(msg->org, response + (last_cln + 1), i - (last_cln + 1));
                msg->org[i - (last_cln + 1)] = '\0';
            }

            last_cln = i;
            cln_cnt++;
        }
        
        if ( cln_cnt == 3) { 
            break;
        } 

        i++;
    }

    if(flag) {
        msg->contents = (char *)malloc(nlen - (last_cln + 1));
        strncpy(msg->contents, response + (last_cln + 1), nlen - (last_cln + 1));
        msg->contents[nlen - (last_cln + 1)] = '\0';
    }   
    
    if (!msg->type && !msg->originator && !msg->org) {
        fprintf(stdout, "-- Unable to process publish: Invalid metadata --\n");
        print_actions(tk);
    }
    
    return 0;
} /* parse_response */

/* Common Menu for Type */
char * get_type(struct talk * tk) {
    char * type;
    fprintf(stdout, "# Enter Type from the following list: \n");
    fprintf(stdout, "# 1. Sports 2. Lifestyle 3. Entertainment\n");
    fprintf(stdout, "#  4. Business 5. Technology 6. Science \n");
    fprintf(stdout, "#    7. Politics 8. Health 9. None \n");
    fprintf(stdout, "#          10. Return to menu\n");
    fprintf(stdout, "-- Selection: ");

    std::string sel; 
    std::getline(std::cin, sel);
    int num = atoi(sel.c_str());
    
    switch (num) {
        case 1:
            type = (char *)"Sports";
            break;
        case 2:
            type = (char *)"Lifestyle";
            break;
        case 3:
            type = (char *)"Entertainment";
            break;
        case 4:
            type = (char *)"Business";
            break;
        case 5:
            type = (char *)"Technology";
            break;
        case 6:
            type = (char *)"Science";
            break;
        case 7:
            type = (char *)"Politics";
            break;
        case 8:
            type = (char *)"Health";
            break;
        case 9:
            type = NULL;
            break;
        case 10:
            print_actions(tk);
            break;
        default:
            fprintf(stdout, "Invalid selection %d, select again", num);
            return get_type(tk);
            break;
    }
    return type;
} /* get_type */

/* Common Function for struct msgqueue --> char * */
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
    return arti;
} /* concat_msg */

/* publish_1 handler */
void handle_pub(struct talk *tk) {
    int * result;
    size_t artlen = 0;
    std::string sel;
    struct msgqueue msg;

    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "#            RPC PUBLISH MENU            #\n");
    fprintf(stdout, "#---------------------------------------\n");
    char * type = get_type(tk); 

    if ( type != NULL ) {
        msg.type = type;
	artlen += strlen(msg.type);
    }

    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "#            RPC PUBLISH MENU            #\n");
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "# Enter Originator Below or Press Return\n");
    fprintf(stdout, "#      in the Case there is not one\n");
    fprintf(stdout, "-- Originator: ");
    
    std::string orig;
    std::getline(std::cin, orig);

    if (orig.length() > 0) {
        msg.originator = (char *)malloc(orig.length() + 1);
        strncpy(msg.originator,orig.c_str(), orig.length());
        msg.originator[orig.length()] = '\0';
	artlen += strlen(msg.originator);
    } else {
        msg.originator = NULL;
    }
    
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "#            RPC PUBLISH MENU            #\n");
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "# Enter Organization Below or Press Return\n");
    fprintf(stdout, "#      in the Case there is not one\n");
    fprintf(stdout, "-- Organization: ");
    
    std::string org;
    std::getline(std::cin, org);

    if (org.length() > 0) {
        msg.org = (char *)malloc(org.length());
        strncpy(msg.org, org.c_str(), org.length());
        msg.org[org.length()] = '\0';
	artlen += strlen(msg.org);
    } else {
        msg.org = NULL;
    }
    
    if ( msg.type == NULL && msg.originator == NULL && msg.org == NULL) {
        fprintf(stdout, "Format Error: Publish must have at least\n");
        fprintf(stdout, "        one of the following \n");
        fprintf(stdout, "   [ type | originator | organization ]\n");
        handle_pub(tk);
    }
    
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "#            RPC PUBLISH MENU            #\n");
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "# Enter Contents of article: \n");
    fprintf(stdout, "-- Contents: ");
    
    std::string cntn;
    std::getline(std::cin, cntn);

    if (cntn.length() > 0){
        msg.contents = (char *)malloc(cntn.length());
        strncpy(msg.contents, cntn.c_str(), cntn.length());
        msg.contents[cntn.length()] = '\0';
    } else {
        fprintf(stdout, "Format Error: Article must have content in order to publish");
        handle_pub(tk);
    }
    
    artlen += strlen(msg.contents);

    char * arti = concat_msg(&msg, artlen, 1);

    fprintf(stdout, "\n#--- Sending article: %s ---#\n", arti);

    struct pub pubfil;
    pubfil.art = arti;
    pubfil.addr = tk->addr;
    pubfil.port = tk->port;
    
    if ((result = publish_1(&pubfil, clnt)) == NULL ){
        clnt_perror(clnt, "Publishing failed: ");
    }

    fprintf(stdout, "\n#---------- Publish completed ---------#\n");

    if(msg.originator != NULL)
        free(msg.originator);
    if(msg.org != NULL)
        free(msg.org);
    free(msg.contents);
    free(arti);

    check_msgs();
    print_actions(tk);
}

/* subscribe_1 handler */        
void handle_sub(struct talk * tk) {
    int * result;
    size_t artlen = 0;
    struct msgqueue msg;
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "#           RPC SUBSCRIBE MENU           #\n");
    fprintf(stdout, "#---------------------------------------\n");
    char * type = get_type(tk);

    if (type != NULL) {
	msg.type = type;
        artlen += strlen(type);
    }

    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "#           RPC SUBSCRIBE MENU           #\n");
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "# Enter Originator Below or Press Return\n");
    fprintf(stdout, "#      in the Case there is not one\n");
    fprintf(stdout, "-- Originator: ");

    std::string orig;
    std::getline(std::cin, orig);

    if (orig.length() > 0) {
        msg.originator = (char *)malloc(orig.length());
        strncpy(msg.originator,orig.c_str(),orig.length());
        msg.originator[orig.length()] = '\0';
        artlen += strlen(msg.originator);
    } else {
        msg.originator = NULL;
    }
    
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "#           RPC SUBSCRIBE MENU           #\n");
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "# Enter Organization Below or Press Return\n");
    fprintf(stdout, "#      in the Case there is not one\n");
    fprintf(stdout, "-- Organization: ");

    std::string org;
    std::getline(std::cin, org);

    if (org.length() > 0) {
        msg.org = (char *)malloc(org.length());
        strncpy(msg.org, org.c_str(), org.length());
        msg.org[org.length()] = '\0';
        artlen += strlen(msg.org);
    } else {
        msg.org = NULL;
    }
    
    if ( msg.type == NULL && msg.originator == NULL && msg.org == NULL) {
        fprintf(stdout, "Format Error: Publish must have at least\n");
        fprintf(stdout, "        one of the following \n");
        fprintf(stdout, "   [ type | originator | organization ]\n");
        handle_pub(tk);
    }

    char * arti = concat_msg(&msg, artlen, 0);
    struct sub subcr;
    subcr.art = arti;
    subcr.addr = tk->addr;
    subcr.port = tk->port;

    if ((result = subscribe_1(&subcr, clnt)) == NULL) {
	fprintf(stdout, "Subscription failed: %s\n", arti);
    }

    fprintf(stdout, "#------- Subscription Complete --------#\n");

    if(msg.originator != NULL)
	free(msg.originator);
    if(msg.org != NULL)
	free(msg.org);

    has_subs++;
    check_msgs();
    print_actions(tk);
}

/* unsubscribe_1 handler */
void handle_unsub(struct talk * tk) {
    int * result;
    size_t artlen = 0;
    struct msgqueue msg;
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "#          RPC UNSUBSCRIBE MENU          #\n");
    fprintf(stdout, "#---------------------------------------\n");
    char * type = get_type(tk);

    if (type != NULL) {
        msg.type = type;
        artlen += strlen(type);
    }

    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "#          RPC UNSUBSCRIBE MENU          #\n");
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "# Enter Originator Below or Press Return\n");
    fprintf(stdout, "#      in the Case there is not one\n");
    fprintf(stdout, "-- Originator: ");

    std::string orig;
    std::getline(std::cin, orig);

    if (orig.length() > 0) {
        msg.originator = (char *)malloc(orig.length());
        strncpy(msg.originator, orig.c_str(), orig.length());
        msg.originator[orig.length()] = '\0';
        artlen += strlen(msg.originator);
    } else {
        msg.originator = NULL;
    }

    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "#          RPC UNSUBSCRIBE MENU          #\n");
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "# Enter Organization Below or Press Return\n");
    fprintf(stdout, "#      in the Case there is not one\n");
    fprintf(stdout, "-- Organization: ");

    std::string org;
    std::getline(std::cin, org);

    if (org.length() > 0) {
        msg.org = (char *)malloc(org.length()); 
        strncpy(msg.org, org.c_str(), org.length());
        msg.org[org.length()] = '\0';
        artlen += strlen(msg.org);
    } else {
        msg.org = NULL;
    }

    if ( msg.type == NULL && msg.originator == NULL && msg.org == NULL) {
        fprintf(stdout, "Format Error: Publish must have at least\n");
        fprintf(stdout, "        one of the following \n");
        fprintf(stdout, "   [ type | originator | organization ]\n");
        handle_pub(tk);
    }

    char * arti = concat_msg(&msg, artlen, 0);

    struct sub subcr;
    subcr.art = arti;
    subcr.addr = tk->addr;
    subcr.port = tk->port;

    if ((result = unsubscribe_1(&subcr, clnt)) == NULL) {
	fprintf(stdout, "Unsubscribe failed: %s\n", arti);
    }

    fprintf(stdout, "#------ Unsubscription Complete -------#\n");

    if(msg.originator)
        free(msg.originator);
    if(msg.org)
        free(msg.org);
  
    has_subs--;
    check_msgs();
    print_actions(tk);
}

/* leave_1 handler */        
void handle_leave(struct talk * tk) {
    int * result;
    fprintf(stdout, "Leaving...\n");

    if ((result = leave_1(tk, clnt)) == NULL) {
	fprintf(stdout, "Failed Leave\n");
    }

    free(tk->addr);
    finished = 1;
    udpthread.join();

    exit(0);
}

/* Inital Menu */
void print_welcome() {
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "#           RPC PUBSUB SYSTEM            #\n");
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "# Choose from the following options:\n");
    fprintf(stdout, "#   1. Join\n");
    fprintf(stdout, "#   2. Ping\n");
    fprintf(stdout, "#   3. Exit\n");
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "Selection: ");
}

/* Main Menu for Pub\Sub Actions */
void print_actions(struct talk * tk) {
    int sel, opts = 2;
    std::string tmp;

    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "#            RPC ACTIONS MENU            #\n");
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "# Choose from the following options:\n");
    fprintf(stdout, "#   1. Subscribe\n");
    if (has_subs > 0) {
        fprintf(stdout, "#   %d. Unsubscribe\n",opts++);
    }
    fprintf(stdout, "#   %d. Publish\n",opts++);
    fprintf(stdout, "#   %d. Check Messages\n",opts++);
    fprintf(stdout, "#   %d. Leave\n",opts++);
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "Selection: ");
   
    std::getline(std::cin, tmp);
    sel = atoi(tmp.c_str());
  
    switch (sel) {
        case 1:
            handle_sub(tk);
            break;
        case 2:
            if (has_subs) {
                handle_unsub(tk);
            } else {
                handle_pub(tk);
            }
	    break;
        case 3:
            if (has_subs) {
                handle_pub(tk);
            } else {
                check_msgs();
            }
	    break;
        case 4:
            if (has_subs){
                check_msgs();
            } else {
                handle_leave(tk);
            }
	    break;
	case 5:
	    if (has_subs) {
		handle_leave(tk);
	    } else {
                fprintf(stdout, "-------Invalid selection! Select again.-------\n");
                print_actions(tk);
	    }	
        default:
           fprintf(stdout, "-------Invalid selection! Select again.-------\n");
           print_actions(tk);
           break;
    }
    print_actions(tk);
}

/* Join RPC Server & start UDP thread */
void joining(char * host) {
    int * result;
    struct talk tk;
    std::string addr, pt;

    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "#              JOIN SERVER               #\n");
    fprintf(stdout, "#---------------------------------------\n");
    fprintf(stdout, "#--     IP & Port Required to Join     --#\n");
    fprintf(stdout, "IP Address: ");
    
    std::getline(std::cin, addr);

    fprintf(stdout, "Port: ");    
    std::getline(std::cin, pt);
    size_t ptsz = pt.length();
    tk.port = stoi(pt, &ptsz);

    tk.addr = (char *)malloc(addr.length());
    strncpy(tk.addr, addr.c_str(), addr.length());
    tk.addr[addr.length()] = '\0';
    
    loc.sin_family = AF_INET;
    loc.sin_port = htons(tk.port);
    loc.sin_addr.s_addr = inet_addr(tk.addr);

    if ((result = join_1(&tk, clnt)) == NULL){
        clnt_perror(clnt, "Join failed: ");
    }
    
    udpthread = std::thread(gather_msgs, &tk);
    print_actions(&tk);
}

int main(int argc, const char * argv[]) {
    char * host;
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " [hostname]" << std::endl;
        host = strdup("localhost");
    } else {
        host = (char *)malloc(strlen(argv[1]));
        strncpy(host, argv[1], strlen(argv[1]));
    }

    clnt = clnt_create(host, PUBSUBPROG, SUBSUBVERS, "udp");
    if (clnt == NULL) {
        clnt_pcreateerror(host);
        exit(1);
    }

    while(1) {
        int sel;
        int * result = NULL;
        std::string tmpsel;
        print_welcome();
        std::getline(std::cin, tmpsel);

        if(!isdigit(tmpsel[0])){
	    fprintf(stdout, "#------------ Invalid Input -------------#\n");
	    continue;
        }

        size_t tmpsz = tmpsel.length();
        sel = stoi(tmpsel, &tmpsz);

        switch (sel) {
            case 1:
                joining(host);
                break;
	    case 2:
		if ((result = ping_1((void*)result, clnt)) == NULL){
		    clnt_perror(clnt, "Unable to ping host");
		} else {
		    fprintf(stdout, "#--- Host %s is up ---#\n", host);
		}
                break;		
            case 3:
		fprintf(stdout, "Leaving...\n");
		exit(0);                
            default:
                break;
        }
    }
    
    return 0;
}