#include "chat.h"

struct client_data
{
    int active;
    long client_id;
    char client_name[NAME_LEN];
    time_t last_seen;
};

static int global_msqid = -1;

// Remove message queue on Ctrl+C
void handle_sigint(int sig)
{
    (void)sig;
    if (global_msqid >= 0)
    {
        msgctl(global_msqid, IPC_RMID, NULL);
        printf("\nServer stopped, message queue removed.\n");
    }
    exit(0);
}

// Create IPC message queue and return msqid
int create_message_queue(void)
{
    key_t key = ftok(PATH_TO_SERVER_NAME, 0);
    if (key < 0)
    {
        perror("ftok");
        exit(-1);
    }

    int msqid = msgget(key, 0666 | IPC_CREAT);
    if (msqid < 0)
    {
        printf("Can\'t get msqid\n");
        exit(-1);
    }

    return msqid;
}

// Find active client index by name, returns -1 if not found
int find_client_by_name(struct client_data *clients, const char *name)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].active && strcmp(clients[i].client_name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

// Find active client index by ID, returns -1 if not found
int find_client_by_id(struct client_data *clients, long client_id)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].active && clients[i].client_id == client_id)
        {
            return i;
        }
    }
    return -1;
}

// Find first inactive slot in clients array, returns -1 if full
int find_free_slot(struct client_data *clients)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (!clients[i].active)
        {
            return i;
        }
    }
    return -1;
}

// Handle new client registration request
void handle_register(int msqid, struct client_data *clients, long *next_client_id, const struct msg_buf *pckg)
{
    long reply_type = pckg->client_id;
    int name_exists = (find_client_by_name(clients, pckg->client_name) != -1);
    int free_idx    = find_free_slot(clients);

    struct msg_buf resp = {0};
    resp.mtype  = reply_type;
    resp.action = ACT_REGISTER;

    if (name_exists || free_idx == -1)
    {
        resp.client_id = 0;
        strcpy(resp.message, name_exists ? "Name already taken" : "Server is full");
    }
    else
    {
        clients[free_idx].active    = 1;
        clients[free_idx].client_id = *next_client_id;
        clients[free_idx].last_seen = time(NULL);
        strncpy(clients[free_idx].client_name, pckg->client_name, NAME_LEN - 1);

        resp.client_id = *next_client_id;
        strncpy(resp.client_name, pckg->client_name, NAME_LEN - 1);

        printf("[+] Registered '%s' with ID = %ld\n", clients[free_idx].client_name, *next_client_id);
        *next_client_id += 2;
    }

    msgsnd(msqid, (struct msgbuf *) &resp, PACKET_SIZE, 0);
}

// Handle recipient ID lookup request
void handle_get_id(int msqid, struct client_data *clients, const struct msg_buf *pckg)
{
    long reply_type = pckg->client_id;
    int idx = find_client_by_name(clients, pckg->rcv_name);

    struct msg_buf resp = {0};
    resp.mtype     = reply_type;
    resp.action    = ACT_GET_ID;
    resp.client_id = (idx != -1) ? clients[idx].client_id : 0;

    msgsnd(msqid, (struct msgbuf *) &resp, PACKET_SIZE, 0);
}

// Update client's last activity timestamp
void handle_ping(struct client_data *clients, const struct msg_buf *pckg)
{
    int idx = find_client_by_id(clients, pckg->client_id);
    if (idx != -1)
    {
        clients[idx].last_seen = time(NULL);
    }
}

// Remove client on explicit exit command
void handle_exit(struct client_data *clients, const struct msg_buf *pckg)
{
    int idx = find_client_by_id(clients, pckg->client_id);
    if (idx != -1)
    {
        printf("[-] Client '%s' (ID = %ld) exited.\n", clients[idx].client_name, clients[idx].client_id);
        clients[idx].active = 0;
    }
}

// Remove clients inactive for more than 10 seconds
void check_inactive_clients(struct client_data *clients)
{
    time_t now = time(NULL);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].active && (now - clients[i].last_seen > 10))
        {
            printf("[!] Client '%s' (ID = %ld) timed out (>10s) and was removed.\n",
                   clients[i].client_name, clients[i].client_id);
            clients[i].active = 0;
        }
    }
}

int main()
{
    signal(SIGINT, handle_sigint);

    global_msqid = create_message_queue();

    struct client_data clients[MAX_CLIENTS] = {0};
    // Even IDs (2, 4, 6...) never collide with odd reply_type values
    long next_client_id = 2;

    printf("Arbiter server started (msqid = %d). Press Ctrl+C to stop.\n", global_msqid);

    struct msg_buf pckg = {0};

    while (1)
    {
        // Non-blocking receive to check 10s timeouts in parallel
        ssize_t len = msgrcv(global_msqid, (struct msgbuf *) &pckg, PACKET_SIZE, SERVER_TYPE, IPC_NOWAIT);

        if (len >= 0)
        {
            switch (pckg.action)
            {
                case ACT_REGISTER:
                    handle_register(global_msqid, clients, &next_client_id, &pckg);
                    break;
                case ACT_GET_ID:
                    handle_get_id(global_msqid, clients, &pckg);
                    break;
                case ACT_PING:
                    handle_ping(clients, &pckg);
                    break;
                case ACT_EXIT:
                    handle_exit(clients, &pckg);
                    break;
                default:
                    break;
            }
        }

        check_inactive_clients(clients);

        // Sleep 0.1s to avoid 100% CPU usage
        usleep(100000);
    }

    return 0;
}
