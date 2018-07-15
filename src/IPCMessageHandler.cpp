#include "IPCMessageHandler.h"

RTSPMessageType IPCMessageHandler::get_message_type(char* buf)
{
    string buffer(buf);
    cout << "Recevied buffer - " << buf << endl;
    for (int i = 0; i < RTSPMessageHeader.size(); i++) {
        if (!buffer.compare(0, RTSPMessageHeader[i].size(), RTSPMessageHeader[i])) {
            cout << "Message type - " << RTSPMessageHeader[i] << " " << static_cast<RTSPMessageType>(i) << endl;
            return static_cast<RTSPMessageType>(i);
        }
    }
    return ERROR;
}

string IPCMessageHandler::get_message_payload(char* buf)
{
    string buffer(buf);
    return buffer.substr(4);
}

string IPCMessageHandler::serialise_device_props(pair<string, v4l2_info> device_props)
{
    string list;
    list = RTSPMessageHeader[RTSPMessageType::GET_DEVICE_PROPS] + "$";
    string dev_info;
    // weird hack to work around \0?
    char info_buffer[1000];
    info_buffer[0] = '\0';
    cout << "BITMASK - " << device_props.second.frame_property_bitmask;

    RTSPAdaptiveStreaming* stream;
    stream = rtsp_stream_server->get_stream_map().at(device_props.first);

    string ip_address;
    ip_address = rtsp_stream_server->get_ip_address();

    string port;
    port = rtsp_stream_server->get_port();

    sprintf(info_buffer, "{"
            "\"ip\": \"%s\", "
            "\"port\": \"%s\", "
            "\"dev_mount\": \"%s\", "
            "\"name\": \"%s\", "
            "\"mount\": \"%s\", "
            "\"camtype\": %d, "
            "\"frame_property_bitmask\": %llu, "
            "\"current_quality\": %u"
            "}",
            ip_address.c_str(),
            port.c_str(),
            device_props.first.c_str(),
            device_props.second.camera_name.c_str(),
            device_props.second.mount_point.c_str(),
            device_props.second.camera_type,
            device_props.second.frame_property_bitmask,
            stream->current_quality);
    // cout << dev_info;
    printf("SERIALIST!! %s", info_buffer);
    return string(info_buffer);
}

bool IPCMessageHandler::send_string(string data)
{
    int numbytes;
    numbytes = send(client_fd, data.c_str(), data.length(), 0);
    if (numbytes > 0) {
        cout << "Bytes sent - " << numbytes << endl;
        return true;
    }
    return false;
}

void IPCMessageHandler::send_device_props()
{
    auto device_map = rtsp_stream_server->get_device_map();
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
    cout << "\n\nJSON Msg" << json_message << endl;

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
    cout << "Got an SDP message! " << buffer << endl;
    char video_device[20];
    int camera_setting;

    msg_payload = get_message_payload(buffer);
    cout << "Payload string - " << msg_payload << endl;
    sscanf(msg_payload.c_str(), "%s %d", video_device, &camera_setting);

    RTSPAdaptiveStreaming* stream;
    try {
        stream = rtsp_stream_server->get_stream_map().at(string(video_device));
        if (stream->get_media_prepared()) {
            stream->change_quality_preset(camera_setting);
        } else {
            g_warning("Stream not connected yet");
        }
    } catch (const out_of_range& err) {
        cerr << err.what();
    }
}

void IPCMessageHandler::process_msg(char* buf)
{
    RTSPMessageType msgtype;
    msgtype = get_message_type(buf);
    switch (msgtype) {
    case GET_DEVICE_PROPS:
        send_device_props();
        break;
    case SET_DEVICE_PROPS:
        set_device_quality(buf);
        break;
    case ERROR:
        g_warning("Unrecognised header");
        break;
    }
}