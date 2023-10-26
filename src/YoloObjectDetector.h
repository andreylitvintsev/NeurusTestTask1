/*
 * Based on https://learnopencv.com/deep-learning-based-object-detection-using-yolov3-with-opencv-python-c/
 */

#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <opencv2/opencv.hpp>

class YoloObjectDetector {
private:
    std::vector<std::string> _classes{};
    cv::dnn::Net _net{};

    float _confidenceThreshold{};
    float _nonMaximumThreshold{};

public:
    YoloObjectDetector(
        const std::string& configPath,
        const std::string& weightsPath,
        const std::string& classesPath,
        float confidenceThreshold,
        float nonMaximumThreshold
    );

    YoloObjectDetector(const YoloObjectDetector& other) = delete;
    YoloObjectDetector(YoloObjectDetector&& other) noexcept = delete;

    YoloObjectDetector& operator=(const YoloObjectDetector& other) = delete;
    YoloObjectDetector& operator=(YoloObjectDetector&& other) noexcept = delete;

    ~YoloObjectDetector() = default;

    cv::Mat HandleFrame(const cv::Mat& frame, bool drawResults) {
        cv::Mat inputBlob = cv::dnn::blobFromImage(frame, 1 / 255.0f, cv::Size(416, 416), cv::Scalar(), true, false);
        _net.setInput(inputBlob);
        std::vector<cv::Mat> outs;
        _net.forward(outs, GetOutputsNames(_net));

        cv::Mat handledFrame{frame};
        if (drawResults)
            Postprocess(handledFrame, outs);
        return std::move(handledFrame);
    }

private:
    static std::vector<cv::String> GetOutputsNames(const cv::dnn::Net& net);

    // Draw the predicted bounding box
    void DrawPred(int classId, float conf, int left, int top, int right, int bottom, cv::Mat& frame);

    // Remove the bounding boxes with low confidence using non-maxima suppression
    void Postprocess(cv::Mat& frame, const std::vector<cv::Mat>& outs);
};
