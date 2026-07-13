#include "binarizationhandler.h"

#include "binarizationpage.h"

#include <QColor>
#include <QImage>
#include <QMessageBox>
#include <QtGlobal>

BinarizationHandler::BinarizationHandler(const EditorContext &context,
                                         BinarizationPage *page,
                                         QObject *parent)
    : QObject(parent)
    , ctx(context)
    , page(page)
{
}

// Sprawdza, czy aktualnie jest wczytany poprawny obraz.
// Jeśli nie, pokazuje komunikat ostrzegawczy i zwraca false.
bool BinarizationHandler::ensureImageAvailable() const
{
    if (!ctx.currentImage || ctx.currentImage->isNull()) {
        if (ctx.messageParent) {
            QMessageBox::warning(ctx.messageParent,
                                 "Brak obrazu",
                                 "Najpierw otwórz obraz.");
        }
        return false;
    }

    return true;
}

// Konwertuje dowolny obraz wejściowy do 8-bitowej skali szarości.
// Jasność piksela wyznaczana jest na podstawie składowych RGB.
QImage BinarizationHandler::toGrayscaleImage(const QImage &src)
{
    if (src.isNull()) {
        return {};
    }

    QImage rgb = src.convertToFormat(QImage::Format_RGB32);
    QImage gray(rgb.size(), QImage::Format_Grayscale8);

    for (int y = 0; y < rgb.height(); ++y) {
        for (int x = 0; x < rgb.width(); ++x) {
            const QColor c(rgb.pixel(x, y));
            const int g = qGray(c.red(), c.green(), c.blue());
            gray.setPixel(x, y, g);
        }
    }

    return gray;
}

// Oblicza histogram jasności dla obrazu w skali szarości.
// Każdy element tablicy odpowiada liczbie pikseli o danej jasności 0-255.
std::array<int, 256> BinarizationHandler::computeGrayHistogram(const QImage &gray)
{
    std::array<int, 256> hist{};
    hist.fill(0);

    if (gray.isNull()) {
        return hist;
    }

    QImage img = gray.convertToFormat(QImage::Format_Grayscale8);

    for (int y = 0; y < img.height(); ++y) {
        const uchar *line = img.constScanLine(y);
        for (int x = 0; x < img.width(); ++x) {
            ++hist[line[x]];
        }
    }

    return hist;
}

// Tworzy obraz binarny na podstawie obrazu w skali szarości i progu.
// Piksele >= threshold dostają wartość 255, w przeciwnym razie 0.
QImage BinarizationHandler::makeBinaryImage(const QImage &gray, int threshold)
{
    QImage src = gray.convertToFormat(QImage::Format_Grayscale8);
    QImage out(src.size(), QImage::Format_Grayscale8);

    for (int y = 0; y < src.height(); ++y) {
        const uchar *srcLine = src.constScanLine(y);
        uchar *dstLine = out.scanLine(y);

        for (int x = 0; x < src.width(); ++x) {
            dstLine[x] = (srcLine[x] >= threshold) ? 255 : 0;
        }
    }

    return out;
}

// Wyznacza próg binaryzacji metodą Otsu.
// Algorytm szuka takiego progu, dla którego wariancja między klasami
// tła i obiektu jest maksymalna.
int BinarizationHandler::computeOtsuThreshold(const std::array<int, 256> &hist, int totalPixels)
{
    if (totalPixels <= 0) {
        return 0;
    }

    double sum = 0.0;
    for (int i = 0; i < 256; ++i) {
        sum += i * hist[i];
    }

    double sumBackground = 0.0;
    int weightBackground = 0;
    double maxVariance = -1.0;
    int bestThreshold = 0;

    for (int t = 0; t < 256; ++t) {
        weightBackground += hist[t];
        if (weightBackground == 0) {
            continue;
        }

        const int weightForeground = totalPixels - weightBackground;
        if (weightForeground == 0) {
            break;
        }

        sumBackground += t * hist[t];

        const double meanBackground = sumBackground / weightBackground;
        const double meanForeground = (sum - sumBackground) / weightForeground;
        const double diff = meanBackground - meanForeground;

        const double betweenClassVariance =
            static_cast<double>(weightBackground) *
            static_cast<double>(weightForeground) *
            diff * diff;

        if (betweenClassVariance > maxVariance) {
            maxVariance = betweenClassVariance;
            bestThreshold = t;
        }
    }

    return bestThreshold + 1;
}

// Odświeża stan panelu binaryzacji:
// ustawia informację o dostępności obrazu, histogram i podgląd progu Otsu.
void BinarizationHandler::refreshPage()
{
    if (!page || !ctx.currentImage) {
        return;
    }

    const bool loaded = !ctx.currentImage->isNull();
    page->setImageLoaded(loaded);

    if (!loaded) {
        page->setOtsuPreview(-1);
        page->updateHistogram(std::array<int, 256>{});
        return;
    }

    const QImage gray = toGrayscaleImage(*ctx.currentImage);
    const auto hist = computeGrayHistogram(gray);
    const int otsu = computeOtsuThreshold(hist, gray.width() * gray.height());

    page->updateHistogram(hist);
    page->setOtsuPreview(otsu);
}

// Wykonuje binaryzację ręczną na podstawie progu podanego przez użytkownika.
// Po przetworzeniu aktualizuje historię, podgląd obrazu i panel.
void BinarizationHandler::applyManualThreshold(int threshold)
{
    if (!ensureImageAvailable()) {
        return;
    }

    const QImage gray = toGrayscaleImage(*ctx.currentImage);
    const QImage result = makeBinaryImage(gray, threshold);

    if (result.isNull()) {
        return;
    }

    if (ctx.pushHistory) {
        ctx.pushHistory();
    }

    *ctx.currentImage = result;

    if (ctx.updateImagePreview) {
        ctx.updateImagePreview();
    }

    if (ctx.refreshHistogramIfNeeded) {
        ctx.refreshHistogramIfNeeded();
    }

    refreshPage();
}

// Wyznacza próg metodą Otsu, wykonuje binaryzację i aktualizuje UI.
// Dodatkowo ustawia wyliczony próg także w kontrolkach panelu.
void BinarizationHandler::applyOtsuThreshold()
{
    if (!ensureImageAvailable()) {
        return;
    }

    const QImage gray = toGrayscaleImage(*ctx.currentImage);
    const auto hist = computeGrayHistogram(gray);
    const int otsu = computeOtsuThreshold(hist, gray.width() * gray.height());
    const QImage result = makeBinaryImage(gray, otsu);

    if (result.isNull()) {
        return;
    }

    if (ctx.pushHistory) {
        ctx.pushHistory();
    }

    *ctx.currentImage = result;

    if (ctx.updateImagePreview) {
        ctx.updateImagePreview();
    }

    if (ctx.refreshHistogramIfNeeded) {
        ctx.refreshHistogramIfNeeded();
    }

    if (page) {
        page->setThreshold(otsu);
        page->setOtsuPreview(otsu);
    }

    refreshPage();
}