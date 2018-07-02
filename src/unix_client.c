#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

char socket_path[80] = "mysocket";


typedef enum CameraType {RAW_CAM, H264_CAM} CameraType;
typedef enum RTSPMessageType {GET_DEVICE_PROPS, TMP, ERR, COUNT} RTSPMessageType;
typedef struct v4l2_info {
    char camera_name[100];
    char mount_point[100];
    CameraType camera_type;
} v4l2_info;

const char* RTSPMessageHeader[] = {
    "GDP", "TMP", "RES"
};


void print_v4l2_info(v4l2_info* info)
{
    printf("\n camname - %s // mountpt -  %s // cam_type - %d\n", info->camera_name, info->mount_point, info->mount_point);
}

RTSPMessageType get_message_type(char* buf)
{
    printf("gettingmsgtype\n");
    for (int i = 0; i < COUNT; i++) {
        if (!strcmp(buf, RTSPMessageHeader[i])) {
            printf("\nMSG TYPE - %s", RTSPMessageHeader[i]);
            // cout << "Message type - " << RTSPMessageHeader[i] << " " << static_cast<RTSPMessageType>(i);
            return i;
        }
    }
    return ERR;
}

void process_msg(char* read_buffer)
{
    printf("%s\n\n", read_buffer);
    char* p = strtok(read_buffer, "|");

    char* msg_header = strdup(p);
    RTSPMessageType message_type;
    message_type = get_message_type(msg_header);

    switch (message_type) {
    case GET_DEVICE_PROPS:
        p = strtok(NULL, "!");
        char* camera_name = strdup(p);
        p = strtok(NULL, "!");
        char* mount_pt = strdup(p);
        p = strtok(NULL, "!");
        char* cam_type = strdup(p);
        printf("CAMTYPE%s strlen - %d", cam_type, strlen(cam_type));

        // printf("header - %s // camera_name - %s // mountpt - %s // cam_type - %s\n", msg_header, camera_name, mount_pt, cam_type);
        v4l2_info cam0;
        strcpy(cam0.camera_name, camera_name);
        strcpy(cam0.mount_point, mount_pt);
        cam0.camera_type = atoi(cam_type);
        print_v4l2_info(&cam0);
        break;
    case TMP:
        break;
    case ERR:
        break;
    }

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
        int bytes_read=read(fd,read_buffer,sizeof(read_buffer));
        process_msg(read_buffer);
        printf("\nread %u bytes: %s\n", bytes_read, read_buffer);
    }
}