#include "histogramhandler.h"
#include "histogrampage.h"

#include <QColor>
#include <QMessageBox>
#include <QRgb>
#include <algorithm>

HistogramHandler::HistogramHandler(const EditorContext &context, QObject *parent)
    : QObject(parent), ctx(context)
{
}

// Ogranicza wartość do zakresu poprawnego dla składowej koloru 0-255.
int HistogramHandler::clampToByte(int value)
{
    return std::max(0, std::min(255, value));
}

// Zapewnia obraz w formacie wygodnym do szybkiego odczytu kanałów RGB.
QImage HistogramHandler::ensureRgbImage(const QImage &img)
{
    if (img.format() == QImage::Format_RGB32 ||
        img.format() == QImage::Format_ARGB32 ||
        img.format() == QImage::Format_ARGB32_Premultiplied) {
        return img;
    }

    return img.convertToFormat(QImage::Format_RGB32);
}

// Liczy histogram wskazanego kanału:
// 0 - czerwony, 1 - zielony, 2 - niebieski.
std::array<int, 256> HistogramHandler::computeHistogramChannel(const QImage &img, int channel) const
{
    std::array<int, 256> hist{};
    QImage rgb = ensureRgbImage(img);

    for (int y = 0; y < rgb.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(rgb.constScanLine(y));

        for (int x = 0; x < rgb.width(); ++x) {
            const QRgb px = line[x];
            int value = 0;

            if (channel == 0) value = qRed(px);
            else if (channel == 1) value = qGreen(px);
            else value = qBlue(px);

            hist[value]++;
        }
    }

    return hist;
}

// Liczy histogram jasności obrazu według luminancji:
// gray = 0.299 * R + 0.587 * G + 0.114 * B.
std::array<int, 256> HistogramHandler::computeHistogramGray(const QImage &img) const
{
    std::array<int, 256> hist{};
    QImage rgb = ensureRgbImage(img);

    for (int y = 0; y < rgb.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(rgb.constScanLine(y));

        for (int x = 0; x < rgb.width(); ++x) {
            const QRgb px = line[x];
            const int gray = clampToByte(static_cast<int>(
                0.299 * qRed(px) + 0.587 * qGreen(px) + 0.114 * qBlue(px)));

            hist[gray]++;
        }
    }

    return hist;
}

// Odświeża wykres histogramu tylko wtedy, gdy istnieje panel histogramu
// oraz aktualnie załadowany obraz.
void HistogramHandler::refreshHistogramIfNeeded()
{
    if (ctx.histogramPage == nullptr || ctx.currentImage == nullptr || ctx.currentImage->isNull()) {
        return;
    }

    ctx.histogramPage->updateHistogram(
        computeHistogramChannel(*ctx.currentImage, 0),
        computeHistogramChannel(*ctx.currentImage, 1),
        computeHistogramChannel(*ctx.currentImage, 2),
        computeHistogramGray(*ctx.currentImage));
}

// Rozciąga histogram każdego kanału RGB osobno do pełnego zakresu 0-255.
// Dzięki temu transformacja jest spójna z histogramami R/G/B pokazywanymi w panelu.
void HistogramHandler::applyHistogramStretch()
{
    if (ctx.currentImage == nullptr || ctx.currentImage->isNull()) {
        QMessageBox::warning(ctx.messageParent, "Uwaga", "Najpierw otwórz obraz.");
        return;
    }

    QImage img = ensureRgbImage(*ctx.currentImage);

    int minR = 255, minG = 255, minB = 255;
    int maxR = 0,   maxG = 0,   maxB = 0;

    // Szukanie minimalnych i maksymalnych wartości każdego kanału.
    for (int y = 0; y < img.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(img.constScanLine(y));

        for (int x = 0; x < img.width(); ++x) {
            const QRgb px = line[x];

            minR = std::min(minR, qRed(px));
            minG = std::min(minG, qGreen(px));
            minB = std::min(minB, qBlue(px));

            maxR = std::max(maxR, qRed(px));
            maxG = std::max(maxG, qGreen(px));
            maxB = std::max(maxB, qBlue(px));
        }
    }

    // Jeżeli każdy kanał ma stałą wartość, rozciąganie nie ma sensu.
    if (minR == maxR && minG == maxG && minB == maxB) {
        QMessageBox::information(ctx.messageParent, "Histogram",
                                 "Nie można rozciągnąć histogramu dla obrazu o stałej jasności.");
        return;
    }

    if (ctx.pushHistory) ctx.pushHistory();

    // Przeskalowanie kanałów do pełnego zakresu 0-255.
    for (int y = 0; y < img.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(img.scanLine(y));

        for (int x = 0; x < img.width(); ++x) {
            const QRgb px = line[x];

            const int r = (minR == maxR) ? qRed(px)
                                         : clampToByte((qRed(px)   - minR) * 255 / (maxR - minR));
            const int g = (minG == maxG) ? qGreen(px)
                                         : clampToByte((qGreen(px) - minG) * 255 / (maxG - minG));
            const int b = (minB == maxB) ? qBlue(px)
                                         : clampToByte((qBlue(px)  - minB) * 255 / (maxB - minB));

            line[x] = qRgb(r, g, b);
        }
    }

    *ctx.currentImage = img;

    if (ctx.updateImagePreview) ctx.updateImagePreview();
    refreshHistogramIfNeeded();
}

// Wyrównuje histogram na składowej jasności V w przestrzeni HSV.
// Hue i saturation pozostają bez zmian, więc efekt jest zwykle bardziej naturalny kolorystycznie.
void HistogramHandler::applyHistogramEqualization()
{
    if (ctx.currentImage == nullptr || ctx.currentImage->isNull()) {
        QMessageBox::warning(ctx.messageParent, "Uwaga", "Najpierw otwórz obraz.");
        return;
    }

    QImage img = ensureRgbImage(*ctx.currentImage);
    std::array<int, 256> hist{};

    // Budowanie histogramu dla składowej V.
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            QColor hsv = img.pixelColor(x, y).toHsv();
            int h, s, v, a;
            hsv.getHsv(&h, &s, &v, &a);
            hist[v]++;
        }
    }

    // Dystrybuanta histogramu.
    std::array<int, 256> cdf{};
    cdf[0] = hist[0];
    for (int i = 1; i < 256; ++i) {
        cdf[i] = cdf[i - 1] + hist[i];
    }

    const int totalPixels = img.width() * img.height();
    int cdfMin = 0;

    // Pierwsza niezerowa wartość dystrybuanty.
    for (int i = 0; i < 256; ++i) {
        if (cdf[i] != 0) {
            cdfMin = cdf[i];
            break;
        }
    }

    // Obraz o stałej jasności nie nadaje się do equalizacji.
    if (totalPixels == cdfMin) {
        QMessageBox::information(ctx.messageParent, "Histogram",
                                 "Nie można wyrównać histogramu dla obrazu o stałej jasności.");
        return;
    }

    if (ctx.pushHistory) ctx.pushHistory();

    // Mapowanie starej wartości V na nową na podstawie dystrybuanty.
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            QColor hsv = img.pixelColor(x, y).toHsv();
            int h, s, v, a;
            hsv.getHsv(&h, &s, &v, &a);

            const int newV = clampToByte(
                static_cast<int>((cdf[v] - cdfMin) * 255.0 / (totalPixels - cdfMin)));

            QColor result;
            result.setHsv(h, s, newV, a);
            img.setPixelColor(x, y, result.toRgb());
        }
    }

    *ctx.currentImage = img;

    if (ctx.updateImagePreview) ctx.updateImagePreview();
    refreshHistogramIfNeeded();
}