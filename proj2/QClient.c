#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for exit() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#define RCVBUFSIZE 32   /* Size of receive buffer */

void DieWithError(char *errorMessage); /* Error handling function */

void HandleTCPClient(int clntSocket) {}

void DieWithError(char *errorMessage) {
    fprintf(stderr, "%s\n", errorMessage);
    exit(-1);
}
