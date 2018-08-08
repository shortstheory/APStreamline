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

// Separate thread for managing the IPC with the APWeb server
void ipc_loop(RTSPStreamServer& stream_server)
{
    struct sockaddr_un addr;
    char buf[IPC_BUFFER_SIZE];
    int socket_fd, client_fd, bytes_read;

    if ((socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        cerr << "Local socket creation failed, IPC will be disabled" << endl;
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH.c_str());
    unlink(SOCKET_PATH.c_str());

    if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        cerr << "Bind error" << endl;
        return;
    }

    if (listen(socket_fd, 1) == -1) {
        cerr << "Listen error" << endl;
        return;
    }

    while ((client_fd = accept(socket_fd, NULL, NULL))) {
        IPCMessageHandler message_handler(client_fd, &stream_server);
        while ((bytes_read=read(client_fd, buf, sizeof(buf))) > 0) {
            buf[bytes_read] = '\0';
            message_handler.process_msg(buf);
        }
    }
}

string get_ip_address(string interface = "lo")
{
    struct ifaddrs *ifaddr, *ifa;
    int s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }
        s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST,
                        NULL, 0, NI_NUMERICHOST);
        if ((strcmp(ifa->ifa_name, interface.c_str()) == 0) && (ifa->ifa_addr->sa_family==AF_INET)) {
            if (s != 0) {
                cout << "getnameinfo() failed: %s\n" << gai_strerror(s) << endl;
            }
            cout << "Interface : " << ifa->ifa_name << endl;
            cout << "Address : " <<  host << endl;
            freeifaddrs(ifaddr);
            return string(host);
        }
    }
    freeifaddrs(ifaddr);
    cerr << "No IP found for given interface" << endl;
    return string("127.0.0.1");
}

GMainLoop* loop;

// SIGTERM handler so we can clean up before destroying the stream
void terminate_process(int signum)
{
    g_main_loop_quit(loop);
    cerr << "Process terminated " << signum << endl;
}

// First argument is used as the network interface to use for the Stream Server
// e.g. ./stream_server eth0
int main(int argc, char *argv[])
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = terminate_process;
    sigaction(SIGINT, &action, NULL);

    gst_init(&argc, &argv);
    loop = g_main_loop_new(NULL, FALSE);

    string ip_addr;
    if (argc > 1) {
        ip_addr = get_ip_address(argv[1]);
    } else {
        ip_addr = "127.0.0.1";
    }
    RTSPStreamServer rtsp_stream_server(ip_addr, "8554");
    rtsp_stream_server.set_service_id(gst_rtsp_server_attach(rtsp_stream_server.get_server(), NULL));

    thread t(&ipc_loop, ref(rtsp_stream_server));
    t.detach();

    g_main_loop_run(loop);
    return 0;
}