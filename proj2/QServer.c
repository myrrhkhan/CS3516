#include <arpa/inet.h> /* for sockaddr_in and inet_ntoa() */
#include <getopt.h>    /* for getopt() */
#include <getopt.h>
#include <netdb.h>      /* for getaddrinfo() */
#include <netinet/in.h> /* for struct addrinfo */
#include <poll.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <sys/stat.h>
#include <time.h>
#include <unistd.h> /* for close() */
#define RCVBUFSIZE 32
#define TIMEOUT 10
#define MAXSIZE 250000 /* Maximum size of image */

// https://www.delftstack.com/howto/c/try-catch-c/
#define TRY                                                                    \
    do {                                                                       \
        jmp_buf buf_state;                                                     \
        if (!setjmp(buf_state)) {
#define CATCH                                                                  \
    }                                                                          \
    else {
#define ENDTRY                                                                 \
    }                                                                          \
    }                                                                          \
    while (0)
#define THROW longjmp(buf_state, 1)

/*
 * CMD Args:
 * port: port to run server on
 * rate: rate limit for server in times per minute
 * max: maximum number of connections
 * timeout: timeout for server in seconds
 * */
int port = 2012;
int rate = 3;
int max = 3;
int timeout = 80;

void delay_send(void) {
    printf("Sending in\n");
    for (int i = 3; i > 0; i--) {
        printf("%d\n", i);
        sleep(1);
    }
    printf("Now.\n");
}

void write_log(char *ip, char *type, char *violations, char *url) {
    // get current time, convert to "YYYY-MM-DD HH:MM:SS"
    time_t t = time(NULL);
    char time[20];
    strftime(time, 20, "%Y-%m-%d %H:%M:%S", localtime(&t));

    /**
     * Log columns (CSV)
     * Time, IP, Type, Violations, URL
     */
    FILE *log = fopen("log.csv", "a");
    fprintf(log, "%s, %s, %s, %s, %s\n", time, ip, type, violations, url);
    fclose(log);
}

void DieWithError(char *errorMessage) {
    fprintf(stderr, "%s\n", errorMessage);
    write_log("", "Error", errorMessage, "");
    exit(-1);
}

void SendError(int clntSocket, int status_code, char *ip) {
    char status[10];
    char *err = "";
    int err_len;
    char err_len_str[10];

    printf("Sending error\n");
    snprintf(status, 10, "%d", status_code);
    write_log(ip, "Sending status code", "", "");
    if (send(clntSocket, status, strlen(status), 0) != strlen(status)) {
        DieWithError("send() could not send status code back to client");
    }

    if (status_code == 1) {
        err = "No barcode found";
    } else if (status_code == 2) {
        err = "Timeout. Either took too long to respond, or server is too "
              "busy.";
    } else if (status_code == 3) {
        err = "Rate limit exceeded, image too large.";
    }

    delay_send();

    err_len = strlen(err);
    snprintf(err_len_str, 10, "%d", err_len);
    // send length of error
    write_log(ip, "Sending error length", "", "");
    if (send(clntSocket, err_len_str, strlen(err_len_str), 0) !=
        strlen(err_len_str)) {
        DieWithError("send() could not send error length back to client");
    }

    delay_send();

    // send error
    write_log(ip, "Sending error", "", "");
    write_log(ip, "Error", err, "");
    if (send(clntSocket, err, err_len, 0) != err_len) {
        DieWithError("send() could not send error back to client");
    }

    printf("Error sent\n");

    close(clntSocket);
}

int get_listener(int port) {
    // from beej's guide
    int listener;
    int yes = 1;
    int rv;

    struct addrinfo hints, *ai, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    char portstr[10];
    snprintf(portstr, 10, "%d", port);
    if ((rv = getaddrinfo(NULL, portstr, &hints, &ai)) != 0) {
        DieWithError("Couldn't get listener");
    }

    for (p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }

        // Lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    freeaddrinfo(ai); // All done with this

    // If we got here, it means we didn't get bound
    if (p == NULL) {
        return -1;
    }

    // Listen
    if (listen(listener, 10) == -1) {
        return -1;
    }

    return listener;
}

// Add a new file descriptor to the set
void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count,
                 int *fd_size) {
    // If we don't have room, add more space in the pfds array
    if (*fd_count == *fd_size) {
        // send timeout error to listener
        SendError(newfd, 2, "");
        return;
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

    (*fd_count)++;
}

// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count) {
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count - 1];

    (*fd_count)--;
}

void analyze_send_url(int clntSocket, char *name, char *ip) {
    int status_code = 0;
    // fstat
    struct stat st;
    stat(name, &st);
    int len = st.st_size;
    printf("File size: %d\n", len);
    printf("analyzing image %s\n", name);

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
     * As such, we need to detect when parsed result is reached and save the
     * url for after
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
            SendError(clntSocket, status_code, ip);
            return;
        }
    }
    printf("URL: %s\n", url);
    // close file and remove image
    pclose(fp);
    if (remove(name) != 0) {
        DieWithError("Failed to remove image file");
    }
    // send status code
    char *status = "0";
    write_log(ip, "Sending status code", "", "");
    if (send(clntSocket, status, strlen(status), 0) != strlen(status)) {
        DieWithError("send() could not send status code back to client");
    }
    // send length of URL
    // edge case
    if (url[strlen(url) - 1] == '\n') {
        url[strlen(url) - 1] = '\0';
    }
    char url_len[10];
    snprintf(url_len, 10, "%lu", strlen(url));
    printf("URL length: %s\n", url_len);
    write_log(ip, "Sending URL length", "", "");
    if (send(clntSocket, url_len, strlen(url_len), 0) != strlen(url_len)) {
        DieWithError("send() could not send URL length back to client");
    }
    // send result back to client, wait 3 seconds before sending to prevent
    // data loss

    delay_send();

    write_log(ip, "Sending URL", "", url);
    if (send(clntSocket, url, strlen(url), 0) != strlen(url)) {
        DieWithError("send() could not send URL back to client");
    }
}

void HandleTCPClient(int clntSocket, int thread_num, char *ip) {
    // variables
    unsigned int len = 0;

    char buffer[RCVBUFSIZE]; /* Buffer for echo string */

    int recvMsgSize;
    int num_received = 0;

    int status_code = 0;

    // if capture fail, else receive image length
    if ((recvMsgSize = recv(clntSocket, buffer, RCVBUFSIZE, 0)) < 0) {
        printf("recv() failed or connection closed prematurely\n");
        // pthread_exit((void *)(intptr_t)-1);
    }

    write_log(ip, "Image length received", "", "");

    buffer[recvMsgSize] = '\0'; /* Terminate the string! */
    len = atoi(buffer);
    if (len > MAXSIZE) {
        status_code = 3;
        write_log(ip, "Image too large", "RATE", "");
        SendError(clntSocket, status_code, ip);
        return;
    }

    char *imgbuf = calloc(len, sizeof(char));

    // File to save image
    char name[10];
    snprintf(name, 10, "%d.png", thread_num);
    FILE *imgfile = fopen(name, "wb");
    if (imgfile == NULL) {
        DieWithError("Failed to open file");
    }

    // save image
    while (num_received < len) {
        write_log(ip, "Image data received", "", "");
        int remaining_bytes = len - num_received;
        if (remaining_bytes == 28) {
            // special case, always happens, send error message
            // status_code = 2;
            // SendError(clntSocket, status_code);
            // close(clntSocket);
            free(imgbuf);
            fclose(imgfile);
            SendError(clntSocket, 2, ip);
            return;
        }
        int bytes_received =
            recv(clntSocket, imgbuf + num_received, remaining_bytes, 0);
        // for (int i = 0; i < len; i++) {
        //     printf("%d ", imgbuf[i]);
        // }
        if (bytes_received <= 0) {
            free(imgbuf);
            fclose(imgfile);
            printf("recv() failed or connection closed prematurely");
            write_log(ip, "Image data receive failed", "", "");
            write_log(ip, "Connection closed", "", "");
            return;
        }
        num_received += bytes_received;
    }

    // Write the received image data to the file
    if (fwrite(imgbuf, 1, len, imgfile) != len) {
        free(imgbuf);
        DieWithError("Failed to write image data to file");
    }
    free(imgbuf);
    fclose(imgfile);

    write_log(ip, "Image fully received", "", "");

    // analyze image
    analyze_send_url(clntSocket, name, ip);

    TRY {
        close(clntSocket);
        write_log(ip, "Connection closed", "", "");
    }
    CATCH {
        printf("Couldn't close socket, one of the weird 28 ones.\n");
        // pthread_exit((void *)(intptr_t)-1);
    }
    ENDTRY;
}

void process_args(int argc, char **argv) {

    const char *const short_options = "p:r:m:t:";

    const struct option long_options[] = {

        {"port", required_argument, NULL, 'p'},
        {"rate", required_argument, NULL, 'r'},
        {"max", required_argument, NULL, 'm'},
        {"timeout", required_argument, NULL, 't'},

    };

    while (1) {
        const int opt = getopt_long(argc, argv, short_options, long_options,
                                    NULL); // get the next option

        if (opt == -1) {
            break;
        }

        switch (opt) {
        case 'p':
            printf("Port: %s\n", optarg);
            port = atoi(optarg);
            break;
        case 'r':
            printf("Rate: %s\n", optarg);
            rate = atoi(optarg);
            break;
        case 'm':
            printf("Max: %s\n", optarg);
            max = atoi(optarg);
            break;
        case 't':
            printf("Timeout: %s\n", optarg);
            timeout = atoi(optarg);
            break;
        default:
            break;
        }
    }
}

int main(int argc, char *argv[]) {

    // print args
    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");

    process_args(argc, argv);

    write_log("", "Server startup attempt", "", "");

    int listener = get_listener(port);
    if (listener == -1) {
        DieWithError("Failed to bind listener");
    }

    struct pollfd *pfds = malloc(sizeof(*pfds) * max);
    pfds[0].fd = listener;
    pfds[0].events = POLLIN;

    int fd_count = 1;

    printf("Server running on port %d\n", port);
    write_log("", "Server startup", "", "");

    for (;;) {
        int poll_count = poll(pfds, fd_count, timeout * 1000);

        if (poll_count == -1) {
            DieWithError("poll() failed");
        }

        for (int i = 0; i < fd_count; i++) {
            if (pfds[i].revents & POLLIN) {
                // new connection
                socklen_t addrlen = sizeof(struct sockaddr_storage);
                struct sockaddr_storage client_addr;
                addrlen = sizeof(client_addr);

                // have newfd accept socket
                int newfd = 0;
                if ((newfd = accept(listener, (struct sockaddr *)&client_addr,
                                    &addrlen)) == -1) {
                    DieWithError("accept() failed");
                }

                // get IP
                char ip[INET_ADDRSTRLEN];
                struct sockaddr_in *sin = (struct sockaddr_in *)&client_addr;
                inet_ntop(AF_INET, &sin->sin_addr, ip, sizeof(ip));
                write_log(ip, "New connection", "", "");

                if (fd_count < max) {
                    add_to_pfds(&pfds, newfd, &fd_count, &max);
                } else {
                    SendError(newfd, 2, ip);
                }

                printf("New connection from %d\n", newfd);

                struct args {
                    int newfd;
                    int i;
                    char *ip;
                } args = {newfd, i, ip};

                // HandleTCPClient in separate thread
                pthread_t thread;
                pthread_create(&thread, NULL,
                               (void *(*)(void *))HandleTCPClient,
                               (void *)&args);
                pthread_detach(thread);
            }
        }
    }
    write_log("", "Server shutdown", "", "");
    /* NOT REACHED */
}
