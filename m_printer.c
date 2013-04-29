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

#include "networks.h"

void print_message(struct message_packet *message, int direction)
{
    int i=0;
    int dest_handle_size = 0; //size of destinatino handle
    int s_handle_size_index = 0;
    size_t s_handle_size = 0;
    int s_handle_index = 0;
    int s_handle_end = 0;
    
    if(direction == SENDING)
    {
        printf("Sending...\n");
    }
    else
    {
        printf("Recieved:\n");
    }
    
    if(message->flag == FLAG_1)
    {
        printf("---C to S Initial message packet---\n");
        printf("sequence number    = %d\n", message->sequence_number);
        printf("checksum           = %d\n", message->check_sum);
        printf("flag               = %d\n", message->flag);
        printf("handle length      = %d\n", message->data[S_I_HANDLE_SIZE_INDEX]);
        printf("handle             = \"");
        s_handle_size = message->data[S_I_HANDLE_SIZE_INDEX];
        for(i=1;i<s_handle_size+1;i++)
        {
            printf("%c", message->data[i]);
        }
        printf("\"\n");
        printf("-----------------------------------\n\n");
        
    }
    else if(message->flag == FLAG_2)
    {
        printf("---S to C Good handle conformation---\n");
        printf("sequence number    = %d\n", message->sequence_number);
        printf("checksum           = %d\n", message->check_sum);
        printf("flag               = %d\n", message->flag);
        printf("-------------------------------------\n\n");
    }
    else if(message->flag == FLAG_3)
    {
        printf("---S to C Error on initial packet---\n");
        printf("sequence number    = %d\n", message->sequence_number);
        printf("checksum           = %d\n", message->check_sum);
        printf("flag               = %d\n", message->flag);
        printf("------------------------------------\n\n");
    }
    else if(message->flag == FLAG_6)
    {
        
        dest_handle_size = message->data[D_HANDLE_SIZE_INDEX];
        s_handle_size_index = 1 + dest_handle_size;
        s_handle_index = 2 + dest_handle_size;
        s_handle_size = message->data[s_handle_size_index];
        s_handle_end = s_handle_index + s_handle_size;
        
        printf("---\%M message packet---\n");
        printf("sequence number    = %d\n", message->sequence_number);
        printf("checksum           = %d\n", message->check_sum);
        printf("flag               = %d\n", message->flag);
        printf("d_handle length    = %d\n", message->data[D_HANDLE_SIZE_INDEX]);
        printf("d_handle           = \"");
        for(i=1;i<=dest_handle_size;i++)
        {
            printf("%c", message->data[i]);
        }
        printf("\"\n");
        printf("s_handle length    = %d\n", message->data[s_handle_size_index]);
        printf("s_handle           = \"");
        for(i=s_handle_index;i<s_handle_end;i++)
        {
            printf("%c", message->data[i]);
        }
        printf("\"\n");
        printf("message            = \"");
        while(message->data[i] != '\0')
        {
            printf("%c", message->data[i]);
            i++;
        }
        printf("\"\n");
        printf("------------------------\n\n");
    }
    else if(message->flag == FLAG_7)
    {
        printf("---Requested handle does not exist---\n");
        printf("sequence number    = %d\n", message->sequence_number);
        printf("checksum           = %d\n", message->check_sum);
        printf("flag               = %d\n", message->flag);
        printf("-------------------------------------\n");
    }
    else if(message->flag == FLAG_8)
    {
        printf("---C to S exit request---\n");
        printf("sequence number    = %d\n", message->sequence_number);
        printf("checksum           = %d\n", message->check_sum);
        printf("flag               = %d\n", message->flag);
        printf("-------------------------\n\n");
    }
    else if(message->flag == FLAG_9)
    {
        printf("---S to C exit ACK response---\n");
        printf("sequence number    = %d\n", message->sequence_number);
        printf("checksum           = %d\n", message->check_sum);
        printf("flag               = %d\n", message->flag);
        printf("------------------------------\n\n");
    }
    else if(message->flag == FLAG_10)
    {
        printf("---C to S number of handles requess---\n");
        printf("sequence number    = %d\n", message->sequence_number);
        printf("checksum           = %d\n", message->check_sum);
        printf("flag               = %d\n", message->flag);
        printf("--------------------------------------\n\n");
    }
    else if(message->flag == FLAG_11)
    {
        printf("---S to C number of handles response---\n");
        printf("sequence number    = %d\n", message->sequence_number);
        printf("checksum           = %d\n", message->check_sum);
        printf("flag               = %d\n", message->flag);
        printf("---------------------------------------\n\n");
    }
    else if(message->flag == FLAG_12)
    {
        printf("---C to S request handle #---\n");
        printf("sequence number    = %d\n", message->sequence_number);
        printf("checksum           = %d\n", message->check_sum);
        printf("flag               = %d\n", message->flag);
        printf("-----------------------------\n\n");
    }
    else if(message->flag == FLAG_13)
    {
        printf("---S to C handle # response---\n");
        printf("sequence number    = %d\n", message->sequence_number);
        printf("checksum           = %d\n", message->check_sum);
        printf("flag               = %d\n", message->flag);
        printf("------------------------------\n\n");
    }
    else if(message->flag == FLAG_255)
    {
        struct message_ack *ack;
        ack = (struct message_ack *)message;
        printf("---C to C ACK response---\n");
        printf("ack sequence number= %d\n", ack->ack_sequence_number);
        printf("sequence number    = %d\n", ack->sequence_number);
        printf("checksum           = %d\n", ack->check_sum);
        printf("flag               = %d\n", ack->flag);
        printf("-------------------------\n\n");
    }
   
    else
    {
        printf("there is no print code for the flag %d", message->flag);
    }
}
