#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char socket_path[80] = "mysocket";

int main()
{
    struct sockaddr_un addr;
    char buf[100];
    char read_buffer[100];
    int fd,rc;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(-1);
    }
    fprintf(stderr, "Socket fd - %d\n", fd);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, socket_path);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect error");
        exit(-1);
    }

    while ((rc=read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        if (write(fd, buf, rc) != rc) {
            if (rc > 0) {
                fprintf(stderr,"partial write");
            }
            else {
                perror("write error");
            }
        }
        int bytes_read=read(fd,read_buffer,sizeof(read_buffer));
        for (int i = 0; i < bytes_read; i++) {
            // printf("%c", read_buffer[i]);
        }
        printf("read %u bytes: %s\n", bytes_read, read_buffer);
    }
}