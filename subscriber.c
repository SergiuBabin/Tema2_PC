#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "struct.h"

int readInstruction(int tcpsocket){
    char *str = calloc(BUFFLEN, sizeof(char));
    char *newstr = calloc(BUFFLEN, sizeof(char));
    struct Data sub;
    unsigned int subsSize = sizeof(struct Data);
    fgets(str, BUFFLEN, stdin);

    int len = strlen(str);
    strncpy(newstr, str, len - 1);
    if(strcmp(newstr,"exit") == 0)
        exit(0);

    char *command = strtok(str, " ");
    if(command == NULL){
        return -1;
    }

    if (strcmp(command,"subscribe") == 0){
        char *newsff = calloc(BUFFLEN, sizeof(char));
        if(!newsff)
            exit(0);
        char *topic = strtok(NULL, " ");
        if(!topic)
            return -1;
        strcpy(sub.topic, topic);
        char *sff = strtok(NULL, " ");
        if(!sff)
            return -1;
        sub.subscr_type = 0;
        len = strlen(sff);
        strncpy(newsff,sff, len - 1);
        if(strcmp(newsff,"1") != 0 && strcmp(newsff, "0") != 0)
            return -1;
        sub.sf_ind = newsff[0] - '0';
        char *ssub = (char *)&sub;
        if (send(tcpsocket, ssub, subsSize, 0) < 0){
            perror("eroare la send");
            exit(0);
        }

        unsigned int found = 0;
        if(recv(tcpsocket, (char*)&found, 4, 0) < 0){
            perror("eroare la recv");
            exit(0);
        }
        if(found){
            printf("Ati mai fost abonat la acest topic\n");
            return 0;
        }
        printf("Sunteti abonat cu succes la acest topic\n");
        return 0;
    }
    if(strcmp(command,"unsubscribe") == 0){
        char *topic = strtok(NULL, " ");
        if (!topic)
            return -1;
        len = strlen(topic); 
        strncpy(sub.topic, topic, len - 1);
        sub.subscr_type = 1;
        if(command != NULL){
            char *topic = strtok(NULL, " ");
            if (topic != NULL && strcmp(topic, "\n") != 0)
                return -1;
        }
        if (send(tcpsocket,(char *)&sub, subsSize, 0) < 0){
            perror("eroare la send");
            exit(0);
        }
        unsigned int found = 0;
        if(recv(tcpsocket, (char*)&found, 4, 0) < 0){
            perror("eroare la recv");
            exit(0);
        }
        if (found){
            printf("Nu mai sunteti abonat la acest topic\n");
            return 0;
        }
        printf("Nu ati fost abonat la acest topic\n");
        return 0;
    } 
    return -1;
}

void parseBuffer(int tcpsocket){
    struct SentPacket msg;
    char *topic = calloc(50, sizeof(char));
    int semn, res = 0, puterea, mod, i;
    char *smsg = (char*)&msg;
    unsigned int structsize = sizeof(struct SentPacket);
    int rs = recv(tcpsocket, smsg, structsize, 0);
    if (rs < 0){
        perror("eroare la recv");
        exit(0);
    }
    if (rs == 0){
        printf("A aparut o problema\n");
        exit(0);
    }
    strcpy(topic, msg.str);
    memcpy(&mod, msg.str + 50, 1); 

    if(mod == 0){
        uint8_t a4 = msg.str[55];
        uint8_t a3 = msg.str[54];
        uint8_t a2 = msg.str[53];
        uint8_t a1 = msg.str[52];
        res = (a1 << 24) + (a2 << 16) + (a3 << 8) + a4;
        semn =  msg.str[51];
        if(semn)
            res = res * -1;
        printf("%s:%d - %s - INT - %d\n", msg.ip, msg.nr_port, topic, res);                    
        return;
    }
    if(mod == 1){
        uint8_t a1 = msg.str[52];
        uint8_t a2 = msg.str[51];
        res = (a2 << 8) + a1;
        printf("%s:%d - %s - SHORT_REAL - %.2f\n", msg.ip, msg.nr_port, topic, (float)res / 100);
        return;
    }
    if (mod == 2){
        puterea = msg.str[56];
        uint8_t a4 = msg.str[55];
        uint8_t a3 = msg.str[54];
        uint8_t a2 = msg.str[53];
        uint8_t a1 = msg.str[52];
        semn =  msg.str[51];
        res = (a1 << 24) + (a2 << 16) + (a3 << 8) + a4;
        if(semn)
            res = res * -1;
        int multiplu = 1;
        for(i = 0; i < puterea; i++)
            multiplu *= 10;            
        printf("%s:%d - %s - FLOAT - %.4f\n",msg.ip, msg.nr_port, topic, (float)res / multiplu);
        return;
    }
    if (mod == 3){
        printf("%s:%d - %s - STRING - %s\n",msg.ip,msg.nr_port,topic, msg.str + 51);
        return;
    }
}

int main(int argc, char *argv[]){
    char address;    
    struct sockaddr_in serv_addr;
    int fdmax = 0, i;
    if (argc != 4){
        perror("Usage ./subscriber <ID_Client> <IP_Server> <Port_Server>");
        exit(0);
    }
    int idlen = strlen(argv[1]);
    if(idlen > 20){
        perror("S-a depasit lungimea maxima a id-ului");
        exit(0);
    }
    int tcpsocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpsocket == -1){
        perror("eroare la deschiderea socketului tcp");
        exit(0);
    }
    serv_addr.sin_family = AF_INET;
    inet_aton(argv[2], &serv_addr.sin_addr);
    serv_addr.sin_port = htons(atoi(argv[3]));
    unsigned int serv_size = sizeof(struct sockaddr_in);
    struct sockaddr* serv_ad = (struct sockaddr*) &serv_addr;
    if (connect(tcpsocket, serv_ad, serv_size) < 0){
        perror("eroare la connect");
        exit(0);
    }
    int len_id = strlen(argv[1]);
    if (send(tcpsocket, argv[1], len_id, 0) < 0){
        perror("eroare la send");
        exit(0);
    }
    fd_set readdsc, temporarydsc;
    FD_ZERO(&readdsc);
    FD_ZERO(&temporarydsc);
    FD_SET(0, &readdsc);
    FD_SET(tcpsocket, &readdsc);
    if(tcpsocket > fdmax)
        fdmax = tcpsocket;
    if (setsockopt(tcpsocket, IPPROTO_TCP, TCP_NODELAY, &address, sizeof(int)) < 0){
        perror("eroare la setsockopt");
        exit(0);
    }
    for( ; ; ) {
        temporarydsc = readdsc;
        int sl = select(fdmax + 1, &temporarydsc, NULL, NULL, NULL); 
		if (sl < -1){
			perror("eroare la select");
            exit(0);
        }
        for(i = 0; i < fdmax + 1; ++i)
            if(FD_ISSET(i, &temporarydsc)){
                if (i == tcpsocket)
                    parseBuffer(tcpsocket);
                else if(readInstruction(tcpsocket) == -1)
                    printf("Posibile instructiuni: exit, subscribe topic SF, unsubscribe topic\n");
            }
    }
    return 0;
}
