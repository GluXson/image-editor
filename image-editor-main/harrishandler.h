#pragma once

#include <QObject>
#include <QImage>
#include <vector>

#include "editorcontext.h"

class HarrisHandler : public QObject
{
    Q_OBJECT

public:
    explicit HarrisHandler(const EditorContext &context, QObject *parent = nullptr);

public slots:
    void applyHarris(int blockSize, int sobelKernelSize, double k, int threshold);

private:
    EditorContext ctx;

    static int clampToByte(int value);
    static QImage ensureRgbImage(const QImage &img);
    static QImage toGrayscaleImage(const QImage &img);

    static std::vector<std::vector<double>> imageToMatrix(const QImage &img);
    static std::vector<std::vector<double>> convolveGray(
        const std::vector<std::vector<double>> &src,
        const std::vector<std::vector<double>> &kernel);

    static std::vector<std::vector<double>> buildSobelKernelX(int kernelSize);
    static std::vector<std::vector<double>> buildSobelKernelY(int kernelSize);

    static std::vector<std::vector<double>> computeHarrisResponse(
        const std::vector<std::vector<double>> &ix,
        const std::vector<std::vector<double>> &iy,
        int blockSize,
        double k);

    static double maxResponse(const std::vector<std::vector<double>> &response);

    static QImage drawCorners(
        const QImage &baseImage,
        const std::vector<std::vector<double>> &response,
        double responseThreshold);
};