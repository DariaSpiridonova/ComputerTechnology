#include "chat.h"

// Shared data for client threads
struct thread_args
{
    int msqid;                  // the queue number
    long my_id;
    char clean_name[NAME_LEN];
};

// Listener thread: receives incoming messages for my_id
void *listener_thread(void *arg)
{
    struct thread_args *data = (struct thread_args *)arg;
    struct msg_buf mybuf = {0};

    while (1)
    {
        if (msgrcv(data->msqid, (struct msgbuf *) &mybuf, PACKET_SIZE, data->my_id, 0) < 0)
        {
            printf("\nServer closed the queue. Listener exiting...\n");
            exit(0);
        }
        printf("\n[%s]: %s\n> ", mybuf.client_name, mybuf.message);
        fflush(stdout);
    }
    return NULL;
}

// Ping thread: confirms activity to server every 5 seconds (< 10s timeout)
void *ping_thread(void *arg)
{
    struct thread_args *data = (struct thread_args *)arg;
    struct msg_buf ping_pckg = {0};

    ping_pckg.mtype     = SERVER_TYPE;
    ping_pckg.action    = ACT_PING;
    ping_pckg.client_id = data->my_id;
    strncpy(ping_pckg.client_name, data->clean_name, NAME_LEN - 1);

    while (1)
    {
        sleep(5);
        if (msgsnd(data->msqid, (struct msgbuf *) &ping_pckg, PACKET_SIZE, 0) < 0)
        {
            break;
        }
    }
    return NULL;
}

// Read and validate client's name (#<Name>), returns 0 on "##" exit
int read_client_name(char *clean_name)
{
    char name[NAME_LEN] = {0};

    printf("Please, enter your client's name (#<Name>): ");
    while (1)
    {
        if (fgets(name, NAME_LEN, stdin) == NULL || name[0] != '#')
        {
            printf("Enter your name in the next format: #<Name>\n");
            continue;
        }
        else if (strcmp(name, "##\n") == 0)
        {
            printf("Goodbye, NoName!\n");
            return 0;
        }
        else
        {
            char *n_ptr = strchr(name, '\n');
            if (n_ptr != NULL) *n_ptr = '\0';

            if (strlen(name + 1) == 0)
            {
                printf("Name can't be empty. Try again:\n");
                continue;
            }

            strncpy(clean_name, name + 1, NAME_LEN - 1);
            return 1;
        }
    }
}

// Connect to server's message queue
int connect_to_queue(void)
{
    key_t key = ftok(PATH_TO_SERVER_NAME, 0);
    if (key < 0)
    {
        perror("ftok");
        exit(-1);
    }

    int msqid = msgget(key, 0666);
    if (msqid < 0)
    {
        printf("Can\'t get msqid (start server first!)\n");
        exit(-1);
    }

    return msqid;
}

// Perform handshake with arbiter server and return assigned client ID
long register_client(int msqid, const char *clean_name, long reply_type)
{
    struct msg_buf serv_pckg = {0};
    serv_pckg.mtype     = SERVER_TYPE;
    serv_pckg.action    = ACT_REGISTER;
    serv_pckg.client_id = reply_type;
    strncpy(serv_pckg.client_name, clean_name, NAME_LEN - 1);

    // Send hello message to the server
    if (msgsnd(msqid, (struct msgbuf *) &serv_pckg, PACKET_SIZE, 0) < 0)
    {
        printf("Can\'t send message to queue\n");
        exit(-1);
    }

    // Wait for handshake completion on our reply_type
    if (msgrcv(msqid, (struct msgbuf *) &serv_pckg, PACKET_SIZE, reply_type, 0) < 0)
    {
        printf("Can\'t receive message from queue\n");
        exit(-1);
    }

    if (serv_pckg.client_id <= 1)
    {
        printf("Registration failed: %s\n", serv_pckg.message);
        exit(-1);
    }

    return serv_pckg.client_id;
}

// Ask arbiter server for recipient's ID by name
long get_recipient_id(int msqid, long reply_type, const char *target_name)
{
    struct msg_buf id_req = {0};
    id_req.mtype     = SERVER_TYPE;
    id_req.action    = ACT_GET_ID;
    id_req.client_id = reply_type;
    strncpy(id_req.rcv_name, target_name, NAME_LEN - 1);

    if (msgsnd(msqid, (struct msgbuf *) &id_req, PACKET_SIZE, 0) < 0)
    {
        return -1;
    }

    struct msg_buf id_resp = {0};
    if (msgrcv(msqid, (struct msgbuf *) &id_resp, PACKET_SIZE, reply_type, 0) < 0)
    {
        return -1;
    }

    return id_resp.client_id;
}

// Parse !<Name>#<Message> and send message directly to recipient
void process_send_command(int msqid, long reply_type, const char *clean_name, char *input)
{
    char *sep = strchr(input + 1, '#');
    if (sep == NULL)
    {
        printf("Wrong format! Use: !<Name>#<Message>\n");
        return;
    }

    *sep = '\0';
    char *target_name = input + 1;
    char *msg_text    = sep + 1;

    if (strlen(target_name) == 0 || strlen(msg_text) == 0)
    {
        printf("Recipient name and message can't be empty!\n");
        return;
    }

    long target_id = get_recipient_id(msqid, reply_type, target_name);
    if (target_id < 0)
    {
        printf("Can\'t communicate with server\n");
        return;
    }
    if (target_id <= 1)
    {
        printf("User '%s' is offline or not found!\n", target_name);
        return;
    }

    // Send message directly to recipient's ID
    struct msg_buf direct_msg = {0};
    direct_msg.mtype  = target_id;
    direct_msg.action = ACT_SEND_MSG;
    strncpy(direct_msg.client_name, clean_name, NAME_LEN - 1);
    strncpy(direct_msg.message, msg_text, MESSAGE_LEN - 1);

    if (msgsnd(msqid, (struct msgbuf *) &direct_msg, PACKET_SIZE, 0) < 0)
    {
        printf("Can\'t send message to %s\n", target_name);
    }
}

// Notify server about client exit
void send_exit_notice(int msqid, long my_id, const char *clean_name)
{
    struct msg_buf exit_pckg = {0};
    exit_pckg.mtype     = SERVER_TYPE;
    exit_pckg.action    = ACT_EXIT;
    exit_pckg.client_id = my_id;
    strncpy(exit_pckg.client_name, clean_name, NAME_LEN - 1);

    msgsnd(msqid, (struct msgbuf *) &exit_pckg, PACKET_SIZE, 0);
}

// Main writer loop reading user commands
void run_writer_loop(int msqid, long my_id, long reply_type, const char *clean_name)
{
    char input[MESSAGE_LEN + NAME_LEN + 8] = {0};
    printf("Enter !<Name>#<Message> to send or ## to exit:\n");

    while (1)
    {
        printf("> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL || strcmp(input, "##\n") == 0)
        {
            send_exit_notice(msqid, my_id, clean_name);
            break;
        }

        char *n_ptr = strchr(input, '\n');
        if (n_ptr != NULL) *n_ptr = '\0';

        if (input[0] == '!')
        {
            process_send_command(msqid, reply_type, clean_name, input);
        }
        else
        {
            printf("Unknown command. Use !<Name>#<Message> or ##\n");
        }
    }
}

int main()
{
    char clean_name[NAME_LEN] = {0};

    // 1. Read client's name
    if (!read_client_name(clean_name))
    {
        return 0;
    }

    // 2. Connect to queue and register on server
    int msqid = connect_to_queue();
    long reply_type = (long)getpid() * 2 + 1;
    long my_id = register_client(msqid, clean_name, reply_type);

    printf("Registered successfully! Name: %s, My ID: %ld\n", clean_name, my_id);

    // 3. Start listener and ping threads
    struct thread_args t_args = {0};
    t_args.msqid = msqid;
    t_args.my_id = my_id;
    strncpy(t_args.clean_name, clean_name, NAME_LEN - 1);

    pthread_t tid_listener;
    pthread_t tid_ping;

    if (pthread_create(&tid_listener, NULL, listener_thread, &t_args) != 0)
    {
        perror("pthread_create (listener)");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&tid_ping, NULL, ping_thread, &t_args) != 0)
    {
        perror("pthread_create (ping)");
        pthread_cancel(tid_listener);
        pthread_join(tid_listener, NULL);
        exit(EXIT_FAILURE);
    }

    // 4. Run writer loop in main thread
    run_writer_loop(msqid, my_id, reply_type, clean_name);

    // 5. Stop background threads and exit
    printf("Stopping threads...\n");
    pthread_cancel(tid_listener);
    pthread_join(tid_listener, NULL);

    pthread_cancel(tid_ping);
    pthread_join(tid_ping, NULL);

    return 0;
}
