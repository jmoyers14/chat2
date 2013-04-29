/******************************************************************************
 * tcp_client.c
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

struct message_packet *watch_list[1024];
int num_entries=0;


main(int argc, char * argv[])
{
    int socket_num;           //socket descriptor
    int i =0;
    uint32_t p =0;
    int message_len=0;
    double percent_error;
    uint8_t *sender_handle;   //clients handle
    uint32_t sequence_number = 0;
    struct timeval timeout;
    fd_set read_socks;
    fd_set socks;
    int ready_sockets_count = 0;
    int raw_data_len= 0;      //amount of data to send
    int highest_socket = 0;
    struct message_packet message;
    struct message_packet *send_message;
    struct message_packet *init_packet;
    struct message_packet *num_request_message;
    u_short c_sum;
    /* check command line arguments  */
    if(argc!= 5)
    {
        printf("usage: %s handle percent-error host-name port-number \n", argv[0]);
        exit(1);
    }
        
    for(i=0;i<1024;i++)
        watch_list[i]=NULL;
    
    sender_handle = argv[1];
    percent_error = strtod(argv[2], (char **) NULL);
    
    /* set up the socket for TCP transmission  */
    socket_num = tcp_send_setup(argv[3], argv[4]);
    sendErr_init(percent_error, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_ON);
    
    
    if(strlen(sender_handle) > 100)
    {
        printf("Your handle may not exceed 100 characters\n");
        exit(1);
    }
    //send initial packet
    init_packet = malloc (sizeof(struct message_packet));
    send_initial_packet(init_packet, sequence_number, sender_handle, socket_num);
    add_to_watch_list(init_packet);
    

    printf(">");
    fflush(stdout);
    if(STDIN_FILENO > socket_num)
        highest_socket = STDIN_FILENO;
    else
        highest_socket = socket_num;
    
    while(1)
    {
        FD_ZERO(&socks);
        FD_SET(socket_num, &socks);
        FD_SET(STDIN_FILENO, &socks);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        ready_sockets_count = select(highest_socket+1, &socks, (fd_set *)0, (fd_set *)0, &timeout);
        
        if(ready_sockets_count < 0)
        {
            perror("select");
            exit(-1);
        }
        else if(ready_sockets_count == 0)
        {
            if(num_entries > 0)
                resend_messages(socket_num);
        }
        else
        {
            if(FD_ISSET(0, &socks))
            {
                send_message = malloc(sizeof(struct message_packet));
                if(handle_stdin(send_message, sequence_number, sender_handle, socket_num) >0)
                    add_to_watch_list(send_message);
                 
            }
            
            if(FD_ISSET(socket_num, &socks))
            {
                if ((message_len = recv(socket_num, &message, sizeof(message) + 1, 0)) < 0)
                {
                    perror("recv call");
                    exit(-1);
                }

                c_sum = in_cksum((unsigned short*)&message, sizeof(	message));
                if(c_sum != 0)
                {
                }
                else if(message.flag == FLAG_2)
                {
                    remove_from_watch_list(message.sequence_number);
                }
                else if(message.flag == FLAG_3)
                {
                    printf("that handle is already taken\n");
                    remove_from_watch_list(message.sequence_number);
                    exit(-1);
                }
                else if(message.flag == FLAG_6)
                {
                    output_message(&message);
                    sequence_number++;
                    send_ack_message(sequence_number, message.sequence_number, message, socket_num);
                }
                else if(message.flag == FLAG_7)
                {
                    print_handle_error(message);
                    remove_from_watch_list(message.sequence_number);
                }
                else if(message.flag == FLAG_9)
                {
                    close(socket_num);
                    for(i=0;i<1024;i++)
                    {
                        if(watch_list[i] != NULL)
                            free(watch_list[i]);
                    }
                    return EXIT_SUCCESS;
                }
                else if(message.flag == FLAG_11)
                {
                    struct message_handle_count *c_message;
                    c_message = (struct message_handle_count *)&message;
                    remove_from_watch_list(c_message->sequence_number);
                    printf("There are %d handles online\n", c_message->handle_count);
                    printf(">");
                    for(p=1;p<=c_message->handle_count;p++)
                    {
                        num_request_message = malloc(sizeof(struct message_packet));
                        sequence_number++;
                        request_handle_number(p, num_request_message, sequence_number, socket_num);
                        add_to_watch_list(num_request_message);
                    }
                    fflush(stdout);
                }
                else if(message.flag == FLAG_13)
                {
                    print_handle(message);
                    remove_from_watch_list(message.sequence_number);
                }
                else if(message.flag == FLAG_255)
                {
                    struct message_ack *ack;
                    ack = (struct message_ack *)&message;
                    remove_from_watch_list(ack->ack_sequence_number);
                }
            }
        }
    }
    

    return EXIT_SUCCESS;
}

void request_handle_number(uint32_t p, struct message_packet *message, uint32_t sequence_number, int socket_num)
{
    u_short c_sum=0;
    message->sequence_number = sequence_number;
    message->check_sum = 0;
    message->flag = FLAG_12;
    memcpy(message->data, &p, sizeof(p));
    
    c_sum = in_cksum((unsigned short*)message, sizeof(*message));
    message->check_sum = c_sum;
    
    c_send_message_packet(socket_num, message);
}

void print_handle(struct message_packet message)
{
    int j, d_handle_size = 0;
    printf("\n");
    printf("user: \"");
    d_handle_size = message.data[D_HANDLE_SIZE_INDEX];
    for(j=1;j<=d_handle_size;j++)
    {
        printf("%c", message.data[j]);
    }
    printf("\"\n");
    printf(">");
    fflush(stdout);
}


void print_handle_error(struct message_packet message)
{
    int j, d_handle_size = 0;
    printf("\n");
    printf("That handle \"");
    d_handle_size = message.data[D_HANDLE_SIZE_INDEX];
    for(j=1;j<=d_handle_size;j++)
    {
        printf("%c", message.data[j]);
    }
    printf("\" does not exist\n");
    printf(">");
    fflush(stdout);
}

void send_ack_message(uint32_t sequence_number, uint32_t ack_sequence_number, struct message_packet message, int socket_num)
{
    struct message_ack ack_message;
    int s_handle_size;
    int i, j=0;
    int dest_handle_size = 0; //size of destinatino handle
    int s_handle_size_index = 0;
    int s_handle_index = 0;
    int s_handle_end = 0;
    int sent=0;
    u_short c_sum=0;
    
    dest_handle_size = message.data[D_HANDLE_SIZE_INDEX];
    s_handle_size_index = 1 + dest_handle_size;
    s_handle_index = 2 + dest_handle_size;
    s_handle_size = message.data[s_handle_size_index];
    s_handle_end = s_handle_index + s_handle_size;
    
    ack_message.sequence_number = sequence_number;
    ack_message.check_sum = 0;
    ack_message.flag = FLAG_255;
    ack_message.ack_sequence_number = ack_sequence_number;
    
    ack_message.data[0] = s_handle_size;
    i=1;
    
    for(j=s_handle_index;j<s_handle_end;j++)
    {
        ack_message.data[i] = message.data[j];
        i++;
    }
    
    c_sum = in_cksum((unsigned short*)&ack_message, sizeof(ack_message));
    ack_message.check_sum = c_sum;
    
    c_send_message_packet(socket_num, (struct message_packet *)&ack_message);
     
}

void output_message (struct message_packet *message)
{    
    int i=0;
    int dest_handle_size = 0; //size of destinatino handle
    int s_handle_size_index = 0;
    size_t s_handle_size = 0;
    int s_handle_index = 0;
    int s_handle_end = 0;

    dest_handle_size = message->data[D_HANDLE_SIZE_INDEX];
    s_handle_size_index = 1 + dest_handle_size;
    s_handle_index = 2 + dest_handle_size;
    s_handle_size = message->data[s_handle_size_index];
    s_handle_end = s_handle_index + s_handle_size;
    printf("\n");
    for(i=s_handle_index;i<s_handle_end;i++)
    {
        printf("%c", message->data[i]);
    }
    printf(": ");
    while(message->data[i] != '\0')
    {
        printf("%c", message->data[i]);
        i++;
    }
    printf("\n");
    printf(">");
    fflush(stdout);
}

void resend_messages(int socket_num)
{
    int i=0;
    
    for(i=0;i<1024;i++)
    {
        if(watch_list[i] != 0)
        {
            //print_message(watch_list[i], SENDING);
            resend_message(watch_list[i], socket_num);
        }
    }
}

void resend_message(struct message_packet *message, int socket_num)
{
    c_send_message_packet(socket_num, message);
}

void add_to_watch_list(struct message_packet* message)
{
    int i, spot_found=0;
    if(num_entries == 1024)
    {
        printf("too many handles not being returned STOP WASTING INTERNTS!\n");
        exit(-1);
    }
    
    if(num_entries == 0)
    {
        watch_list[num_entries] = message;
        num_entries++;
    }
    else
    {
        for(i=0;i<num_entries;i++)
        {
            if(watch_list[i]==NULL)
            {
                watch_list[i] = message;
                spot_found = -1;
                num_entries++;
                break;
            }
        }
        if(spot_found==0)
        {
            watch_list[num_entries] = message;
            num_entries++;
        }
    }
}

void remove_from_watch_list(uint32_t sequence_number)
{
    int i=0;
    for(i=0;i<1024;i++)
    {
        if((watch_list[i] != 0) && (watch_list[i]->sequence_number == sequence_number))
        {
            free(watch_list[i]);
            watch_list[i] = NULL;
            //printf("No longer watching packet %d\n", message->sequence_number);
            num_entries--;
        }
    }
    
    if(num_entries < 0)
        printf("num_entries is less than zero.. shits bad\n");
}


int handle_stdin(struct message_packet *s_message, uint32_t sequence_number, uint8_t *sender_handle, int socket_num)
{
    int raw_data_len = 0;
    char raw_data[BUFFSIZE];  //data buffer
    while ((raw_data[raw_data_len] = getchar()) != '\n' && raw_data_len < BUFFSIZE)
        raw_data_len++;
    
    raw_data[raw_data_len] = '\0';
    
    if(raw_data[0] == '%' && raw_data[1] == 'M')
    {
        sequence_number++;
        send_message(s_message, raw_data, sequence_number, sender_handle, socket_num);
    }
    else if (raw_data[0] == '%' && raw_data[1] == 'L')
    {
        sequence_number++;
        send_normal_header_packet(s_message, sequence_number, FLAG_10, socket_num);
    }
    else if (raw_data[0] == '%' && raw_data[1] == 'E')
    {
        sequence_number++;
        send_normal_header_packet(s_message, sequence_number, FLAG_8, socket_num);
    }
    else
    {
        printf(">");
        fflush(stdout);
        return -1;
    }
    printf(">");
    fflush(stdout);
    return 1;
}

void send_normal_header_packet(struct message_packet *message, uint32_t sequence_number, uint8_t flag, int socket_num)
{
    //struct message_packet message;
    int sent = 0;
    u_short c_sum=0;
    
    message->sequence_number = sequence_number;
    message->check_sum = 0;
    message->flag = flag;
    
    c_sum = in_cksum((unsigned short*)message, sizeof(*message));
    message->check_sum = c_sum;
    
    //print_message(message, SENDING);
    c_send_message_packet(socket_num, message);  
}

void send_initial_packet(struct message_packet *message, uint32_t sequence_number, uint8_t *sender_handle, int socket_num)
{
    int i, j, handle_len = 0;
    u_short c_sum=0;
    
    message->sequence_number = sequence_number;
    message->flag = FLAG_1;
    message->check_sum = 0;
    handle_len = strlen(sender_handle);
    message->data[S_I_HANDLE_SIZE_INDEX] = handle_len;
    i=0;
    for(j=1;j<=handle_len;j++)
    {
        message->data[j] = sender_handle[i];
        i++;
    }
    
    c_sum = in_cksum((unsigned short*)message, sizeof(*message));
    message->check_sum = c_sum;
    
    
    //print_message(message, SENDING);
    
    
    
    c_send_message_packet(socket_num, message);
    

}

void send_message(struct message_packet *message, uint8_t *raw_data, uint32_t sequence_number, uint8_t *sender_handle, int socket_num)
{
    int i, j = 0;
    int dest_handle_size = 0; //size of destinatino handle
    int s_handle_size_index = 0;
    size_t s_handle_size = 0;
    int s_handle_index = 0;
    int s_handle_end = 0;
    int raw_data_index = 0;
    int text_length;
    u_short c_sum;
    
    message->sequence_number = sequence_number;
    message->check_sum = 0;
    message->flag = FLAG_6;
    
    i=3;
    j=1;
    while(raw_data[i] != ' ')
    {
        message->data[j] = raw_data[i];
        dest_handle_size++;
        j++;
        i++;
    }
    if(dest_handle_size > 100)
    {
        printf("A handle may not exceed 100 characters\n");
        exit(1);
    }
    raw_data_index = i + 1;
    message->data[D_HANDLE_SIZE_INDEX] = dest_handle_size;
    s_handle_size_index = 1 + dest_handle_size;
    s_handle_index = 2 + dest_handle_size;
    s_handle_size = strlen(sender_handle);
    s_handle_end = s_handle_index + s_handle_size;
    
    message->data[s_handle_size_index] = s_handle_size;
    i=0;
    for(j=s_handle_index;j<s_handle_end;j++)
    {
        message->data[j] = sender_handle[i];
        i++;
    }
    
    i=raw_data_index;
    j=s_handle_end;
    text_length = 0;
    while((j<BUFFSIZE) && (raw_data[i] != '\0') && (i < BUFFSIZE))
    {
        message->data[j] = raw_data[i];
        i++;
        j++;
        text_length++;
    }
    message->data[j] = '\0';
    
    if(text_length > 1000)
    {
        printf("Your message may not exceed 1000 characters\n");
        exit(1);
    }
    
    //print_message(message, SENDING);
    
    c_sum = in_cksum((unsigned short*)message, sizeof(*message));
    message->check_sum = c_sum;
    
    c_send_message_packet(socket_num, message);

}

void c_send_message_packet(int socket_num, struct message_packet *message)
{
    int sent = 0;
    
    //sent = send(socket_num, (char *)message, sizeof(*message), 0);
    sent = sendErr(socket_num, (char *)message, sizeof(*message), 0);
    if(sent < 0)
    {
        perror("send call");
        exit(-1);
    }

}

int tcp_send_setup(char *host_name, char *port)
{
    int socket_num;
    struct sockaddr_in remote;       // socket address for remote side
    struct hostent *hp;              // address of remote host

    // create the socket
    if ((socket_num = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
	    perror("socket call");
	    exit(-1);
	}


    // designate the addressing family
    remote.sin_family= AF_INET;

    // get the address of the remote host and store
    if ((hp = gethostbyname(host_name)) == NULL)
	{
	  printf("Error getting hostname: %s\n", host_name);
	  exit(-1);
	}

    memcpy((char*)&remote.sin_addr, (char*)hp->h_addr, hp->h_length);

    // get the port used on the remote side and store
    remote.sin_port= htons(atoi(port));

    if(connect(socket_num, (struct sockaddr*)&remote, sizeof(struct sockaddr_in)) < 0)
    {
	perror("connect call");
	exit(-1);
    }

    return socket_num;
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

