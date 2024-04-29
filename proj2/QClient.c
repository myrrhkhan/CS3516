#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <unistd.h>     /* for close() */
#define RCVBUFSIZE 1000 /* Size of receive buffer */

/* Error handling function */
void DieWithError(char *errorMessage) {
    fprintf(stderr, "%s\n", errorMessage);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sock;                          /* Socket descriptor */
    struct sockaddr_in echoServAddr;   /* Echo server address */
    unsigned short port;               /* Echo server port */
    char *ip;                          /* Server IP address (dotted quad) */
    char *qr_filename;                 /* String to send to echo server */
    char echoBuffer[RCVBUFSIZE];       /* Buffer for echo string */
    int bytesRcvd, totalBytesRcvd = 0; /* Bytes read in single recv()
    and total bytes read */

    /* Set things based on args */
    if (argc != 4) /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage: %s <Filename> <Server IP> [<Echo Port>]\n",
                argv[0]);
        exit(1);
    }
    ip = argv[2];          /* Second arg: server IP address (dotted quad) */
    qr_filename = argv[1]; /* First arg: string to echo */
    // save contents of qr_filename to qr_string
    FILE *qr_file = fopen(qr_filename, "r");
    if (qr_file == NULL) {
        DieWithError("Failed to open file");
    }
    char qr_string[RCVBUFSIZE];
    fgets(qr_string, RCVBUFSIZE, qr_file);
    fclose(qr_file);
    port = atoi(argv[3]); /* Use given port, if any */
    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr)); /* Zero out structure */
    echoServAddr.sin_family = AF_INET;            /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(ip); /* Server IP address */
    echoServAddr.sin_port = htons(port);          /* Server port */

    /* Establish the connection to the echo server */
    if (connect(sock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) <
        0)
        DieWithError("connect() failed");

    char len[10];
    snprintf(len, 10, "%ld", strlen(qr_string));
    printf("Sending over first message: %s\n", len);
    if (send(sock, len, sizeof(len), 0) != sizeof(len))
        DieWithError("send() could not send URL back to client");
    printf("Sending over second message: %s\n", qr_string);
    if (send(sock, qr_string, sizeof(qr_string), 0) != sizeof(qr_string))
        DieWithError("send() could not send URL back to client");

    // echoStringLen = strlen(echoString); /* Determine input length */
    //
    // /* Send the string to the server */
    // if (send(sock, echoString, echoStringLen, 0) != echoStringLen)
    //     DieWithError("send() could not successfully pass message.\nERR: sent
    //     a "
    //                  "different number of bytes than expected");
    //
    /* Receive the same string back from the server */
    // totalBytesRcvd = 0;
    printf("Received: "); /* Setup to print the echoed string */

    while (totalBytesRcvd < RCVBUFSIZE - 1) {
        /* Receive up to the buffer size (minus 1 to leave space for
        a null terminator) bytes from the sender */
        if ((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE - 1, 0)) <= 0)
            DieWithError("recv() failed or connection closed prematurely");
        totalBytesRcvd += bytesRcvd;  /* Keep tally of total bytes */
        echoBuffer[bytesRcvd] = '\0'; /* Terminate the string! */
        printf("%s\n", echoBuffer);   /* Print the echo buffer */
    }

    printf("\n"); /* Print a final linefeed */
    close(sock);
    exit(0);
}
