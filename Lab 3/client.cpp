#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// Output client

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int fd, writenOrReadLength;
    struct sockaddr_in serverAddress;

    struct hostent *server = gethostbyname("localhost");
    int port = 5000;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        error("ERROR opening socket");
    }

    if (server == NULL) {
        error("ERROR, no such host\n");
    }

    bzero((char *) &serverAddress, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;

    bcopy((char *)server->h_addr, (char *)&serverAddress.sin_addr.s_addr, server->h_length);

    serverAddress.sin_port = htons(port);

    if (connect(fd,(struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        error("ERROR connecting");
    }

    char buffer[256];

    while (true) {
        printf("Message: ");

        bzero(buffer, 256);
        fgets(buffer, 255, stdin);

        writenOrReadLength = write(fd, buffer, strlen(buffer));

        if (writenOrReadLength < 0) {
            error("ERROR can't write to socket");
        }

        bzero(buffer, 256);
        writenOrReadLength = read(fd, buffer, 255);

        if (writenOrReadLength < 0) {
            error("ERROR can't read from socket");
        }

        printf("Read from server: %s\n", buffer);
    }

    close(fd);

    return 0;
}
