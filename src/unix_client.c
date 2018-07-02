#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


typedef enum CameraType {RAW_CAM, H264_CAM} CameraType;
typedef enum RTSPMessageType {GET_DEVICE_PROPS, TMP, ERR, COUNT} RTSPMessageType;
typedef struct v4l2_info {
    char camera_name[100];
    char mount_point[100];
    CameraType camera_type;
} v4l2_info;

char socket_path[80] = "mysocket";

void process_msg(char* read_buffer)
{

}

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
        // use a non-blocking fd here?
        int bytes_read=read(fd,read_buffer,sizeof(read_buffer));
        process_msg(read_buffer);
        printf("read %u bytes: %s ", bytes_read, read_buffer);
    }
}