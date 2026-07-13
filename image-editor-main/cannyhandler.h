#pragma once

#include <QObject>
#include <QImage>

#include <vector>

#include "editorcontext.h"

class CannyHandler : public QObject
{
    Q_OBJECT

public:
    explicit CannyHandler(const EditorContext &context, QObject *parent = nullptr);

public slots:
    void applyCanny(int kernelSize,
                    double sigma,
                    int lowThreshold,
                    int highThreshold,
                    bool useL2Gradient);

private:
    EditorContext ctx;

    static int clampToByte(int value);
    static QImage ensureRgbImage(const QImage &img);
    static QImage toGrayscaleImage(const QImage &img);

    static std::vector<std::vector<double>> buildGaussianKernel(int kernelSize, double sigma);
    static std::vector<std::vector<double>> buildSobelKernelX();
    static std::vector<std::vector<double>> buildSobelKernelY();

    static std::vector<std::vector<double>> convolveGray(
        const std::vector<std::vector<double>> &src,
        const std::vector<std::vector<double>> &kernel);

    static std::vector<std::vector<double>> imageToMatrix(const QImage &img);
    static QImage matrixToImage(const std::vector<std::vector<double>> &matrix);
    static QImage binaryMatrixToImage(const std::vector<std::vector<int>> &matrix);

    static std::vector<std::vector<double>> computeGradientMagnitude(
        const std::vector<std::vector<double>> &gx,
        const std::vector<std::vector<double>> &gy,
        bool useL2Gradient);

    static std::vector<std::vector<double>> computeGradientDirection(
        const std::vector<std::vector<double>> &gx,
        const std::vector<std::vector<double>> &gy);

    static std::vector<std::vector<double>> nonMaximumSuppression(
        const std::vector<std::vector<double>> &magnitude,
        const std::vector<std::vector<double>> &direction);

    static std::vector<std::vector<int>> applyHysteresis(
        const std::vector<std::vector<double>> &suppressed,
        int lowThreshold,
        int highThreshold);
};