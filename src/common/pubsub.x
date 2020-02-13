const MAXSTRING = 120;
const MAXADDR = 15;
typedef string ip<MAXADDR>;
typedef string article<MAXSTRING>;

struct talk {
    int port;
    ip addr;
};

struct sub {
    int port;
    ip addr;
    article art;
};

struct pub {
    article art;
    ip addr;
    int port;
};

program PUBSUBPROG {
    version SUBSUBVERS {
        int JOIN(talk) = 1;
        int LEAVE(talk) = 2;
        int SUBSCRIBE(sub) = 3;
        int UNSUBSCRIBE(sub) = 4;
        int PUBLISH(pub) = 5;
        int PING() = 6;
    } = 1;
} = 0x20000001;
