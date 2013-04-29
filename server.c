/******************************************************************************
 * tcp_server.c
 *
 * CPE 464 - Program 1
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>


#include "networks.h"
#include "cpe464.h"

int welcome_socket;     //welcome door socket
fd_set socks;           //socket fds we want to listen to
int highest_socket;     //socket with highest val for select
int num_entries  = 0;
int num_allocations = 5;
struct handle_entry *connection_list;

main(int argc, char *argv[])
{
    struct message_packet message;
    struct timeval timeout;     //timeout for select
    int client_socket= 0;       //socket descriptor for the client socket
    char *buf;                  //buffer for receiving from client
    int buffer_size= 3072;      //packet size variable
    int message_len= 0;         //length of the received message
    int ready_sockets_count= 0; //number of sockets ready for reading
    double percent_error=0;
    //struct handle_entry connection_list[MAX_HANDLE_ENTRY];
    connection_list = malloc (5 * sizeof(struct handle_entry));
    
    if(argc !=2 )
    {
        printf("bad params\n");
        exit(-1);
    }
    
    printf("sockaddr: %d sockaddr_in %d\n", sizeof(struct sockaddr), sizeof(struct sockaddr_in));
    
    percent_error = strtod(argv[1], (char **) NULL);
    sendErr_init(percent_error, DROP_ON, FLIP_OFF, DEBUG_OFF, RSEED_ON);
    
    if(connection_list == NULL)
    {
        printf("shit was null\n");
        perror("malloc");
        exit(-1);
    }
    
    //create packet buffer
    //buf=  (char *) malloc(buffer_size);

    //create the server socket
    welcome_socket = tcp_recv_setup();
    
    if(listen(welcome_socket, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    highest_socket = welcome_socket;
    
    
    while (1)
    {
        build_select_list();
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        ready_sockets_count = select(highest_socket+1, &socks, (fd_set *)0, (fd_set *)0, &timeout);
        
        if(ready_sockets_count < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }
        else if(ready_sockets_count == 0)
        {
            printf(".");
        }
        else
        {
            read_sockets();
        }
    }
    
    //printf("Message received, length: %d\n", message_len);
    //printf("Data: %s\n", buf);
    
    /* close the sockets */
    close(welcome_socket);
    close(client_socket);
    free(connection_list);
}

void build_select_list()
{
    int i;
    
    FD_ZERO(&socks);
    FD_SET(welcome_socket, &socks);
    
    for(i=0;i<num_allocations;i++)
    {
        if(connection_list[i].handle_in_use != 0)
        {
            FD_SET(connection_list[i].socket_num, &socks);
            if(connection_list[i].socket_num > highest_socket)
                highest_socket = connection_list[i].socket_num;
        }
    }
}

void read_sockets()
{
    int i;
    
    if(FD_ISSET(welcome_socket, &socks))
    {
        handle_new_connection();
    }

    for(i=0;i<num_allocations;i++)
    {
        if(FD_ISSET(connection_list[i].socket_num, &socks))
        {
            handle_data(i);
        }
    }
}

void handle_new_connection()
{
    int i, j;
    int client_socket;
    struct handle_entry entry;
    
    if ((client_socket= accept(welcome_socket, (struct sockaddr*)0, (socklen_t *)0)) < 0)
    {
        perror("accept call");
        exit(EXIT_FAILURE);
    }
    
    set_non_blocking(client_socket);
   
    entry.socket_num = client_socket;
    entry.handle_in_use = 1;
    AddToArray(entry);
}

int is_valid_handle(char *entry)
{
    int i=0;
    int is_valid=0;
    
    for(i=0;i<num_allocations;i++)
    {
        if((strcmp(connection_list[i].handle, entry) == 0) &&(connection_list[i].handle_in_use == 1))
        {
            is_valid = connection_list[i].socket_num;
        }
    }
    return is_valid;
}



void handle_data(int index)
{
    int message_len = 0;
    struct message_packet message;
    struct handle_entry *entry;
    int handle_len = 0;
    int j, i = 0;
    int sent, socket_num=0;
    uint32_t p;
    u_short c_sum;
    
    entry = &connection_list[index];
    
    if ((message_len = recv(entry->socket_num, &message, sizeof(message), 0)) < 0)
    {
            perror("recv call");
            exit(-1);
    }
    print_message(&message, RECIEVED);
    
    c_sum = in_cksum((unsigned short*)&message, sizeof(	message));
    
    if(c_sum != 0)
    {
        printf("toss the fucker\n");
    }
    else if(message.flag == FLAG_1)
    {
        connection_list[index].handle_in_use = 1;
        if(fill_handle_entry(&message, entry) == 1)
        {
            s_send_normal_header_packet(message.sequence_number, FLAG_2, entry->socket_num);
        }
        else
        {
            s_send_normal_header_packet(message.sequence_number, FLAG_3, entry->socket_num);
            connection_list[index].handle_in_use = 0;
            num_entries--;
            close(entry->socket_num);
        }
    }
    else if(message.flag == FLAG_6)
    {
        if((socket_num = handle_message(&message, entry->socket_num)) > 0)
        {
            send_message_packet(socket_num, message);
        }
        else
        {
            //sequence_number++;
            //s_send_normal_header_packet(sequence_number, FLAG_7, entry->socket_num);
        }
    }
    else if(message.flag == FLAG_8)
    {
        s_send_normal_header_packet(message.sequence_number, FLAG_9, entry->socket_num);
        remove_from_array(*entry);
    }
    else if(message.flag == FLAG_10)
    {
        s_send_number_of_handles(message.sequence_number, entry->socket_num);
    }
    else if(message.flag == FLAG_12)
    {
        memcpy(&p, message.data, sizeof(p));
        handle_request(p, message.sequence_number, entry->socket_num);
    }
    else if(message.flag == FLAG_255)
    {
        struct message_ack *ack;
        ack = (struct message_ack *)&message;
        if((socket_num = handle_ack_message(ack)) > 0)
        {
            send_message_packet(socket_num, message);
        }
        else
        {
            printf("if were here something bad happend when acking a message");
        }
    }
}

void handle_request(uint32_t requestNum, uint32_t sequence_number, int socket_num)
{
    struct message_packet message;
    struct handle_entry entry;
    int i, j=0;
    uint32_t num_full;
    u_short c_sum=0;
    
    message.sequence_number = sequence_number;
    message.check_sum = 0;
    message.flag = FLAG_13;
    
    for(i=0;i<num_allocations;i++)
    {
        if(connection_list[i].handle_in_use == 1)
        {
            num_full++;
        }
        if(num_full == requestNum)
        {
            break;
        }
    }
    
    entry = connection_list[i];
    i=0;
    j=1;
    while(entry.handle[i] != '\0')
    {
        message.data[j] = entry.handle[i];
        i++;
        j++;
    }

    message.data[0]= i;
    
    c_sum = in_cksum((unsigned short*)&message, sizeof(message));
    message.check_sum = c_sum;
    
    send_message_packet(socket_num, message);
    
}

void s_send_number_of_handles(uint32_t sequence_number, int socket_num)
{
    u_short c_sum=0;
    struct message_handle_count c_message;
    struct message_packet *message;
    c_message.sequence_number = sequence_number;
    c_message.check_sum = 0;
    c_message.flag = FLAG_11;
    c_message.handle_count = num_entries;
    
    c_sum = in_cksum((unsigned short*)&c_message, sizeof(c_message));
    c_message.check_sum = c_sum;
    
    message = (struct message_packet *)&c_message;
    send_message_packet(socket_num, *message);
}


int handle_ack_message(struct message_ack *ack)
{
    int i, j, d_handle_size, socket_num=0;
    char d_handle[MAX_HANDLE_SIZE];
    
    d_handle_size = ack->data[D_HANDLE_SIZE_INDEX];
    i=0;
    for(j=1;j<=d_handle_size+1;j++)
    {
        d_handle[i] = ack->data[j];
        i++;
    }
    d_handle[d_handle_size] = '\0';
    
    socket_num = is_valid_handle(d_handle);
    
    return socket_num;
}

int handle_message(struct message_packet *message, int sender_socket)
{
    int i, j, d_handle_size, socket_num=0;
    char d_handle[MAX_HANDLE_SIZE];
    struct message_packet error_message;
    u_short c_sum=0;
    
    d_handle_size = message->data[D_HANDLE_SIZE_INDEX];
    
    i=0;
    for(j=1;j<=d_handle_size+1;j++)
    {
        d_handle[i] = message->data[j];
        error_message.data[j] = message->data[j];
        i++;
    }
    d_handle[d_handle_size] = '\0';
    
    socket_num = is_valid_handle(d_handle);

    if(socket_num == 0) {
        error_message.flag = FLAG_7;
        error_message.sequence_number = message->sequence_number;
        error_message.check_sum = 0;
        error_message.data[0] = d_handle_size;
        
        c_sum = in_cksum((unsigned short*)&error_message, sizeof(error_message));
        error_message.check_sum = c_sum;
        
        send_message_packet(sender_socket, error_message);
    }
    
    return socket_num;
    
}

int fill_handle_entry(struct message_packet *message, struct handle_entry *entry)
{
    int i, j, check = 0;
    int handle_len=0;
    
    entry->handle_in_use = 1;
    entry->sequence_number = message->sequence_number;
    handle_len = message->data[S_I_HANDLE_SIZE_INDEX];
    i=0;
    for(j=1;j<=handle_len+1;j++)
    {
        entry->handle[i] = message->data[j];
        i++;
    }
    entry->handle[handle_len] = '\0';
    
    
    if(is_valid_handle(entry->handle) > 0)
    {
        printf("Handle is valid\n");
        return 1;
    }
    else
    {
        printf("That handle is already taken\n");
        return -1;
    }
    return 0;
}

void print_handle_entry(struct handle_entry entry, int index)
{
    int i=0;
    int handle_len = 0;
    
    handle_len = strlen(entry.handle)-1;
    printf("\n");
    printf("---Handle Entry slot %d---\n", index);
    printf("handle             = \"");
    for(i=0;i<handle_len;i++)
    {
        printf("%c", entry.handle[i]);
    }
    printf("\"\n");
    printf("socket number      = %d\n", entry.socket_num);
    printf("sequence number    = %d\n", entry.sequence_number);
    printf("--------------------------\n");
}

void s_send_normal_header_packet(uint32_t sequence_number, uint8_t flag, int socket_num)
{
    struct message_packet message;
    int sent = 0;
    u_short c_sum = 0;
    
    message.sequence_number = sequence_number;
    message.check_sum = 0;
    message.flag = flag;
    
    c_sum = in_cksum((unsigned short*)&message, sizeof(message));
    message.check_sum = c_sum;
    
    print_message(&message, SENDING);

    send_message_packet(socket_num, message);
}

void send_message_packet(int socket_num, struct message_packet message)
{
    int sent =0;
    
    sent = sendErr(socket_num, (char *)&message, sizeof(message), 0);
    if(sent < 0)
    {
        perror("send call");
        exit(-1);
    }
}


void set_non_blocking(int welcome_socket)
{
    int options;
    
    options = fcntl(welcome_socket, F_GETFL);
    if(options < 0)
    {
        perror("fcntl(F_GETFL)");
        exit(EXIT_FAILURE);
    }
    options = (options | O_NONBLOCK);
    if (fcntl(welcome_socket, F_SETFL, options) < 0)
    {
        perror("fcntl(F_SETFL)");
        exit(EXIT_FAILURE);
    }
}

/* This function sets the server socket.  It lets the system
   determine the port number.  The function returns the server
   socket number and prints the port number to the screen.  */

int tcp_recv_setup()
{
    int welcome_socket= 0;
    struct sockaddr_in local;      /* socket address for local side  */
    socklen_t len= sizeof(local);  /* length of local address        */
    int reuse_addr = 1;            //address for rebinding
    
    /* create the socket  */
    welcome_socket= socket(AF_INET, SOCK_STREAM, 0);
    if(welcome_socket < 0)
    {
      perror("socket call");
      exit(1);
    }
    //allow welcom socket to be re-bound
    setsockopt(welcome_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
    set_non_blocking(welcome_socket);
    
    
    local.sin_family= AF_INET;         //internet family
    local.sin_addr.s_addr= INADDR_ANY; //wild card machine address
    local.sin_port= htons(0);                 //let system choose the port

    /* bind the name (address) to a port */
    if (bind(welcome_socket, (struct sockaddr *) &local, sizeof(local)) < 0)
      {
          perror("bind call");
          exit(-1);
      }
    
    //get the port name and print it out
    if (getsockname(welcome_socket, (struct sockaddr*)&local, &len) < 0)
    {
          perror("getsockname call");
          exit(-1);
    }

    printf("socket has port %d \n", ntohs(local.sin_port));
	        
    return welcome_socket;
}

/* This function waits for a client to ask for services.  It returns
   the socket number to service the client on.    */

int tcp_listen(int welcome_socket, int back_log)
{
  int client_socket= 0;
  if (listen(welcome_socket, back_log) < 0)
    {
      perror("listen call");
      exit(-1);
    }
  
  if ((client_socket= accept(welcome_socket, (struct sockaddr*)0, (socklen_t *)0)) < 0)
    {
      perror("accept call");
      exit(-1);
    }
  
  return(client_socket);
}

void remove_from_array(struct handle_entry entry)
{
    int i;
    for(i=0;i<num_allocations;i++)
    {
        if(connection_list[i].socket_num == entry.socket_num)
        {
            close(connection_list[i].socket_num);
            connection_list[i].handle_in_use = 0;
            num_entries--;
        }
    }
}

int AddToArray(struct handle_entry entry)
{
    int i=0;

    
    if(num_entries == num_allocations)
    {
        //make some more space dawg
//        if(num_allocations == 0)
//        {
//            num_allocations = 3;
//        }
        
        
        num_allocations *= 2;
        
        
        void *_tmp = realloc (connection_list, (num_allocations * sizeof(struct handle_entry)));
        
        if(!_tmp)
        {
            fprintf(stderr, "ERRORL Couldnt realloc memory!\n");
            return -1;
        }
        connection_list = (struct handle_entry *)_tmp;
        connection_list[num_entries] = entry;
        printf("\nConnection accepts FD=%d, Slot %d\n", entry.socket_num, num_entries);
        num_entries++;
        return num_entries;
    }
    else if(num_entries < num_allocations)
    {
        //find an open spot yo!
        for(i=0;i<num_allocations;i++)
        {
            if(connection_list[i].handle_in_use == 0)
            {
                printf("");
                connection_list[i] = entry;
                printf("\nConnection accepts FD=%d, Slot %d\n", entry.socket_num, i);
                num_entries++;
                break;
            }
        }
        return num_entries;
    }
    else
    {
        printf("num_entries is higher than numallocate... bad news\n");
    }
    return -1;
}



/*
 * Copyright (c) 1989, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Muuss.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

unsigned short in_cksum(unsigned short *addr,int len)
{
    register int sum = 0;
    u_short answer = 0;
    register u_short *w = addr;
    register int nleft = len;
    
    /*
     * Our algorithm is simple, using a 32 bit accumulator (sum), we add
     * sequential 16 bit words to it, and at the end, fold back all the
     * carry bits from the top 16 bits into the lower 16 bits.
     */
    while (nleft > 1)  {
        sum += *w++;
        nleft -= 2;
    }
    
    /* mop up an odd byte, if necessary */
    if (nleft == 1) {
        *(u_char *)(&answer) = *(u_char *)w ;
        sum += answer;
    }
    
    /* add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
    sum += (sum >> 16);                     /* add carry */
    answer = ~sum;                          /* truncate to 16 bits */
    return(answer);
}







