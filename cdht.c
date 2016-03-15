#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>

int successor_id[2], precessor_id[2];
int my_id, quit_sum = 0;
struct hostent *h;
int server_sd, client_sd, udp_sd;
struct sockaddr_in local_addr, server_addr;
struct sockaddr_in udp_local_addr, successor_addr1, successor_addr2;
int addr_len = sizeof(struct sockaddr_in);
char * local_ip = "localhost";
int port_base = 50000, random_port = 10000;
void update_successor(int quit_num, int m);
void ping_successors();
void *thread_listen(void *arg);
void *thread_function2(void *arg);
void *listen_thread(void *arg);
int main(int argc, char *argv[])
{
    if(argc < 4)
    {
        perror("cannot start peer");
        return 1;
    }
    int cli_rc, ser_rc, udp_rc;
    pthread_t a_thread1, a_thread2;
    void *thread_result1;
    void *thread_result2;
    h = gethostbyname(local_ip);
    if(h==NULL)
    {
        printf("Unknown host '%s'\n", local_ip);
        exit(1);
    }

    my_id = atoi(argv[1]);
    printf("This is peer %d\n", my_id);
    successor_id[0] = atoi(argv[2]);
    successor_id[1] = atoi(argv[3]);
    precessor_id[0] = 0;
    precessor_id[1] = 0;
    /* create socket */
    server_sd = socket(AF_INET, SOCK_STREAM, 0);
    client_sd = socket(AF_INET, SOCK_STREAM, 0);
    udp_sd = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_sd<0||client_sd<0||server_sd<0)
    {
        perror("cannot open socket ");
        return 1;
    }

    /* client addr */
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(port_base+10000 + my_id);
    udp_local_addr.sin_family = AF_INET;
    udp_local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_local_addr.sin_port = htons(port_base+my_id);
    /* server addr */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port_base + my_id);
    /* successors addr */
    successor_addr1.sin_family = AF_INET;
    successor_addr1.sin_port = htons(port_base+successor_id[0]);
    memcpy((char *) &successor_addr1.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    successor_addr2.sin_family = AF_INET;
    successor_addr2.sin_port = htons(port_base+successor_id[1]);
    memcpy((char *) &successor_addr2.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    /* bind socket with addr */
    udp_rc = bind(udp_sd, (struct sockaddr *) &udp_local_addr, sizeof(udp_local_addr));
    if(udp_rc<0)
    {
        printf("Error: Peer %d: cannot bind udp", my_id);
        exit(1);
    }
    cli_rc = bind(client_sd, (struct sockaddr *) &local_addr, sizeof(local_addr));
    if(cli_rc<0)
    {
        printf("Error: Peer %d: client bind tcp", my_id);
        exit(1);
    }

    printf("Ping successors in initialization!\n");
    ping_successors();
    printf("Ping process finished!\n");
    //close(udp_sd);

    ser_rc = bind(server_sd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if(ser_rc < 0)
    {
        perror("server cannot bind port ");
        return 1;
    }
    /* listen for client request */
    if(listen(server_sd, 20)<0)
    {
        perror("listen error");
        return -1;
    }

    sleep(1);

    cli_rc = connect(client_sd, (struct sockaddr *) &successor_addr1, sizeof(successor_addr1));
    if(cli_rc<0)
    {
        perror("cannot connect successor using tcp");
        exit(1);
    }

    //int flags = fcntl(server_sd, F_GETFL, 0);
    //fcntl(server_sd, F_SETFL, flags | O_NONBLOCK);
    pthread_create(&a_thread1, NULL, thread_listen, NULL);
    while(1)
    {
        char command[50];
        //pthread_create(&a_thread2, NULL, thread_function2, NULL);

        scanf("%s", command);
        if(!strcmp(command, "request"))
        {
            int file_num, tmp_fnum;
            char file_str[10], my_port_str[10];
            scanf("%s", file_str);
            printf("File request message for %s has been sent to my successor.\n", file_str);
            char request_file[100] = {"request "};
            //sprintf(file_str, "%d", file_num);
            sprintf(my_port_str, "%d", my_id+port_base);
            strcat(request_file, file_str);
            strcat(request_file, " ");
            strcat(request_file, my_port_str);
            int rc = send(client_sd, request_file, strlen(request_file) + 1, 0);
            //printf("send: %s\n", request_file);
            if(rc<0)
            {
                perror("cannot send data ");
                exit(1);
            }
        }
        else if(!strcmp(command, "quit"))
        {
        	//printf("%d %d\n", precessor_id[0], precessor_id[1]);
        	struct sockaddr_in tmp_local_addr1, tmp_local_addr2;
            int tmp_sd1, tmp_sd2;
            struct sockaddr_in pre_addr1, pre_addr2;
            pre_addr1.sin_family = AF_INET;
            pre_addr1.sin_port = htons(port_base+precessor_id[0]);
            memcpy((char *) &pre_addr1.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
            tmp_local_addr1.sin_family = AF_INET;
            tmp_local_addr1.sin_addr.s_addr = htonl(INADDR_ANY);
            tmp_local_addr1.sin_port = htons(port_base-random_port-150 + my_id);
            tmp_sd1 = socket(AF_INET, SOCK_STREAM, 0);
            if(bind(tmp_sd1, (struct sockaddr *) &tmp_local_addr1, sizeof(tmp_local_addr1)) < 0)
            {
                printf("Error: cannot bind precessor1\n");
                exit(1);
            }
            if(connect(tmp_sd1, (struct sockaddr *) &pre_addr1, sizeof(pre_addr1))<0)
            {
                printf("Error: cannot connect precessor1\n");
                exit(1);
            }

            char quit_str1[20] = {"quit "};
            char next1_str[10], my_port_str[10];
            sprintf(my_port_str, "%d", my_id+port_base);
            strcat(quit_str1, my_port_str);
            strcat(quit_str1, " ");
            sprintf(next1_str, "%d", successor_id[0]);
            strcat(quit_str1, next1_str);
            if(send(tmp_sd1, quit_str1, strlen(quit_str1) + 1, 0) < 0)
            {
                printf("Error: cannot send quit msg to precessor1\n");
                exit(1);
            }
            close(tmp_sd1);

            pre_addr2.sin_family = AF_INET;
            pre_addr2.sin_port = htons(port_base+precessor_id[1]);
            memcpy((char *) &pre_addr2.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
            tmp_local_addr2.sin_family = AF_INET;
            tmp_local_addr2.sin_addr.s_addr = htonl(INADDR_ANY);
            tmp_local_addr2.sin_port = htons(port_base-random_port-200 + my_id);
            tmp_sd2 = socket(AF_INET, SOCK_STREAM, 0);
            if(bind(tmp_sd2, (struct sockaddr *) &tmp_local_addr2, sizeof(tmp_local_addr2)) < 0)
            {
                printf("Error: cannot bind precessor2\n");
                exit(1);
            }
            if(connect(tmp_sd2, (struct sockaddr *) &pre_addr2, sizeof(pre_addr2))<0)
            {
                printf("Error: cannot connect precessor2\n");
                exit(1);
            }
            char quit_str2[20] = {"quit "};
            char next2_str[10];
            sprintf(my_port_str, "%d", my_id+port_base);
            strcat(quit_str2, my_port_str);
            strcat(quit_str2, " ");
            sprintf(next2_str, "%d", successor_id[1]);
            strcat(quit_str2, next2_str);
            if(send(tmp_sd2, quit_str2, strlen(quit_str2) + 1, 0) < 0)
            {
                printf("Error: cannot send quit msg to precessor1\n");
                exit(1);
            }
            close(tmp_sd2);
        }
        else if(!strcmp(command, "bye"))
        {
            close(client_sd);
            close(server_sd);
            return 0;
        }
        getchar();
    }


    return 0;
}
void update_successor(int quit_num, int m)
{
    close(client_sd);
    client_sd = socket(AF_INET, SOCK_STREAM, 0);
    int tmp_sd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in tmp_local_addr;
    int tmp_rc;

    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(port_base-random_port + my_id);
    random_port += 20;
    tmp_local_addr.sin_family = AF_INET;
    tmp_local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    tmp_local_addr.sin_port = htons(port_base-random_port + my_id);
    random_port += 20;
    successor_addr1.sin_family = AF_INET;
    successor_addr1.sin_port = htons(port_base+successor_id[0]);
    memcpy((char *) &successor_addr1.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    successor_addr2.sin_family = AF_INET;
    successor_addr2.sin_port = htons(port_base+successor_id[1]);
    memcpy((char *) &successor_addr2.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    if(bind(client_sd, (struct sockaddr *) &local_addr, sizeof(local_addr)) < 0)
    {
        printf("Error: Peer %d: client bind tcp in update", my_id);
        exit(1);
    }
    if(connect(client_sd, (struct sockaddr *) &successor_addr1, sizeof(successor_addr1)) < 0)
    {
        perror("cannot connect successor using tcp in update");
        exit(1);
    }
    if(m == 0)
        tmp_rc = bind(tmp_sd, (struct sockaddr *) &tmp_local_addr, sizeof(tmp_local_addr));
    else
        tmp_rc  = bind(tmp_sd, (struct sockaddr *) &tmp_local_addr, sizeof(tmp_local_addr));
    if( tmp_rc< 0)
    {
        printf("Error: cannot bind update port\n");
        exit(1);
    }
    if(m == 0)
        tmp_rc = connect(tmp_sd, (struct sockaddr *) &successor_addr1, sizeof(successor_addr1));
    else
        tmp_rc  = connect(tmp_sd, (struct sockaddr *) &successor_addr2, sizeof(successor_addr2));
    if(tmp_rc<0)
    {
        printf("Error: cannot connect update server\n");
        exit(1);
    }

    char update_str[20] = {"update "};
    char quit_num_str[20], my_port_str[10];
    sprintf(my_port_str, "%d", my_id+port_base);
    sprintf(quit_num_str, "%d", quit_num);
    strcat(update_str, my_port_str);
    strcat(update_str, " ");
    strcat(update_str, quit_num_str);
    if(send(tmp_sd, update_str, strlen(update_str) + 1, 0) < 0)
    {
        printf("Error: cannot send update message\n");
        exit(1);
    }
}
//ping successors
void ping_successors()
{
    int i, j;
    char recv_buffer[1000];
    fd_set fds;
    struct timeval timeout= {1,0};
    struct sockaddr_in udp_res_addr;

    for(i = 0; i < 10; i++)
    {
        int recv_port;
        char head[20], my_port_str[10];
        char send_buffer[100] = {"request from "};
        char resp_buffer[1000] = {"respons from "};
        memset(head, 0, sizeof(head));
        memset(recv_buffer, 0, sizeof(recv_buffer));

        sprintf(my_port_str, "%d", my_id+port_base);
        strcat(send_buffer, my_port_str);
        sendto(udp_sd, send_buffer, strlen(send_buffer), 0, (struct sockaddr *)&successor_addr1, addr_len);
        sendto(udp_sd, send_buffer, strlen(send_buffer), 0, (struct sockaddr *)&successor_addr2, addr_len);
        //printf("I sent a message: %s\n", send_buffer);
        //udp recieve from precessors
        if(i == 0)
        {
            sleep(3);
            continue;
        }

        int len = recvfrom(udp_sd, recv_buffer, sizeof(recv_buffer), MSG_DONTWAIT, (struct sockaddr *)&udp_local_addr ,&addr_len);
        if(len > 0)
        {
        	//printf("I received: %s\n", recv_buffer);
            char port_str[6];
            for(int s = 0, t = 13; s < 5; s++, t++)
                port_str[s] = recv_buffer[t];
            recv_port = atoi(port_str) - port_base;
            strncpy(head, recv_buffer, 7);
            if(!strcmp(head, "request"))
            {
                if(precessor_id[0] == 0)
                    precessor_id[0] = recv_port;
                else if(precessor_id[0] != recv_port && precessor_id[1] == 0)
                {
                    precessor_id[1] = recv_port;
                    if(precessor_id[0] > precessor_id[1])
                    {
                        int tmp = precessor_id[0];
                        precessor_id[0] = precessor_id[1];
                        precessor_id[1] = tmp;
                    }
                    if(precessor_id[0] < my_id && precessor_id[1] > my_id)
                    {
                    	int tmp = precessor_id[0];
                        precessor_id[0] = precessor_id[1];
                        precessor_id[1] = tmp;
                    }
                }
                printf("A ping request message was received from Peer %d.\n", recv_port);
                udp_res_addr.sin_family = AF_INET;
                udp_res_addr.sin_port = htons(port_base+recv_port);
                memcpy((char *) &udp_res_addr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
                strcat(resp_buffer, my_port_str);
                if(sendto(udp_sd, resp_buffer, strlen(resp_buffer), 0, (struct sockaddr *)&udp_res_addr, addr_len)<0)
                {
                    printf("Error: cannot send response to precessor\n");
                    exit(1);
                }
            }
            else if(!strcmp(head, "respons"))
            {
                printf("A ping response message was received from Peer %d.\n", recv_port);
            }
        }
        sleep(3);
    }
}
//listen for clients
void *thread_listen(void *arg)
{
    int cli_len, new_sd, k = 0;
    struct sockaddr_in cli_addr;
    pthread_t b_thread[20];
    while(1)
    {
        cli_len = sizeof(cli_addr);
        new_sd = accept(server_sd, (struct sockaddr *) &cli_addr, &cli_len);
        if(new_sd<0)
        {
            if (errno==EAGAIN || errno == EWOULDBLOCK)
            {
                sleep(2);
                continue;
            }
            else
            {

                perror("cannot accept connection ");
                exit(1);
            }
        }
        //printf("accepted\n");
		char szIp[17];
        memset(szIp, 0, sizeof(szIp));
        inet_ntop(AF_INET,&cli_addr.sin_addr,szIp,16);
        //printf("from client IP:%s,Port:%d\n",szIp,ntohs(cli_addr.sin_port));
        /* create a new thread after accepting a new client */
        pthread_create(&b_thread[k++], NULL, listen_thread, (void *)new_sd);
    }
}
//receive message from client
void *listen_thread(void *arg)
{
	int new_sd = (int)arg;
	char line[100];
	memset(line, 0, sizeof(line));
 	while(1)
    {
    	    int n = recv(new_sd, line, 100, 0);
            if (n<0)
            {
                perror(" cannot receive data ");
                exit(1);
            }
            else if(n == 0)
            {
            	close(new_sd);
            	return NULL;
            }
            else
            {
            	//printf("Received: %s\n", line);
            	if(line[2] == 'q')
                {
                    char file_str[5];
                    int file_num, tmp_num;
                    int s = 0;
                    for(s = 0; s < 4; s++)
                        file_str[s] = line[8+s];
                    file_str[s] = '\0';
                    tmp_num = atoi(file_str);
                    file_num = (tmp_num+1)%256;
                    if((file_num==my_id)||(file_num>=my_id&&file_num<successor_id[0])||(my_id > successor_id[0]&&file_num<successor_id[0])||(my_id > successor_id[0]&&file_num>=my_id))
                    {
                        printf("File %04d is stored here.\n", tmp_num);
                        char dest_str[6], response_str[100], my_port_str[10];
                        int dest_num;
                        for(s = 0; s < 5; s++)
                            dest_str[s] = line[13+s];
                        dest_str[s] = '\0';
                        dest_num = atoi(dest_str) - port_base;
                        sprintf(my_port_str, "%d", my_id+port_base);
                        strcpy(response_str, line);
                        response_str[2] = 's';
                        for(s = 0; s < 5; s++)
                            response_str[13+s] = my_port_str[s];
                        response_str[13+s] = '\0';
                        struct sockaddr_in res_addr;
                        res_addr.sin_family = AF_INET;
                        res_addr.sin_port = htons(port_base+dest_num);
                        memcpy((char *) &res_addr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);

                        struct sockaddr_in tmp_local_addr;
                        int tmp_sd = socket(AF_INET, SOCK_STREAM, 0);
                        tmp_local_addr.sin_family = AF_INET;
                        tmp_local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                        tmp_local_addr.sin_port = htons(port_base-random_port + my_id);
                        random_port += 20;
                        if(bind(tmp_sd, (struct sockaddr *) &tmp_local_addr, sizeof(tmp_local_addr)) < 0)
                        {
                            printf("Error: cannot bind response\n");
                            exit(1);
                        }
                        if(connect(tmp_sd, (struct sockaddr *) &res_addr, sizeof(res_addr))<0)
                        {
                            printf("Error: cannot connect response\n");
                            exit(1);
                        }
                        if(send(tmp_sd, response_str, strlen(response_str) + 1, 0) < 0)
                        {
                            printf("Error: cannot send response\n");
                            exit(1);
                        }
                        printf("A response message, destined for peer %d, has been sent.\n", dest_num);
                        close(tmp_sd);
                    }
                    else
                    {
                        printf("File %04d is not stored here.\n", tmp_num);
                        int rc = send(client_sd, line, strlen(line) + 1, 0);
                        if(rc<0)
                        {
                            perror("cannot send request data to successor.\n");
                            exit(1);
                        }
                        printf("File request message has been forwarded to my successor.\n");
                    }
                }
                else if(line[2] == 's')
                {
                    int s = 0;
                    char file_str[5], dest_str[6];
                    int file_num, dest_num;
                    for(s = 0; s < 4; s++)
                        file_str[s] = line[8+s];
                    file_str[s] = '\0';
                    file_num = atoi(file_str);
                    for(s = 0; s < 5; s++)
                        dest_str[s] = line[13+s];
                    dest_str[s] = '\0';
                    dest_num = atoi(dest_str) - port_base;
                    printf("Received a response message from peer %d, which has the file %04d\n", dest_num, file_num);
                }
                else if(line[2] == 'i')
                {
                	int s = 0;
                	char quit_str[10], next_str[10];
                	int quit_num, next_num;
                	for(s = 0; s < 5; s++)
                		quit_str[s] = line[5+s];
                	quit_str[s] = '\0';
                	quit_num = atoi(quit_str) - port_base;
                	printf("Peer %d will depart from the network.\n", quit_num);
                	for(s = 11; s < strlen(line); s++)
                		next_str[s-11] = line[s];
                	next_str[s-11] = '\0';
                	next_num = atoi(next_str);
                    int m;
                    int back_id[2];
                    back_id[0] = successor_id[0];
                    back_id[1] = successor_id[1];
                	if(successor_id[0] == quit_num)
                	{
                        m = 0;
                    	successor_id[0] = next_num;
                    }
                	else
                	{
                        m = 1;                        
                    	successor_id[1] = next_num;
                    }
                	if((successor_id[0]>successor_id[1]&&successor_id[1]>my_id)||(successor_id[0]<my_id&&successor_id[1]>my_id))
                    {
                        m = !m;
                        int tmp = successor_id[0];
                        successor_id[0] = successor_id[1];
                        successor_id[1] = tmp;
                    }
                    if(successor_id[0]<my_id&&successor_id[1]<my_id&&successor_id[0]>successor_id[1])
                    {
                        m = !m;
                        int tmp = successor_id[0];
                        successor_id[0] = successor_id[1];
                        successor_id[1] = tmp;
                    }
                    //printf("inform: %d\n", successor_id[m]);
                    update_successor(quit_num, m);
                	printf("My first successor is now peer %d.\n", successor_id[0]);
                	printf("My second successor is now peer %d.\n", successor_id[1]);
                    struct sockaddr_in res_addr;
                    res_addr.sin_family = AF_INET;
                    res_addr.sin_port = htons(port_base+quit_num);
                    memcpy((char *) &res_addr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);

                    struct sockaddr_in tmp_local_addr;
                    int tmp_sd = socket(AF_INET, SOCK_STREAM, 0);
                    tmp_local_addr.sin_family = AF_INET;
                    tmp_local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                    tmp_local_addr.sin_port = htons(port_base-random_port-250+my_id);
                    char agree_msg[5] = {"bye"};
                    if(bind(tmp_sd, (struct sockaddr *) &tmp_local_addr, sizeof(tmp_local_addr)) < 0)
                    {
                        printf("Error: cannot bind quit port\n");
                        exit(1);
                    }
                    if(connect(tmp_sd, (struct sockaddr *) &res_addr, sizeof(res_addr))<0)
                    {
                        printf("Error: cannot connect quit port\n");
                        exit(1);
                    }
                    if(send(tmp_sd, agree_msg, strlen(agree_msg) + 1, 0) < 0)
                    {
                        printf("Error: cannot send response to quit port\n");
                        exit(1);
                    }
                }
                else if(line[2] == 'e')
                {
                	quit_sum++;
                	if(quit_sum == 2)
                		exit(0);
                }
                else if(line[2] == 'd')
                {
                    int s = 0, update_num, change_num;
                    char update_str[10], change_str[10];
                    for(s = 7; s < 12; s++)
                        update_str[s-7] = line[s];
                    update_num = atoi(update_str);
                    for(s = 13; s < strlen(line); s++)
                        change_str[s-13] = line[s];
                    change_num = atoi(change_str);
                    //printf("change: %d %d\n", change_num, update_num);
                    if(precessor_id[0] == change_num)
                        precessor_id[0] = update_num-port_base;
                    else
                        precessor_id[1] = update_num-port_base;
                    if((precessor_id[0]>precessor_id[1]&&precessor_id[1]>my_id)||(precessor_id[0]<my_id&&precessor_id[1]>my_id))
                    {
                        int tmp = precessor_id[0];
                        precessor_id[0] = precessor_id[1];
                        precessor_id[1] = tmp;
                    }
                    if(precessor_id[0]<my_id&&precessor_id[1]<my_id&&precessor_id[0]>precessor_id[1])
                    {
                        int tmp = precessor_id[0];
                        precessor_id[0] = precessor_id[1];
                        precessor_id[1] = tmp;
                    }
                    //printf("my precessor_id: %d %d\n", precessor_id[0], precessor_id[1]);
                }
            }
            memset(line, 0, sizeof(line));
        
    }
}

