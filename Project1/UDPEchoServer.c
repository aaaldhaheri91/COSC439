#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <time.h>       /* srand(), rand() */
#include "StructInfo.h"

#define ECHOMAX 140     /* Longest string to echo */
#define TRUE 1
#define FALSE 0

/* client message struct */
typedef struct {

    enum {
        FirstLogin, Login, Follow, Post, Search,
        Receive, Delete, Unfollow, Logout, LoggedIn
    } request_type;                     /* same size as unsigned int */

    unsigned int rquest_id;                      /* request client sends */

    unsigned int UserID;                /* unique client identifier */

    unsigned int LeaderID;              /* unique client indentifiere */

    char message[140];

} ClientMessage;

typedef struct{

    unsigned int LeaderID ;  /* unique client identifier */

    /* store users following, by default its all zeros*/
    int following[10];

    unsigned int UserID;    /* unique user id */

    char message[140];    /* text message */

}ServerMessage;


void DieWithError(char *errorMessage);  /* External error handling function */


void check_options(ServerMessage*, ClientMessage*);
void login(char*, unsigned int);
int generate_id();
int update_followers(int *, int, unsigned int);
void post(char*, int);



int main(int argc, char *argv[])
{

    int sock;                        /* Socket */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int cliAddrLen;         /* Length of incoming message */
    char echoBuffer[ECHOMAX];        /* Buffer for echo string */
    unsigned short echoServPort;     /* Server port */

    ServerMessage send_message;     /* struct for sending */
    ClientMessage recieve_message;  /* struct for recieveing */
	
	int i;
	
    /* initialize following array to default values*/
    for(i = 0; i < 10; i++){
        send_message.following[i] = 0;
    }

    if (argc != 2)         /* Test for correct number of parameters */
    {
        fprintf(stderr,"Usage:  %s <UDP SERVER PORT>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);  /* First arg:  local port */

    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");

    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        cliAddrLen = sizeof(echoClntAddr);

        /* Block until receive message from a client */
        if ((recvfrom(sock, (ClientMessage *) &recieve_message, sizeof(recieve_message), 0,
            (struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0)
            DieWithError("Server: recvfrom() failed");

        printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));
        check_options(&send_message, &recieve_message);

        /* Send received datagram back to the client */
        if (sendto(sock, (ServerMessage *) &send_message, sizeof(send_message), 0,
             (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != sizeof(send_message))
            DieWithError("SERVER: sendto() sent a different number of bytes than expected");
    }
    /* NOT REACHED */
}

//////////////////////////////////////////////////////////////////////////////////////////

/**
 * checks what option client sent
 * @param *send_message pointer to struct that will be sent
 * @param *recieve_message pointer to struct recieved from client
 */
void check_options(ServerMessage *send_message, ClientMessage *recieve_message){

    if(recieve_message->request_type == Login){                          /* if login */
        login(send_message->message, recieve_message->UserID);
        if(strcmp(send_message->message, "Login success") == 0){
            send_message->UserID = recieve_message->UserID;
        }
    } else if(recieve_message->request_type == FirstLogin){               /* if first login */
        send_message->UserID = generate_id();
        strcpy(send_message->message, "Login success");

    } else if(recieve_message->request_type == Follow){                    /* if follow */

        int size = sizeof(send_message->following) / sizeof(int);
         if(update_followers(send_message->following, size,
                            recieve_message->LeaderID))
             strcpy(send_message->message, "Follow success");
        else
            strcpy(send_message->message, "Follow failed");

    } else if(recieve_message->request_type == LoggedIn){
        strcpy(send_message->message, "You are Logged in already!");
    } else if(recieve_message->request_type == Post){
		strcpy(send_message->message, recieve_message->message);
		strcpy(&posted_messages[recieve_message->UserID][posted_index], recieve_message->message);
		printf("posted messages: %s\n", &posted_messages[recieve_message->UserID][posted_index]);
		posted_index;
	}


}

//////////////////////////////////////////////////////////////////////////////////////////

/**
 * this function checks user id entered,
 * if it exists, if not it will not allow user
 * to login
 * @param id the user id entered
 * @return whether login was success or fail
 */
void login(char *word, unsigned int id){


    int i;
    int size = sizeof(user_ids) / sizeof(int);
    for(i = 0; i < size; i++){

        if(id == user_ids[i]){
            strcpy(word, "Login success");
            int length = strlen(word);
            word[length] = '\0';
            return;
        }
    }

    //if id does not exist, fail
    strcpy(word, "Login fail");
    int length = strlen(word);
    word[length] = '\0';

}

//////////////////////////////////////////////////////////////////////////////////////////

/**
 * this functions generates a unique random id for user
 * @return id the user generated id
 */
int generate_id(){

    srand(time(NULL));  // randomize seed
    int id = rand() % 10;

    //size of array
    int size_user_ids = sizeof(user_ids) / sizeof(int);
    int size_sample_leaders = sizeof(sample_leaders_ids) / sizeof(int);

    if(exist(user_ids, size_user_ids, id))
        generate_id();

    //store id in users array
    user_ids[append_users_index] = id;
    logged_in_users[append_logged_in_index] = id;
    append_logged_in_index++;
    append_users_index++;
    return id;

}

//////////////////////////////////////////////////////////////////////////////////////////

/**
 * this updates the leaders list of a client
 * @param array the exisiting list of leaders client is following
 * @param size the size of the array
 * @param leader_id the leader client wants to follow
 */
int update_followers(int *array, int size, unsigned int leader_id){

    int i;
    int size_user_ids = sizeof(user_ids) / sizeof(int);
    int size_sample_leaders = sizeof(sample_leaders_ids) / sizeof(int);

    //check if user exist in sample_leaders_ids array
    if(!exist(user_ids, size_user_ids, leader_id))
              return FALSE;

    //check if user already in list
    if(exist(array, 10, leader_id))
        return FALSE;

    //else add
    for(i = 0; i < 10; i++){
        if(array[i] == 0){
            array[i] = leader_id;
            break;
        }
    }
    return TRUE;


}

//////////////////////////////////////////////////////////////////////////////////////////

/**
 * this function looks if a target exist in array
 * @param array the array
 * @param size the size of array
 * @param target the item to search for
 * @return TRUE or FALSE
 */
int exist(int *array, int size, int target){

    int i;
    for(i = 0; i < size; i++){
        if(array[i] == target)
            return TRUE;
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////


