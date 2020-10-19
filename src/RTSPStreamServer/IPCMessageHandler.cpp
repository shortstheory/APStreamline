#include "IPCMessageHandler.h"
#include <sstream>

string IPCMessageHandler::get_message_payload(char* buf)
{
    string buffer(buf);
    return buffer.substr(4);
}

string IPCMessageHandler::serialise_device_props(pair<string, shared_ptr<RTSPAdaptiveStreaming>> device_props)
{
    string list;
    list = "GDP$";
    string dev_info;
    // weird hack to work around \0?
    char info_buffer[1000];
    info_buffer[0] = '\0';

    string ip_address;
    string port;
    string device;
    string camera_name;
    shared_ptr<Camera> camera;
    shared_ptr<RTSPAdaptiveStreaming> stream;
    stream = device_props.second;
    camera = stream->get_camera();
    ip_address = rtsp_stream_server->get_ip_address();
    port = rtsp_stream_server->get_port();
    device = device_props.first;

    sprintf(info_buffer, "{"
            "\"ip\": \"%s\", "
            "\"port\": \"%s\", "
            "\"dev_mount\": \"%s\", "
            "\"name\": \"%s\", "
            "\"mount\": \"%s\", "
            "\"camtype\": %d, "
            "\"frame_property_bitmask\": %d, "
            "\"resolution\": %u, "
            "\"recording\": %d"
            "}",
            ip_address.c_str(),
            port.c_str(),
            camera->get_device_path().c_str(),
            camera->get_name().c_str(),
            stream->get_uri().c_str(),
            0, // unsupported right now
            camera->get_supported_qualities(),
            camera->get_quality().to_int(),
            false); // also unsupported
    return string(info_buffer);
}

bool IPCMessageHandler::send_string(string data)
{
    int numbytes;
    numbytes = send(client_fd, data.c_str(), data.length(), 0);
    if (numbytes > 0) {
        return true;
    }
    return false;
}

void IPCMessageHandler::send_device_props()
{
    auto device_map = rtsp_stream_server->get_stream_map();
    string json_message;
    json_message = "GDP$[";

    for (auto it = device_map.begin(); it != device_map.end(); it++) {
        if (it == device_map.begin()) {
            json_message = json_message + serialise_device_props(*it);
        } else {
            json_message = json_message + ", " + serialise_device_props(*it);
        }
    }

    json_message = json_message + "]";
    send_string(json_message);
}

IPCMessageHandler::IPCMessageHandler(int fd, RTSPStreamServer* _rtsp_stream_server) : client_fd(fd)
{
    assert(_rtsp_stream_server);
    assert(client_fd != -1);
    rtsp_stream_server = _rtsp_stream_server;
}

void IPCMessageHandler::set_device_quality(char* buffer)
{
    string msg_payload;
    char video_device[20];
    int camera_setting;
    int record_video;

    msg_payload = get_message_payload(buffer);
    sscanf(msg_payload.c_str(), "%s %d %d", video_device, &camera_setting, &record_video);

    shared_ptr<RTSPAdaptiveStreaming> stream;

    try {
        stream = rtsp_stream_server->get_stream_map().at(string(video_device));
        stream->set_quality(camera_setting);
        if (!stream->get_media_prepared()) {
            cerr << "Stream not connected yet" << endl;
            stream->init_media_factory();
        }
    } catch (const out_of_range& err) {
        cerr << err.what();
    }
}

void IPCMessageHandler::process_msg(char* buf)
{
    string buffer(buf);
    if (buffer.find("GDP") != string::npos) {
        send_device_props();
    } else if (buffer.find("SDP") != string::npos) {
        set_device_quality(buf);
    } else {
        cerr << "Unrecognised IPC header" << endl;
    }
}