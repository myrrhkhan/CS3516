#include <arpa/inet.h> /* for sockaddr_in and inet_ntoa() */
#include <pthread.h>
#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>  /* for close() */
#define MAXPENDING 5 /* Maximum outstanding connection requests */
#define RCVBUFSIZE 32
#define TIMEOUT 10
#define MAXSIZE 250000 /* Maximum size of image */

int num_imgs = 0;

void AcceptClient() { struct sockaddr_in ClientAddrs[MAXPENDING]; }

void DieWithError(char *errorMessage) {
    fprintf(stderr, "%s\n", errorMessage);
    exit(-1);
}

void SendError(int clntSocket, int status_code) {
    char status[10];
    char *err;
    int err_len;
    char err_len_str[10];

    printf("Sending error\n");
    snprintf(status, 10, "%d", status_code);
    if (send(clntSocket, status, strlen(status), 0) != strlen(status)) {
        DieWithError("send() could not send status code back to client");
    }

    if (status_code == 1) {
        err = "No barcode found";
    } else if (status_code == 2) {
        err = "Timeout";
    } else if (status_code == 3) {
        err = "Rate limit exceeded, image too large.";
    }

    err_len = strlen(err);
    snprintf(err_len_str, 10, "%d", err_len);
    // send length of error
    if (send(clntSocket, err_len_str, strlen(err_len_str), 0) !=
        strlen(err_len_str)) {
        DieWithError("send() could not send error length back to client");
    }
    // send error
    if (send(clntSocket, err, err_len, 0) != err_len) {
        DieWithError("send() could not send error back to client");
    }

    close(clntSocket);
    return;
}

void HandleTCPClient(int clntSocket) {
    // current time
    time_t now;
    time(&now);
    // variables
    unsigned int len = 0;

    char buffer[RCVBUFSIZE]; /* Buffer for echo string */

    int recvMsgSize;
    int num_received = 0;

    int status_code = 0;

    // if capture fail
    if ((recvMsgSize = recv(clntSocket, buffer, RCVBUFSIZE, 0)) < 0) {
        DieWithError("recv() failed");
    }

    buffer[recvMsgSize] = '\0'; /* Terminate the string! */
    len = atoi(buffer);
    if (len > MAXSIZE) {
        status_code = 3;
        return SendError(clntSocket, status_code);
    }

    // File to save image
    char name[10];
    snprintf(name, 10, "%d.png", num_imgs);
    num_imgs++;
    FILE *imgfile = fopen(name, "wb");
    if (imgfile == NULL) {
        DieWithError("Failed to open file");
    }

    while (num_received < len) {

        if ((recvMsgSize = recv(clntSocket, buffer, RCVBUFSIZE, 0)) < 0) {
            DieWithError("recv() failed");
            // } else if (recvMsgSize + num_received > len) {
            //     printf("Overflow checking\n");
            //     recvMsgSize = len - num_received;
        }
        printf("Received: %d\n", recvMsgSize);
        printf("Total received: %d\n", num_received);
        printf("Total length: %d\n", len);
        num_received += recvMsgSize;
        if (fwrite(buffer, 1, recvMsgSize, imgfile) != recvMsgSize) {
            DieWithError("Failed to write image data to file");
        }
    }

    fclose(imgfile);

    // fstat
    struct stat st;
    stat(name, &st);
    len = st.st_size;
    printf("File size: %d\n", len);

    // find QR code with Java and popen, save result
    char command[210];
    snprintf(command, 210,
             "java -cp javase.jar:core.jar "
             "com.google.zxing.client.j2se.CommandLineRunner %s",
             name);
    FILE *fp = popen(command, "r");
    printf("Attempted to run Java command...\n");
    char url[250];
    char all[1000];
    if (fp == NULL) {
        DieWithError("Failed to run Java command");
    }

    // record output
    /* Output is as follows
     * Raw result:
     * [URL]
     * Parsed result:
     * [URL]
     * As such, we need to detect when parsed result is reached and save the url
     * for after
     */
    int isnext = 0;
    while (fgets(all, sizeof(url), fp) != NULL) {
        printf("%s", all);
        if (strstr(all, "Parsed result") != NULL) {
            isnext = 1;
        } else if (isnext) {
            strcpy(url, all);
            break;
        } else if (strstr(all, "No barcode found") != NULL) {
            strcpy(url, "No barcode found");
            status_code = 1;
            return SendError(clntSocket, status_code);
        }
    }
    printf("URL: %s\n", url);
    // close file and remove image
    pclose(fp);
    if (remove(name) != 0) {
        DieWithError("Failed to remove image file");
    }
    // send status code
    char status[10];
    snprintf(status, 10, "%d", status_code);
    if (send(clntSocket, status, strlen(status), 0) != strlen(status)) {
        DieWithError("send() could not send status code back to client");
    }
    // send result back to client
    if (send(clntSocket, url, strlen(url), 0) != strlen(url)) {
        DieWithError("send() could not send URL back to client");
    }
    // reset length
    len = 0;

    close(clntSocket); /* Close client socket */
}

int main(int argc, char *argv[]) {
    int servSock; /* Socket descriptor for server */
    int clntSock; /* Socket descriptor for client */

    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */

    unsigned short echoServPort; /* Server port */
    unsigned int clntLen;        /* Length of client address data structure */

    if (argc == 1) {
        echoServPort = 2012;
    } else {
        echoServPort = atoi(argv[1]); /* First arg: local port */
    }

    /* Create socket for incoming connections */
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr)); /* Zero out structure */

    echoServAddr.sin_family = AF_INET; /* Internet address family */
    echoServAddr.sin_addr.s_addr =
        htonl(INADDR_ANY);                       /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort); /* Local port */

    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) <
        0)
        DieWithError("bind() failed");

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    // if (setsockopt(servSock, SOL_SOCKET, SO_RCVTIMEO, &timeout,
    //                sizeof(timeout)) < 0)
    //     DieWithError("setsockopt() failed");

    /* Mark the socket so it will listen for incoming connections */
    if (listen(servSock, MAXPENDING) < 0)
        DieWithError("listen() failed");

    printf("Server is running on port %d\n", echoServPort);
    for (;;) /* Run forever */
    {

        // // threads for each client
        // pthread_t thread_ids[MAXPENDING];
        // for (int i = 0; i < MAXPENDING; i++) {
        //     pthread_create(&thread_ids[i], NULL, HandleTCPClient, clntSock);
        // }

        /* Set the size of the in-out parameter */
        clntLen = sizeof(echoClntAddr);
        /* Wait for a client to connect */
        if ((clntSock = accept(servSock, (struct sockaddr *)&echoClntAddr,
                               &clntLen)) < 0)
            DieWithError("accept() failed");
        /* clntSock is connected to a client! */
        printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));
        HandleTCPClient(clntSock);
    }
    /* NOT REACHED */
}
