#include "IPCMessageHandler.h"

RTSPMessageType IPCMessageHandler::get_message_type(char* buf)
{
    string buffer(buf);
    for (size_t i = 0; i < RTSPMessageHeader.size(); i++) {
        if (!buffer.compare(0, RTSPMessageHeader[i].size(), RTSPMessageHeader[i])) {
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
    char info_buffer[IPC_BUFFER_SIZE];
    info_buffer[0] = '\0';

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
#if defined(__amd64__) || defined(__aarch64__)
            "\"frame_property_bitmask\": %lu, "
#endif
#ifdef __arm__
            "\"frame_property_bitmask\": %llu, "
#endif
            "\"current_quality\": %u, "
            "\"recording\": %d"
            "}",
            ip_address.c_str(),
            port.c_str(),
            device_props.first.c_str(),
            device_props.second.camera_name.c_str(),
            device_props.second.mount_point.c_str(),
            device_props.second.camera_type,
            device_props.second.frame_property_bitmask,
            device_props.second.quality,
            stream->get_recording());
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

    bool _record_stream;
    _record_stream = (record_video) ? true : false;
    RTSPAdaptiveStreaming* stream;

    try {
        stream = rtsp_stream_server->get_stream_map().at(string(video_device));
        rtsp_stream_server->set_stream_quality(string(video_device), camera_setting);
        if (stream->get_media_prepared()) {
            stream->set_device_properties(camera_setting, _record_stream);
        } else {
            cerr << "Stream not connected yet" << endl;
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
    default:
        cerr << "Unrecognised IPC header" << endl;
        break;
    }
}