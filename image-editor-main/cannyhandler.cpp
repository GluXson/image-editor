#include "cannyhandler.h"

#include <QColor>
#include <QMessageBox>

#include <algorithm>
#include <cmath>
#include <queue>

CannyHandler::CannyHandler(const EditorContext &context, QObject *parent)
    : QObject(parent)
    , ctx(context)
{
}

// Ogranicza wartość do zakresu 0-255
int CannyHandler::clampToByte(int value)
{
    return std::max(0, std::min(255, value));
}

// Zapewnia zgodny format obrazu do dalszego przetwarzania
QImage CannyHandler::ensureRgbImage(const QImage &img)
{
    if (img.format() == QImage::Format_RGB32 ||
        img.format() == QImage::Format_ARGB32 ||
        img.format() == QImage::Format_ARGB32_Premultiplied) {
        return img;
    }

    return img.convertToFormat(QImage::Format_RGB32);
}

// Konwersja obrazu do skali szarości według wag RGB
QImage CannyHandler::toGrayscaleImage(const QImage &img)
{
    QImage rgb = ensureRgbImage(img);
    QImage gray(rgb.size(), QImage::Format_RGB32);

    for (int y = 0; y < rgb.height(); ++y) {
        for (int x = 0; x < rgb.width(); ++x) {
            const QColor c = rgb.pixelColor(x, y);
            const int g = clampToByte(static_cast<int>(std::round(
                0.299 * c.red() + 0.587 * c.green() + 0.114 * c.blue()
                )));
            gray.setPixelColor(x, y, QColor(g, g, g));
        }
    }

    return gray;
}

// Zamiana obrazu grayscale na macierz wartości typu double
std::vector<std::vector<double>> CannyHandler::imageToMatrix(const QImage &img)
{
    QImage gray = toGrayscaleImage(img);
    std::vector<std::vector<double>> matrix(
        gray.height(), std::vector<double>(gray.width(), 0.0));

    for (int y = 0; y < gray.height(); ++y) {
        for (int x = 0; x < gray.width(); ++x) {
            matrix[y][x] = gray.pixelColor(x, y).red();
        }
    }

    return matrix;
}

// Zamiana macierzy intensywności na obraz grayscale
QImage CannyHandler::matrixToImage(const std::vector<std::vector<double>> &matrix)
{
    if (matrix.empty() || matrix[0].empty()) {
        return {};
    }

    const int height = static_cast<int>(matrix.size());
    const int width = static_cast<int>(matrix[0].size());

    QImage img(width, height, QImage::Format_RGB32);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int value = clampToByte(static_cast<int>(std::round(matrix[y][x])));
            img.setPixelColor(x, y, QColor(value, value, value));
        }
    }

    return img;
}

// Zamiana binarnej macierzy krawędzi na obraz czarno-biały
QImage CannyHandler::binaryMatrixToImage(const std::vector<std::vector<int>> &matrix)
{
    if (matrix.empty() || matrix[0].empty()) {
        return {};
    }

    const int height = static_cast<int>(matrix.size());
    const int width = static_cast<int>(matrix[0].size());

    QImage img(width, height, QImage::Format_RGB32);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int value = matrix[y][x] > 0 ? 255 : 0;
            img.setPixelColor(x, y, QColor(value, value, value));
        }
    }

    return img;
}

// Buduje znormalizowaną maskę Gaussa o zadanym rozmiarze i sigma
std::vector<std::vector<double>> CannyHandler::buildGaussianKernel(int kernelSize, double sigma)
{
    std::vector<std::vector<double>> kernel(
        kernelSize, std::vector<double>(kernelSize, 0.0));

    const int radius = kernelSize / 2;
    double sum = 0.0;

    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            const double value = std::exp(-(x * x + y * y) / (2.0 * sigma * sigma));
            kernel[y + radius][x + radius] = value;
            sum += value;
        }
    }

    // Normalizacja maski tak, aby suma elementów wynosiła 1
    if (sum > 0.0) {
        for (int y = 0; y < kernelSize; ++y) {
            for (int x = 0; x < kernelSize; ++x) {
                kernel[y][x] /= sum;
            }
        }
    }

    return kernel;
}

// Zwraca maskę Sobela dla kierunku X
std::vector<std::vector<double>> CannyHandler::buildSobelKernelX()
{
    return {
        { -1.0, 0.0, 1.0 },
        { -2.0, 0.0, 2.0 },
        { -1.0, 0.0, 1.0 }
    };
}

// Zwraca maskę Sobela dla kierunku Y
std::vector<std::vector<double>> CannyHandler::buildSobelKernelY()
{
    return {
        { -1.0, -2.0, -1.0 },
        {  0.0,  0.0,  0.0 },
        {  1.0,  2.0,  1.0 }
    };
}

// Uniwersalny splot obrazu grayscale z podaną maską
std::vector<std::vector<double>> CannyHandler::convolveGray(
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
        height, std::vector<double>(width, 0.0));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double sum = 0.0;

            for (int ky = 0; ky < kernelSize; ++ky) {
                for (int kx = 0; kx < kernelSize; ++kx) {
                    // Replikacja brzegów przez clamp
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

// Oblicza moduł gradientu metodą L1 albo dokładniejszą L2
std::vector<std::vector<double>> CannyHandler::computeGradientMagnitude(
    const std::vector<std::vector<double>> &gx,
    const std::vector<std::vector<double>> &gy,
    bool useL2Gradient)
{
    const int height = static_cast<int>(gx.size());
    const int width = static_cast<int>(gx[0].size());

    std::vector<std::vector<double>> result(
        height, std::vector<double>(width, 0.0));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (useL2Gradient) {
                result[y][x] = std::sqrt(gx[y][x] * gx[y][x] + gy[y][x] * gy[y][x]);
            } else {
                result[y][x] = std::abs(gx[y][x]) + std::abs(gy[y][x]);
            }
        }
    }

    return result;
}

// Oblicza kierunek gradientu w stopniach z zakresu 0-180
std::vector<std::vector<double>> CannyHandler::computeGradientDirection(
    const std::vector<std::vector<double>> &gx,
    const std::vector<std::vector<double>> &gy)
{
    const int height = static_cast<int>(gx.size());
    const int width = static_cast<int>(gx[0].size());

    std::vector<std::vector<double>> result(
        height, std::vector<double>(width, 0.0));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            result[y][x] = std::atan2(gy[y][x], gx[y][x]) * 180.0 / M_PI;
            if (result[y][x] < 0.0) {
                result[y][x] += 180.0;
            }
        }
    }

    return result;
}

// Tłumi odpowiedzi, które nie są lokalnym maksimum w kierunku gradientu
std::vector<std::vector<double>> CannyHandler::nonMaximumSuppression(
    const std::vector<std::vector<double>> &magnitude,
    const std::vector<std::vector<double>> &direction)
{
    const int height = static_cast<int>(magnitude.size());
    const int width = static_cast<int>(magnitude[0].size());

    std::vector<std::vector<double>> result(
        height, std::vector<double>(width, 0.0));

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            const double angle = direction[y][x];
            const double current = magnitude[y][x];

            double q = 0.0;
            double r = 0.0;

            // Dobór sąsiadów do porównania zależnie od kierunku gradientu
            if ((angle >= 0.0 && angle < 22.5) || (angle >= 157.5 && angle <= 180.0)) {
                q = magnitude[y][x + 1];
                r = magnitude[y][x - 1];
            } else if (angle >= 22.5 && angle < 67.5) {
                q = magnitude[y + 1][x - 1];
                r = magnitude[y - 1][x + 1];
            } else if (angle >= 67.5 && angle < 112.5) {
                q = magnitude[y + 1][x];
                r = magnitude[y - 1][x];
            } else {
                q = magnitude[y - 1][x - 1];
                r = magnitude[y + 1][x + 1];
            }

            if (current >= q && current >= r) {
                result[y][x] = current;
            } else {
                result[y][x] = 0.0;
            }
        }
    }

    return result;
}

// Wykonuje progowanie z histerezą i śledzenie słabych krawędzi po połączeniu z silnymi
std::vector<std::vector<int>> CannyHandler::applyHysteresis(
    const std::vector<std::vector<double>> &suppressed,
    int lowThreshold,
    int highThreshold)
{
    const int height = static_cast<int>(suppressed.size());
    const int width = static_cast<int>(suppressed[0].size());

    std::vector<std::vector<int>> result(
        height, std::vector<int>(width, 0));

    std::queue<std::pair<int, int>> strongEdges;

    // Wstępna klasyfikacja pikseli: silne, słabe, tło
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const double value = suppressed[y][x];

            if (value >= highThreshold) {
                result[y][x] = 255;
                strongEdges.push({y, x});
            } else if (value >= lowThreshold) {
                result[y][x] = 128;
            }
        }
    }

    static const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    static const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

    // Rozszerzanie silnych krawędzi na połączone z nimi słabe krawędzie
    while (!strongEdges.empty()) {
        const auto [y, x] = strongEdges.front();
        strongEdges.pop();

        for (int i = 0; i < 8; ++i) {
            const int ny = y + dy[i];
            const int nx = x + dx[i];

            if (ny < 0 || ny >= height || nx < 0 || nx >= width) {
                continue;
            }

            if (result[ny][nx] == 128) {
                result[ny][nx] = 255;
                strongEdges.push({ny, nx});
            }
        }
    }

    // Na końcu zostawiamy tylko czarne tło i białe krawędzie
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            result[y][x] = (result[y][x] == 255) ? 255 : 0;
        }
    }

    return result;
}

// Główna funkcja wykonująca pełny pipeline algorytmu Canny'ego
void CannyHandler::applyCanny(int kernelSize,
                              double sigma,
                              int lowThreshold,
                              int highThreshold,
                              bool useL2Gradient)
{
    // Brak obrazu wejściowego - przerwij operację
    if (ctx.currentImage == nullptr || ctx.currentImage->isNull()) {
        QMessageBox::warning(ctx.messageParent, "Uwaga", "Najpierw otwórz obraz!");
        return;
    }

    // Minimalny rozmiar maski Gaussa
    if (kernelSize < 3) {
        kernelSize = 3;
    }

    // Rozmiar maski musi być nieparzysty
    if (kernelSize % 2 == 0) {
        ++kernelSize;
    }

    // Zabezpieczenie przed błędną sigmą
    if (sigma <= 0.0) {
        sigma = 1.0;
    }

    // Ograniczenie progów do zakresu 0-255
    lowThreshold = clampToByte(lowThreshold);
    highThreshold = clampToByte(highThreshold);

    // Korekta kolejności progów
    if (lowThreshold > highThreshold) {
        std::swap(lowThreshold, highThreshold);
    }

    // Zapis poprzedniego stanu obrazu do historii
    if (ctx.pushHistory) {
        ctx.pushHistory();
    }

    // 1. Konwersja obrazu do macierzy grayscale
    const auto gray = imageToMatrix(*ctx.currentImage);

    // 2. Rozmycie Gaussa
    const auto gaussian = buildGaussianKernel(kernelSize, sigma);
    const auto smoothed = convolveGray(gray, gaussian);

    // 3. Gradienty Sobela w poziomie i pionie
    const auto gx = convolveGray(smoothed, buildSobelKernelX());
    const auto gy = convolveGray(smoothed, buildSobelKernelY());

    // 4. Moduł gradientu i kierunek gradientu
    const auto magnitude = computeGradientMagnitude(gx, gy, useL2Gradient);
    const auto direction = computeGradientDirection(gx, gy);

    // 5. Pocienianie krawędzi
    const auto suppressed = nonMaximumSuppression(magnitude, direction);

    // 6. Progowanie z histerezą
    const auto edges = applyHysteresis(suppressed, lowThreshold, highThreshold);

    // 7. Zamiana wyniku na obraz binarny
    *ctx.currentImage = binaryMatrixToImage(edges);

    // Odświeżenie podglądu obrazu
    if (ctx.updateImagePreview) {
        ctx.updateImagePreview();
    }

    // Odświeżenie histogramu, jeśli panel tego wymaga
    if (ctx.refreshHistogramIfNeeded) {
        ctx.refreshHistogramIfNeeded();
    }
}