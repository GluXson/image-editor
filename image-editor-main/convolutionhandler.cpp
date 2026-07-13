#include "convolutionhandler.h"

#include <QColor>
#include <QMessageBox>
#include <cmath>
#include <algorithm>

ConvolutionHandler::ConvolutionHandler(const EditorContext &context, QObject *parent)
    : QObject(parent), ctx(context)
{
}

// Ogranicza wartość składowej koloru do poprawnego zakresu 0-255.
int ConvolutionHandler::clampToByte(int value)
{
    return std::max(0, std::min(255, value));
}

// Zapewnia obraz w formacie RGB32, wygodnym do odczytu i zapisu pikseli.
QImage ConvolutionHandler::ensureRgbImage(const QImage &img)
{
    if (img.format() == QImage::Format_RGB32 ||
        img.format() == QImage::Format_ARGB32 ||
        img.format() == QImage::Format_ARGB32_Premultiplied) {
        return img;
    }

    return img.convertToFormat(QImage::Format_RGB32);
}

// Buduje maskę splotową na podstawie wybranego filtra i parametrów z panelu.
std::vector<std::vector<double>> ConvolutionHandler::buildKernel(const QString &filterName, int kernelSize, double sigma, bool normalize) const
{
    std::vector<std::vector<double>> kernel(kernelSize, std::vector<double>(kernelSize, 0.0));

    // Filtr uśredniający - każdy element maski ma taką samą wagę.
    if (filterName == "Rozmycie pudełkowe") {
        const double value = 1.0 / (kernelSize * kernelSize);
        for (int y = 0; y < kernelSize; ++y)
            for (int x = 0; x < kernelSize; ++x)
                kernel[y][x] = value;
    }
    // Filtr Gaussa - wagi zależą od odległości od środka maski.
    else if (filterName == "Rozmycie Gaussa") {
        const int radius = kernelSize / 2;
        double sum = 0.0;

        for (int y = -radius; y <= radius; ++y) {
            for (int x = -radius; x <= radius; ++x) {
                const double value = std::exp(-(x * x + y * y) / (2.0 * sigma * sigma));
                kernel[y + radius][x + radius] = value;
                sum += value;
            }
        }

        // Opcjonalna normalizacja zapewnia sumę wag równą 1.
        if (normalize && sum != 0.0) {
            for (int y = 0; y < kernelSize; ++y)
                for (int x = 0; x < kernelSize; ++x)
                    kernel[y][x] /= sum;
        }
    }
    // Maska wyostrzająca - wzmacnia środkowy piksel względem sąsiadów.
    else if (filterName == "Wyostrzanie") {
        kernel = {
            { 0, -1,  0 },
            {-1,  5, -1 },
            { 0, -1,  0 }
        };
    }
    // Maska do wykrywania krawędzi - podkreśla gwałtowne zmiany jasności.
    else if (filterName == "Wykrywanie krawędzi") {
        kernel = {
            {-1, -1, -1},
            {-1,  8, -1},
            {-1, -1, -1}
        };
    }
    // Operator Sobela - wykrywa gradient obrazu w poziomie.
    else if (filterName == "Sobel") {
        kernel = {
            {-1, 0, 1},
            {-2, 0, 2},
            {-1, 0, 1}
        };
    }

    return kernel;
}

// Wykonuje splot całego obrazu z przekazaną maską.
QImage ConvolutionHandler::convolveImage(const QImage &img, const std::vector<std::vector<double>> &kernel) const
{
    QImage src = ensureRgbImage(img);
    QImage result(src.size(), QImage::Format_RGB32);

    const int kernelSize = static_cast<int>(kernel.size());
    const int radius = kernelSize / 2;

    // Przechodzi po każdym pikselu obrazu wynikowego.
    for (int y = 0; y < src.height(); ++y) {
        for (int x = 0; x < src.width(); ++x) {
            double r = 0.0;
            double g = 0.0;
            double b = 0.0;

            // Nakłada maskę na otoczenie bieżącego piksela.
            for (int ky = 0; ky < kernelSize; ++ky) {
                for (int kx = 0; kx < kernelSize; ++kx) {
                    // Zabezpieczenie brzegów obrazu przez przycinanie współrzędnych.
                    const int ix = std::clamp(x + kx - radius, 0, src.width() - 1);
                    const int iy = std::clamp(y + ky - radius, 0, src.height() - 1);

                    const QColor c = src.pixelColor(ix, iy);
                    const double w = kernel[ky][kx];

                    r += c.red() * w;
                    g += c.green() * w;
                    b += c.blue() * w;
                }
            }

            // Zapisuje nowy piksel po zaokrągleniu i ograniczeniu do zakresu 0-255.
            result.setPixelColor(x, y, QColor(
                                           clampToByte(static_cast<int>(std::round(r))),
                                           clampToByte(static_cast<int>(std::round(g))),
                                           clampToByte(static_cast<int>(std::round(b)))));
        }
    }

    return result;
}

// Główna funkcja panelu - buduje maskę, wykonuje splot i odświeża widok.
void ConvolutionHandler::applyConvolution(const QString &filterName, int kernelSize, double sigma, bool normalize)
{
    // Sprawdza, czy użytkownik ma aktualnie załadowany obraz.
    if (ctx.currentImage == nullptr || ctx.currentImage->isNull()) {
        QMessageBox::warning(ctx.messageParent, "Uwaga", "Najpierw otwórz obraz!");
        return;
    }

    // Zapisuje stan przed operacją, aby możliwe było cofnięcie.
    if (ctx.pushHistory) ctx.pushHistory();

    // Tworzy odpowiednią maskę i stosuje ją do obrazu.
    const auto kernel = buildKernel(filterName, kernelSize, sigma, normalize);
    *ctx.currentImage = convolveImage(*ctx.currentImage, kernel);

    // Odświeża podgląd obrazu i histogram po wykonaniu operacji.
    if (ctx.updateImagePreview) ctx.updateImagePreview();
    if (ctx.refreshHistogramIfNeeded) ctx.refreshHistogramIfNeeded();
}