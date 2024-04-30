#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <unistd.h>     /* for close() */
#define RCVBUFSIZE 1024 /* Size of receive buffer */

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
    FILE *qr_file = fopen(qr_filename, "rb");
    if (qr_file == NULL) {
        DieWithError("Failed to open file");
    }
    // printf("here\n");

    // get fstat to get file size and allocate char array
    struct stat st;
    stat(qr_filename, &st);
    int qr_len = st.st_size;
    char qr_string[qr_len + 1];
    fread(qr_string, 1, st.st_size, qr_file);
    qr_string[qr_len] = '\0';
    fclose(qr_file);

    // for (int i = 0; i < qr_len; i++) {
    // printf("%d ", qr_string[i]);
    // }
    // printf("\n");

    // printf("QR string:\n%s\nend\n", qr_string);

    // Set port
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
    snprintf(len, 10, "%d", qr_len);
    // printf("Sending over first message: %s\n", len);
    printf("Sending in\n");
    for (int i = 3; i > 0; i--) {
        printf("%d\n", i);
        sleep(1);
    }
    printf("Now.\n");
    if (send(sock, len, strlen(len), 0) != strlen(len))
        DieWithError("send() could not send URL back to client");
    // printf("Sending over second message: %s\n", qr_string);
    // printf("Length of qr_string: %d\n", qr_len);
    if (send(sock, qr_string, qr_len, 0) != qr_len)
        DieWithError("send() could not send URL back to client");

    // receive status code
    char status[1];
    if ((bytesRcvd = recv(sock, status, sizeof(status), 0)) <= 0)
        DieWithError("recv() failed or connection closed prematurely");
    int status_code = atoi(status);
    printf("Received status code: %d\n", status_code);
    printf("Receiving message in 3 seconds\n");
    // receive length of error message
    char len_error[10];
    if ((bytesRcvd = recv(sock, len_error, 10, 0)) <= 0)
        DieWithError("recv() failed or connection closed prematurely");
    // receive error message
    char error[atoi(len_error)];
    if ((bytesRcvd = recv(sock, error, atoi(len_error), 0)) <= 0)
        DieWithError("recv() failed or connection closed prematurely");
    printf("Received message:\n%s\n", error);

    // printf("\n"); /* Print a final linefeed */
    close(sock);
    exit(0);
}
