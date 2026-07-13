#include "harrishandler.h"

#include <QColor>
#include <QMessageBox>

#include <algorithm>
#include <cmath>

HarrisHandler::HarrisHandler(const EditorContext &context, QObject *parent)
    : QObject(parent)
    , ctx(context)
{
}

int HarrisHandler::clampToByte(int value)
{
    return std::max(0, std::min(255, value));
}

QImage HarrisHandler::ensureRgbImage(const QImage &img)
{
    if (img.format() == QImage::Format_RGB32 ||
        img.format() == QImage::Format_ARGB32 ||
        img.format() == QImage::Format_ARGB32_Premultiplied) {
        return img;
    }

    return img.convertToFormat(QImage::Format_RGB32);
}

QImage HarrisHandler::toGrayscaleImage(const QImage &img)
{
    const QImage rgb = ensureRgbImage(img);
    QImage gray(rgb.size(), QImage::Format_RGB32);

    for (int y = 0; y < rgb.height(); ++y) {
        for (int x = 0; x < rgb.width(); ++x) {
            const QColor c = rgb.pixelColor(x, y);
            const int g = clampToByte(static_cast<int>(
                std::round(0.299 * c.red() + 0.587 * c.green() + 0.114 * c.blue())));
            gray.setPixelColor(x, y, QColor(g, g, g));
        }
    }

    return gray;
}

std::vector<std::vector<double>> HarrisHandler::imageToMatrix(const QImage &img)
{
    const QImage gray = toGrayscaleImage(img);

    std::vector<std::vector<double>> matrix(
        gray.height(),
        std::vector<double>(gray.width(), 0.0));

    for (int y = 0; y < gray.height(); ++y) {
        for (int x = 0; x < gray.width(); ++x) {
            matrix[y][x] = gray.pixelColor(x, y).red();
        }
    }

    return matrix;
}

std::vector<std::vector<double>> HarrisHandler::convolveGray(
    const std::vector<std::vector<double>> &src,
    const std::vector<std::vector<double>> &kernel)
{
    if (src.empty() || src[0].empty() || kernel.empty() || kernel[0].empty()) {
        return {};
    }

    const int height = static_cast<int>(src.size());
    const int width = static_cast<int>(src[0].size());
    const int kernelSize = static_cast<int>(kernel.size());
    const int radius = kernelSize / 2;

    std::vector<std::vector<double>> result(
        height,
        std::vector<double>(width, 0.0));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double sum = 0.0;

            for (int ky = 0; ky < kernelSize; ++ky) {
                for (int kx = 0; kx < kernelSize; ++kx) {
                    const int iy = std::clamp(y + ky - radius, 0, height - 1);
                    const int ix = std::clamp(x + kx - radius, 0, width - 1);
                    sum += src[iy][ix] * kernel[ky][kx];
                }
            }

            result[y][x] = sum;
        }
    }

    return result;
}

std::vector<std::vector<double>> HarrisHandler::buildSobelKernelX(int kernelSize)
{
    if (kernelSize <= 3) {
        return {
            {-1.0, 0.0, 1.0},
            {-2.0, 0.0, 2.0},
            {-1.0, 0.0, 1.0}
        };
    }

    if (kernelSize == 5) {
        return {
            {-1.0, -2.0, 0.0,  2.0,  1.0},
            {-4.0, -8.0, 0.0,  8.0,  4.0},
            {-6.0,-12.0, 0.0, 12.0,  6.0},
            {-4.0, -8.0, 0.0,  8.0,  4.0},
            {-1.0, -2.0, 0.0,  2.0,  1.0}
        };
    }

    return {
        {-1.0, -4.0, -5.0, 0.0,  5.0,  4.0,  1.0},
        {-6.0,-24.0,-30.0, 0.0, 30.0, 24.0,  6.0},
        {-15.0,-60.0,-75.0,0.0, 75.0, 60.0, 15.0},
        {-20.0,-80.0,-100.0,0.0,100.0,80.0,20.0},
        {-15.0,-60.0,-75.0,0.0, 75.0, 60.0,15.0},
        {-6.0,-24.0,-30.0, 0.0, 30.0, 24.0, 6.0},
        {-1.0,-4.0,-5.0,  0.0,  5.0,  4.0, 1.0}
    };
}

std::vector<std::vector<double>> HarrisHandler::buildSobelKernelY(int kernelSize)
{
    if (kernelSize <= 3) {
        return {
            {-1.0, -2.0, -1.0},
            { 0.0,  0.0,  0.0},
            { 1.0,  2.0,  1.0}
        };
    }

    if (kernelSize == 5) {
        return {
            {-1.0, -4.0, -6.0, -4.0, -1.0},
            {-2.0, -8.0,-12.0, -8.0, -2.0},
            { 0.0,  0.0,  0.0,  0.0,  0.0},
            { 2.0,  8.0, 12.0,  8.0,  2.0},
            { 1.0,  4.0,  6.0,  4.0,  1.0}
        };
    }

    return {
        {-1.0, -6.0, -15.0, -20.0, -15.0, -6.0, -1.0},
        {-4.0,-24.0, -60.0, -80.0, -60.0,-24.0, -4.0},
        {-5.0,-30.0, -75.0,-100.0, -75.0,-30.0, -5.0},
        { 0.0,  0.0,   0.0,   0.0,   0.0,  0.0,  0.0},
        { 5.0, 30.0,  75.0, 100.0,  75.0, 30.0,  5.0},
        { 4.0, 24.0,  60.0,  80.0,  60.0, 24.0,  4.0},
        { 1.0,  6.0,  15.0,  20.0,  15.0,  6.0,  1.0}
    };
}

std::vector<std::vector<double>> HarrisHandler::computeHarrisResponse(
    const std::vector<std::vector<double>> &ix,
    const std::vector<std::vector<double>> &iy,
    int blockSize,
    double k)
{
    if (ix.empty() || ix[0].empty() || iy.empty() || iy[0].empty()) {
        return {};
    }

    const int height = static_cast<int>(ix.size());
    const int width = static_cast<int>(ix[0].size());
    const int radius = std::max(1, blockSize / 2);

    std::vector<std::vector<double>> response(
        height,
        std::vector<double>(width, 0.0));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double sumIx2 = 0.0;
            double sumIy2 = 0.0;
            double sumIxIy = 0.0;

            for (int wy = -radius; wy <= radius; ++wy) {
                for (int wx = -radius; wx <= radius; ++wx) {
                    const int iyPos = std::clamp(y + wy, 0, height - 1);
                    const int ixPos = std::clamp(x + wx, 0, width - 1);

                    const double gx = ix[iyPos][ixPos];
                    const double gy = iy[iyPos][ixPos];

                    sumIx2 += gx * gx;
                    sumIy2 += gy * gy;
                    sumIxIy += gx * gy;
                }
            }

            const double det = (sumIx2 * sumIy2) - (sumIxIy * sumIxIy);
            const double trace = sumIx2 + sumIy2;

            response[y][x] = det - k * trace * trace;
        }
    }

    return response;
}

double HarrisHandler::maxResponse(const std::vector<std::vector<double>> &response)
{
    double maxValue = 0.0;

    for (const auto &row : response) {
        for (double value : row) {
            maxValue = std::max(maxValue, value);
        }
    }

    return maxValue;
}

QImage HarrisHandler::drawCorners(
    const QImage &baseImage,
    const std::vector<std::vector<double>> &response,
    double responseThreshold)
{
    if (baseImage.isNull() || response.empty() || response[0].empty()) {
        return QImage();
    }

    QImage result = ensureRgbImage(baseImage);
    const int height = static_cast<int>(response.size());
    const int width = static_cast<int>(response[0].size());

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            const double current = response[y][x];
            if (current < responseThreshold) {
                continue;
            }

            bool isLocalMaximum = true;

            for (int ny = -1; ny <= 1 && isLocalMaximum; ++ny) {
                for (int nx = -1; nx <= 1; ++nx) {
                    if (ny == 0 && nx == 0) {
                        continue;
                    }

                    if (response[y + ny][x + nx] > current) {
                        isLocalMaximum = false;
                        break;
                    }
                }
            }

            if (!isLocalMaximum) {
                continue;
            }

            for (int py = -1; py <= 1; ++py) {
                for (int px = -1; px <= 1; ++px) {
                    const int yy = std::clamp(y + py, 0, height - 1);
                    const int xx = std::clamp(x + px, 0, width - 1);
                    result.setPixelColor(xx, yy, QColor(255, 0, 0));
                }
            }
        }
    }

    return result;
}

void HarrisHandler::applyHarris(int blockSize, int sobelKernelSize, double k, int threshold)
{
    if (ctx.currentImage == nullptr || ctx.currentImage->isNull()) {
        QMessageBox::warning(ctx.messageParent, "Uwaga", "Najpierw otwórz obraz!");
        return;
    }

    if (blockSize < 2) {
        blockSize = 2;
    }

    if (sobelKernelSize < 3) {
        sobelKernelSize = 3;
    }

    if (sobelKernelSize % 2 == 0) {
        --sobelKernelSize;
    }

    if (sobelKernelSize > 7) {
        sobelKernelSize = 7;
    }

    if (k <= 0.0) {
        k = 0.04;
    }

    threshold = clampToByte(threshold);

    if (ctx.pushHistory) {
        ctx.pushHistory();
    }

    const auto gray = imageToMatrix(*ctx.currentImage);
    const auto gx = convolveGray(gray, buildSobelKernelX(sobelKernelSize));
    const auto gy = convolveGray(gray, buildSobelKernelY(sobelKernelSize));
    const auto response = computeHarrisResponse(gx, gy, blockSize, k);

    const double maxVal = maxResponse(response);
    const double responseThreshold = maxVal * (static_cast<double>(threshold) / 255.0);

    const QImage result = drawCorners(*ctx.currentImage, response, responseThreshold);
    if (result.isNull()) {
        return;
    }

    *ctx.currentImage = result;

    if (ctx.updateImagePreview) {
        ctx.updateImagePreview();
    }

    if (ctx.refreshHistogramIfNeeded) {
        ctx.refreshHistogramIfNeeded();
    }
}