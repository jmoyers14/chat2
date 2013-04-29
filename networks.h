// for the server side
#define FLAG_1 1
#define FLAG_2 2
#define FLAG_3 3
#define FLAG_4 4
#define FLAG_5 5
#define FLAG_6 6
#define FLAG_7 7
#define FLAG_8 8
#define FLAG_9 9
#define FLAG_10 10
#define FLAG_11 11
#define FLAG_12 12
#define FLAG_13 13
#define FLAG_255 255
#define D_HANDLE_SIZE_INDEX 0
#define S_I_HANDLE_SIZE_INDEX 0
#define MAX_HANDLE_SIZE 100
#define MAX_HANDLE_ENTRY 1024
#define BUFFSIZE 2048
#define SENDING 0
#define RECIEVED 1


struct message_handle_count
{
    uint32_t sequence_number;
    uint16_t check_sum;
    uint8_t flag;
    uint32_t handle_count;
    uint8_t data[BUFFSIZE-4];
}__attribute__((packed));

struct message_packet
{
	uint32_t sequence_number;
	uint16_t check_sum;
	uint8_t flag;
	uint8_t data[BUFFSIZE];
}__attribute__((packed));

struct message_ack
{
    uint32_t sequence_number;
    uint16_t check_sum;
    uint8_t flag;
    uint32_t ack_sequence_number;
    uint8_t data[BUFFSIZE-4];
}__attribute__((packed));

struct handle_entry
{
    int handle_in_use;
    uint8_t handle[MAX_HANDLE_SIZE];
    uint32_t sequence_number;
    int socket_num;
}__attribute__((packed));

// for server side
int tcp_recv_setup();
int tcp_listen(int server_socket, int back_log);
void set_non_blocking(int welcome_socket);
void build_select_list();
void read_sockets();
void handle_data(int i);
void handle_new_connection();
void print_handle_entry(struct handle_entry entry, int index);
int AddToArray(struct handle_entry entry);
int is_valid_handle(char *entry);
void s_send_normal_header_packet(uint32_t sequence_number, uint8_t flag, int socket_num);
int fill_handle_entry(struct message_packet *message, struct handle_entry *);
int handle_message(struct message_packet *message, int sender_socket);
int handle_ack_message(struct message_ack *ack);
void send_message_packet(int socket_num, struct message_packet message);
void remove_from_array(struct handle_entry entry);
void s_send_number_of_handles(uint32_t sequence_number, int socket_num);
void handle_request(uint32_t requestNum, uint32_t sequence_number, int socket_num);

// for the client side
int tcp_send_setup(char *host_name, char *port);

void send_message(struct message_packet *, uint8_t *data, uint32_t seq_number, uint8_t* sender_handle, int socket_num);
void send_initial_packet(struct message_packet *, uint32_t sequence_number, uint8_t *sender_handle, int socket_num);
void send_normal_header_packet(struct message_packet *, uint32_t sequence_number, uint8_t flag, int socket_num);
void initial_state(uint32_t sequence_number, uint8_t *sender_handle, int socket_num);
int handle_stdin(struct message_packet *, uint32_t sequence_number, uint8_t *sender_handle, int socket_num);
void resend_message(struct message_packet *message, int socket_num);
void resend_messages(int socket_num);
void add_to_watch_list(struct message_packet* message);
void remove_from_watch_list(uint32_t sequence_number);
void output_message (struct message_packet *message);
void c_send_message_packet(int socket_num, struct message_packet *message);
void send_ack_message(uint32_t sequence_number, uint32_t ack_sequence_number, struct message_packet message, int socket_num);
void print_handle_error(struct message_packet message);
void print_handle(struct message_packet message);
void kill_self();
//both
void print_message(struct message_packet *message, int direction);
unsigned short in_cksum(unsigned short *addr,int len);
void request_handle_number(uint32_t p, struct message_packet *message, uint32_t sequence_number, int socket_num);

