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
    return RTSPMessageType::ERR;
}

string IPCMessageHandler::get_message_payload(char* buf)
{
    string buffer(buf);
    return buffer.substr(3);
}

string IPCMessageHandler::serialise_device_props(v4l2_info device_props)
{
    string list;
    list = RTSPMessageHeader[RTSPMessageType::GET_DEVICE_PROPS] + "$";
    string dev_info;
    // weird hack to work around \0?
    // dev_info = device_props.camera_name.substr(0, device_props.camera_name.size()-1) + "!" + device_props.mount_point + "!" + to_string(device_props.camera_type);
    char info_buffer[1000];
    info_buffer[0] = '\0';
    cout << "BITMASK - " << device_props.frame_property_bitmask;
    // dev_info =  + "!" +  + "!" + to_string(device_props.camera_type);
    sprintf(info_buffer, "{\"name\": \"%s\", \"mount\": \"%s\", \"camtype\": %d, \"frame_property_bitmask\": %llu}", 
                            device_props.camera_name.c_str(),
                            device_props.mount_point.c_str(),
                            device_props.camera_type,
                            device_props.frame_property_bitmask);
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
    vector<v4l2_info> device_props;
    device_props = rtsp_stream_server->get_device_properties();
    string json_message;
    json_message = "GDP$[";
    // for (v4l2_info info : device_props) {
    //     string device_list;
    //     device_list = serialise_device_props(info);
    //     send_string(device_list);
    // }
    for (int i = 0; i < device_props.size(); i++) {
        if (i == 0) {
            json_message = json_message + serialise_device_props(device_props.at(i));
        }
        else {
            json_message = json_message + ", " + serialise_device_props(device_props.at(i));
        }
    }
    json_message = json_message + "]";
    cout << "\n\nJSON Msg" << json_message << endl;

    send_string(json_message);
    // v4l2_info sentinel;
    // sentinel.camera_name = "NULL";
    // sentinel.mount_point = "NULL";
    // sentinel.camera_type = CameraType::RAW_CAM;
    // send_string(serialise_device_props(sentinel));
    // send_string("GDP$NULL!NULL!NULL");
}

IPCMessageHandler::IPCMessageHandler(int fd, RTSPStreamServer* _rtsp_stream_server) : client_fd(fd)
{
    assert(_rtsp_stream_server);
    assert(client_fd != -1);
    rtsp_stream_server = _rtsp_stream_server;
}

void IPCMessageHandler::process_msg(char* buf)
{
    RTSPMessageType msgtype;
    msgtype = get_message_type(buf);
    switch (msgtype) {
    case RTSPMessageType::GET_DEVICE_PROPS:
        send_device_props();
        break;
    case RTSPMessageType::TMP:
        break;
    case RTSPMessageType::ERR:
        g_warning("Unrecognised header");
        break;
    }
}