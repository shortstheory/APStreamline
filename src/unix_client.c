#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CAMERAS 8

char socket_path[80] = "/tmp/mysocket";


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

v4l2_info info_objs[MAX_CAMERAS];
int camera_count = 0;

void print_v4l2_info(v4l2_info* info)
{
    printf("camname - %s // mountpt -  %s // cam_type - %d\n", info->camera_name, info->mount_point, info->mount_point);
}

void v4l2_info_json(v4l2_info info, char* json)
{
    sprintf(json, "{\"name\": \"%s\", \"mount\": \"%s\", \"type\": \"%s\"}", info.camera_name, info.mount_point, info.camera_type);
    printf("JSON Str: %s\n", json);
}

void v4l2_array_json()
{
    char jsonarray[1000];
    jsonarray[0] = '\0';
    strcat(jsonarray, "[");
    for (int i = 0; i < camera_count; i++) {
        char tmp[1000];
        v4l2_info_json(info_objs[i], tmp);
        if (i == 0) {
            strcat(jsonarray, tmp);
        }
        else {
            strcat(jsonarray, ", ");
            strcat(jsonarray, tmp);
        }
    }
    strcat(jsonarray, "]");
    // sprintf(jsonarray, "%s]", jsonarray);
    printf("JSON Array: %s\n", jsonarray);

}

RTSPMessageType get_message_type(char* buf)
{
    for (int i = 0; i < COUNT; i++) {
        if (!strcmp(buf, RTSPMessageHeader[i])) {
            // printf("\nMSG TYPE - %s", RTSPMessageHeader[i]);
            // cout << "Message type - " << RTSPMessageHeader[i] << " " << static_cast<RTSPMessageType>(i);
            return i;
        }
    }
    return ERR;
}

void process_device_props(char* p, v4l2_info* info)
{
    // p = strtok(NULL, "|");
    p = strtok(NULL, "!");
    char* camera_name = strdup(p);
    p = strtok(NULL, "!");
    char* mount_pt = strdup(p);
    p = strtok(NULL, "!");
    char* cam_type = strdup(p);
    // printf("CAMTYPE%s strlen - %d", cam_type, strlen(cam_type));

    strcpy(info->camera_name, camera_name);
    strcpy(info->mount_point, mount_pt);
    info->camera_type = atoi(cam_type);
}

int curr_counter=0;
void store_cam_info(v4l2_info info)
{
    if (strcmp(info.mount_point,"NULL")) {
        info_objs[curr_counter++] = info;
        printf("Saving obj %s\n", info.mount_point);
    }
    else {
        printf("Done saving objs @count of %d", camera_count);
        camera_count = curr_counter+1;
        curr_counter = 0;
        v4l2_array_json();
    }
}

void process_msg(char* read_buffer)
{
    printf("%s\n\n", read_buffer);
    char* p = strtok(read_buffer, "$");

    char* msg_header = strdup(p);
    RTSPMessageType message_type;
    message_type = get_message_type(msg_header);

    p = strtok(NULL, "$");

    switch (message_type) {
    case GET_DEVICE_PROPS:
        ; // so bizarre!!!
        printf("Got JSON - %s", p);
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
    int fd;

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

    write(fd, "GDP", 4);
    char read_buffer[1000];
    int bytes_read = recv(fd, read_buffer, sizeof(read_buffer), 0);
    process_msg(read_buffer);

    printf("\nRead buffer - %s\n", read_buffer);
}