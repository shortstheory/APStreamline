#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include "PipelineManager.h"
#include "../Camera/CameraFactory.h"

PipelineManager::PipelineManager(string _device, CameraType type) : congested(false), successive_transmissions(0), auto_mode(true)
{
    Quality startingQuality(Quality::Level::LOW, Quality::Level::MEDIUM);
    cam = CameraFactory(_device, startingQuality, type).get_camera();
}

void PipelineManager::adapt_stream()
{
    QoSReport qos_report;
    qos_report = qos_estimator.get_qos_report();

    if (successive_transmissions >= SUCCESSFUL_TRANSMISSION) {
        congested = false;
    }

    // If we get a number of transmissions without any packet loss, we can increase bitrate
    if (qos_report.fraction_lost == 0) {
        successive_transmissions++;
        if (multi_udp_sink) {
            if (qos_report.encoding_bitrate < qos_report.estimated_bitrate * 1.5) {
                cam->improve_quality(congested);
            } else {
                cerr << "Potential buffer overflow!" << endl;
                cam->degrade_quality(congested);
            }
        } else {
            cam->improve_quality(congested);
        }
    } else {
        congested = true;
        successive_transmissions = 0;
        cam->degrade_quality(congested);
    }
}

bool PipelineManager::get_element_references()
{
    if (pipeline) {
        tee = gst_bin_get_by_name(GST_BIN(pipeline), "tee_element");
        rtph264_payloader = gst_bin_get_by_name(GST_BIN(pipeline), "pay0");
        if (cam->set_element_references(pipeline) && tee && rtph264_payloader) {
            return true;
        }
    }
    return false;
}

string PipelineManager::get_device_path()
{
    // return device;
    return cam->get_device_path();
}

shared_ptr<Camera> PipelineManager::get_camera()
{
    return cam;
}

void PipelineManager::set_pipeline_element(GstElement *_element)
{
    pipeline = _element;
    cam->set_element_references(pipeline);
}

bool PipelineManager::is_auto()
{
    return auto_mode;
}

void PipelineManager::set_auto(bool mode)
{
    auto_mode = mode;
}