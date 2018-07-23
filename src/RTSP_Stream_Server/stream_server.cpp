#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <signal.h>

#include "RTSPStreamServer.h"
#include "IPCMessageHandler.h"

char socket_path[80] = "/tmp/rtsp_server.sock";
RTSPStreamServer* rtsp_stream_server;
// char socket_path[80] = "../../rtsp_server";

void ipc_loop(RTSPStreamServer* streamer)
{
    struct sockaddr_un addr;
    char buf[1000];
    int socket_fd, client_fd, bytes_read;

    if ((socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        g_warning("Local socket creation failed, IPC will be disabled");
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, socket_path);
    unlink(socket_path);

    if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        g_warning("Bind error");
        return;
    }

    if (listen(socket_fd, 1) == -1) {
        g_warning("Listen error");
        return;
    }

    // if ((client_fd = accept(socket_fd, NULL, NULL)) == -1) {
    //     g_warning("Connection failed");

    while (client_fd = accept(socket_fd, NULL, NULL)) {
        IPCMessageHandler message_handler(client_fd, streamer);
        // g_warning("Connection accepted!");
        while ((bytes_read=read(client_fd,buf,sizeof(buf))) > 0) {
            buf[bytes_read] = '\0';
            message_handler.process_msg(buf);
            printf("read %u bytes: %s\n", bytes_read, buf);
        }
        // g_warning("Disconnected from loop!");
    }
}

string get_ip_address(string interface = "lo")
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }
        s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in), host, NI_MAXHOST,
                      NULL, 0, NI_NUMERICHOST);
        if ((strcmp(ifa->ifa_name, interface.c_str()) == 0) && (ifa->ifa_addr->sa_family==AF_INET)) {
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
            }
            printf("Interface : <%s>\n",ifa->ifa_name);
            printf("Address : <%s>\n", host);
            freeifaddrs(ifaddr);
            return string(host);
        }
    }
    freeifaddrs(ifaddr);
    g_warning("No IP found for given interface");
    return string("127.0.0.1");
}

void terminate_process(int signum)
{
    // cerr << "Process terminated" << signum << endl;
    fprintf(stderr, "Process terminated %d\n", signum);
    free(rtsp_stream_server);
    exit(1);
}

int main(int argc, char *argv[])
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = terminate_process;
    sigaction(SIGTERM, &action, NULL);

    gst_init(&argc, &argv);
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);

    string ip_addr;
    if (argc > 1) {
        ip_addr = get_ip_address(argv[1]);
    } else {
        ip_addr = "127.0.0.1";
    }
    rtsp_stream_server = RTSPStreamServer::get_instance(ip_addr, "8554");
    gst_rtsp_server_attach(rtsp_stream_server->get_server(), NULL);

    thread t(&ipc_loop, rtsp_stream_server);
    t.detach();

    g_main_loop_run(loop);
    return 0;
}