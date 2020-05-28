#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>
#include <signal.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <errno.h>

#include "struct.h"

void checkExit(fd_set *temporarydsc){
    if (FD_ISSET(STDIN_FILENO, temporarydsc)) {
        char *read_buffer = calloc(BUFFLEN, sizeof(char));
        if(read(STDIN_FILENO, read_buffer, BUFFLEN) < 0)
            exit(-1);
        int len = strlen(read_buffer);
        read_buffer[len-1] = '\0';
        printf("S-a inchis serverul\n");
        if(strcmp(read_buffer,"exit") == 0)
            exit(0);
        else 
            printf("Unica instructiune admisa este exit\n");
        FD_CLR(STDIN_FILENO, temporarydsc);
    }
}

int checkInDatabase(struct User *vec, struct Database st, int dim){
    int i;
    for(i = 0; i < dim; i++)
        if(vec[i].id && strcmp(vec[i].id, st.id) == 0)
            return 0;
    return 1;
}

void sig_function(int sig_number){
  if (sig_number == SIGINT)
    exit(0);
}
void ctrlCreact(){
  struct sigaction sig;
  sig.sa_handler = sig_function;
  if(sigaction(SIGINT, &sig, 0))
    exit(0);
}
void udp_message(int udpsocket, int tcpsocket, int stdim, struct Database *save, int dim, struct User *vec,
    fd_set readdsc, int dimUsers){
    char topic[50], str[BUFFLEN];
    char ip[50];
    int nr_port;
    int i, j, nr_buff;
    struct SentPacket msg;
    struct Database st;
    struct sockaddr_in from_sock;
    struct sockaddr *sock_addr = (struct sockaddr *)&from_sock;
    socklen_t sock_len = sizeof(from_sock);
    unsigned int packet_size = sizeof(struct SentPacket);


    if (recvfrom(udpsocket, str, 1560, 0, sock_addr, &sock_len) <= 0)
        return;
    strcpy(ip, inet_ntoa(from_sock.sin_addr));
    nr_port = ntohs(from_sock.sin_port);
    memcpy(topic, str, 50);
    strcpy(msg.ip, ip);
    msg.nr_port = nr_port;
    memcpy(msg.str, str, BUFFLEN);
    char *casted_msg = (char*)&msg;

    for(i = 1; i - 1 < dimUsers; i++)
        if (FD_ISSET(i, &readdsc) && i != tcpsocket && i != udpsocket)
            for(j = 0; j < vec[i].nr_subscris; j++)
                if(strcmp(topic, vec[i].subscris[j]) == 0){
                    if(send(i, casted_msg, packet_size, 0) < 0){
                        perror("eroare transmitere");
                        exit(0);
                    }
                    break;
                }
    for (i = 0; i != stdim; i++)
        if(checkInDatabase(vec, save[i], dim)){
            st = save[i];
            for(j = 0; j < st.nr; j++){
                struct SfPacket *lpkt = save[i].packet;
                nr_buff = lpkt[j].nr_buff;
                if(strcmp(topic, st.packet[j].topic) == 0 && st.packet[j].sf_ind){
                        struct SentPacket *pkt = lpkt[j].packet;  
                        pkt[nr_buff].nr_port = nr_port;
                        strcpy(pkt[nr_buff].ip, ip);
                        memcpy(pkt[nr_buff].str, str, BUFFLEN);
                        lpkt[j].nr_buff++;
                    }
            }
        }    
}

void tcp_message(int tcpsocket, int stdim, struct Database *save, int *dim, struct User **vec,
    fd_set *readdsc, int *dimUsers, struct sockaddr_in cl_addr, int parameter){
    int i, j, k;
    unsigned int cllen = sizeof(cl_addr);
    int newtcpsocket = accept(tcpsocket, (struct sockaddr *)&cl_addr, &cllen);
    char *id_client = calloc(20, sizeof(char));
    unsigned int packet_size = sizeof(struct SentPacket);

    if(!id_client)
        exit(-1);
    
    if (newtcpsocket < 0){
        perror("eroare la accept");
        exit(0);
    }
    
    FD_SET(newtcpsocket, readdsc);
    if(newtcpsocket > *dimUsers)
        *dimUsers = newtcpsocket;
    if (recv(newtcpsocket, id_client, 20, 0) <= 0){
        perror("eroare la recv");
        exit(0);
    }
    
    if (!id_client){
        FD_CLR(newtcpsocket, readdsc);
        close(newtcpsocket);
        return;
    }

    for (i = 0; i < *dim; ++i){
        if((*vec)[i].id && strcmp((*vec)[i].id, id_client) == 0){
            FD_CLR(newtcpsocket, readdsc);
            close(newtcpsocket);
            return;
        }
    }
    printf("New client %s connected from %s:%d.\n", id_client, inet_ntoa(cl_addr.sin_addr), parameter);
    if(newtcpsocket > *dim){
        *vec = realloc(*vec, *dim * sizeof(struct User) * 2);
        *dim = newtcpsocket * 2;
    }
    (*vec)[newtcpsocket].id = id_client;

    for(i = 0; i < stdim; i++)
        if(strcmp(save[i].id, (*vec)[newtcpsocket].id) == 0)
            for(j = 0; j < save[i].nr; ++j){
                struct SfPacket *lpkt = save[i].packet;
                for(k = 0; k < lpkt[j].nr_buff; ++k){
                    char *sdpkt = (char*)&lpkt[j].packet[k];
                    if(send(newtcpsocket, sdpkt, packet_size, 0) < 0){
                        perror("eroare la send");
                        exit(0);
                    }
                }
                (*vec)[newtcpsocket].nr_subscris += 1;
                strcpy((*vec)[newtcpsocket].subscris[j], lpkt[j].topic);
                lpkt[j].nr_buff = 0;
            }
}

int findId(int index, int stdim, struct User *vec, struct Database *save){
    int i;
    for(i = 0; i < stdim; i++)
        if(strcmp(vec[index].id, save[i].id) == 0)
            return i;
    return -1;
}

int findTopic(int *i, int index, struct User *vec, struct Data sub){
    for(*i = 0; *i < vec[index].nr_subscris; (*i)++)
        if(strcmp(sub.topic, vec[index].subscris[*i]) == 0)
            return 1;
    return 0;
}

void old_conexion(int index, fd_set *readdsc, struct User *vec, int *stdim, struct Database *save){
    int i;
    struct Data sub;
    char *csub = (char *)&sub;
    unsigned int subsize = sizeof(struct Data);
    int res = recv(index, csub, subsize, 0);

    if(res < 0){
        perror("eroare la recv");
        exit(0);
    }
    if (res == 0) {
        printf("Client %s disconnected.\n", vec[index].id);
        int poz = findId(index, *stdim, vec, save);
        if(poz == -1){
            poz = *stdim;
            (*stdim)++;
        }

        strcpy(save[poz].id, vec[index].id);
        save[poz].nr = 0;
        for(i = 0; i < vec[index].nr_subscris; i++){
            int inr = save[poz].nr;
            struct SfPacket *lpkt = save[poz].packet;
            lpkt[inr].sf_ind = vec[index].sf_ind[i];
            memcpy(lpkt[inr].topic, vec[index].subscris[i], 50);       
            lpkt[inr].nr_buff = 0;
            save[poz].nr = inr + 1;
        }

        vec[index].nr_subscris = 0;
        vec[index].id = NULL;
        FD_CLR(index, readdsc);
        close(index);
    } else {
        i = 0;
        int existTopic = findTopic(&i,index, vec, sub);
        if(existTopic){
            if(sub.subscr_type){
                vec[index].nr_subscris--;
                for( ; i < vec[index].nr_subscris; i++){
                    vec[index].sf_ind[i] = vec[index].sf_ind[i + 1];
                    strcpy(vec[index].subscris[i], vec[index].subscris[i + 1]);
                }
            }
        }else{
            if(!sub.subscr_type){
                int inr = vec[index].nr_subscris;
                strcpy(vec[index].subscris[inr],sub.topic);
                vec[index].sf_ind[inr] = sub.sf_ind;
                vec[index].nr_subscris++;
            }
        }
        char *stopic = (char*)&existTopic;
        if(send(index, stopic, 4, 0) < 0){
            perror("eroare la send");
            exit(0);
        }
    }
}

int main(int argc, char *argv[]){
    if (argc != 2){
        perror("Usage ./server <PORT_DORIT>");
        exit(0);
    }
    ctrlCreact();
    struct Database save[30];
    int dim = 25, stdim = 0;
    char address;
     
    int udpsocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpsocket < 0){
        perror("eroare la crearea socketului udp");
        exit(0);
    }

    int tcpsocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpsocket < 0){
        perror("eroare la crearea tcpsocket");
        exit(0);
    }
    int rezsock = setsockopt(tcpsocket, IPPROTO_TCP, TCP_NODELAY, &address, sizeof(int));
    if (rezsock < 0){
        perror("eroare la setsockopt");
        exit(0);
    }
    struct sockaddr_in server_ad, cl_addr;
    server_ad.sin_port = htons(atoi(argv[1]));
    server_ad.sin_family = AF_INET;
    server_ad.sin_addr.s_addr = INADDR_ANY; 
    struct sockaddr *server_addr = (struct sockaddr *)&server_ad;
    unsigned int socksize = sizeof(struct sockaddr);

    if (bind(udpsocket, server_addr, socksize) < 0){
        perror("eroare la bind");
        exit(0);
    }
    if (bind(tcpsocket, server_addr, socksize) < 0){
        perror("eroare la bind");
        exit(0);
    }
    listen(tcpsocket, 25);
    fd_set temporarydsc, readdsc;     
    FD_ZERO(&temporarydsc);
    FD_ZERO(&readdsc);
    FD_SET(udpsocket, &readdsc);
    FD_SET(tcpsocket, &readdsc);
    int dimUsers = STDIN_FILENO;
    if(tcpsocket > dimUsers)
        dimUsers = tcpsocket;
    if(udpsocket > dimUsers)
        dimUsers = udpsocket;
    int i, parameter = atoi(argv[1]);
    struct User *vec = malloc(25 * sizeof(struct User));
    for(i = 0; i < dim; i++){
        struct User client;
        client.nr_subscris = 0;
        client.id = calloc(20, sizeof(char));
        vec[i] = client;
    }
    FD_SET(STDIN_FILENO, &readdsc);
    for( ; ; ){
        temporarydsc = readdsc; 
        if (select(dimUsers + 1, &temporarydsc, NULL, NULL, NULL) <= 0){
            perror("eroare la select");
            exit(0);
        }
        checkExit(&temporarydsc);
        i = 0;
        while (i - 1 < dimUsers){
            if (FD_ISSET(i, &temporarydsc)) {
                if (i == tcpsocket) {
                    tcp_message(tcpsocket, stdim, save, &dim, &vec, &readdsc, &dimUsers, cl_addr, parameter);
                } else if(i == udpsocket){
                    udp_message(udpsocket, tcpsocket, stdim, save, dim, vec, readdsc, dimUsers);
                } else {
                    old_conexion(i, &readdsc, vec, &stdim, save);
                }
            }
            i++;
        }
    }
    close(udpsocket);
    close(tcpsocket);
    return 0;
}
    