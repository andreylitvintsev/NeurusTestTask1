#include "YoloObjectDetector.h"

YoloObjectDetector::YoloObjectDetector(
        const std::string &configPath,
        const std::string &weightsPath,
        const std::string &classesPath,
        float confidenceThreshold,
        float nonMaximumThreshold
)
: _confidenceThreshold{confidenceThreshold}, _nonMaximumThreshold {nonMaximumThreshold}
{
    // init _classes
    std::ifstream ifs{classesPath};
    std::string line{};
    while (getline(ifs, line))
        _classes.push_back(line);

    // init _net
    _net = cv::dnn::readNetFromDarknet(configPath, weightsPath);
    _net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    _net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
}

std::vector<cv::String> YoloObjectDetector::GetOutputsNames(const cv::dnn::Net &net) {
    static std::vector<cv::String> names{};
    if (names.empty()) {
        //Get the indices of the output layers, i.e. the layers with unconnected outputs
        std::vector<int> outLayers = net.getUnconnectedOutLayers();

        //get the names of all the layers in the network
        std::vector<cv::String> layersNames = net.getLayerNames();

        // Get the names of the output layers in names
        names.resize(outLayers.size());
        for (size_t i = 0; i < outLayers.size(); ++i)
            names[i] = layersNames[outLayers[i] - 1];
    }
    return names;
}

void YoloObjectDetector::DrawPred(int classId, float conf, int left, int top, int right, int bottom, cv::Mat& frame) {
    //Draw a rectangle displaying the bounding box
    rectangle(frame, cv::Point(left, top), cv::Point(right, bottom), cv::Scalar(255, 178, 50), 3);

    //Get the label for the class name and its confidence
    std::string label = cv::format("%.2f", conf);
    if (!_classes.empty()) {
        CV_Assert(classId < (int) _classes.size());
        label = _classes[classId] + ":" + label;
    }

    //Display the label at the top of the bounding box
    int baseLine;
    cv::Size labelSize = getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
    top = std::max(top, labelSize.height);
    rectangle(frame, cv::Point(left, top - round(1.5 * labelSize.height)),
              cv::Point(left + round(1.5 * labelSize.width), top + baseLine), cv::Scalar(255, 255, 255), cv::FILLED);
    putText(frame, label, cv::Point(left, top), cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(0, 0, 0), 1);
}

void YoloObjectDetector::Postprocess(cv::Mat& frame, const std::vector<cv::Mat>& outs) {
    std::vector<int> classIds{};
    std::vector<float> confidences{};
    std::vector<cv::Rect> boxes{};

    for (const auto& out : outs) {
        // Scan through all the bounding boxes output from the network and keep only the
        // ones with high confidence scores. Assign the box's class label as the class
        // with the highest score for the box.
        float* data = reinterpret_cast<float*>(out.data);
        for (int j = 0; j < out.rows; ++j, data += out.cols) {
            cv::Mat scores = out.row(j).colRange(5, out.cols);
            cv::Point classIdPoint{};
            double confidence{};
            // Get the value and location of the maximum score
            minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
            if (confidence > _confidenceThreshold) {
                int centerX = (int) (data[0] * frame.cols);
                int centerY = (int) (data[1] * frame.rows);
                int width = (int) (data[2] * frame.cols);
                int height = (int) (data[3] * frame.rows);
                int left = centerX - width / 2;
                int top = centerY - height / 2;

                classIds.push_back(classIdPoint.x);
                confidences.push_back((float) confidence);
                boxes.emplace_back(left, top, width, height);
            }
        }
    }
    // Perform non-maximum suppression to eliminate redundant overlapping boxes with
    // lower confidences
    std::vector<int> indices{};
    cv::dnn::NMSBoxes(boxes, confidences, _confidenceThreshold, _nonMaximumThreshold, indices);

    for (int idx : indices) {
        cv::Rect box = boxes[idx];
        DrawPred(classIds[idx], confidences[idx], box.x, box.y,
                 box.x + box.width, box.y + box.height, frame);
    }
}
