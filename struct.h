#define BUFFLEN 1560

struct User{
	char *id, subscris[25][50];
	int nr_subscris;
	char  sf_ind[25];
};

struct Data{
	int sf_ind;
	int subscr_type;
	char topic[BUFFLEN];	
};

struct SentPacket{
	int nr_port;
	char ip[50], str[BUFFLEN];
};

struct SfPacket{
	char topic[50];
	int nr_buff, sock;
	int sf_ind;
	struct SentPacket packet[10];
};

struct Database{
	char id[20]; 
	int nr;
	struct SfPacket packet[10];
};