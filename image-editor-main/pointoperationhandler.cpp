#include "pointoperationhandler.h"

#include "netpbm.h"
#include "pointoperationpage.h"

#include <QColor>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <cmath>
#include <algorithm>

PointOperationHandler::PointOperationHandler(const EditorContext& context, QObject* parent)
    : QObject(parent)
    , ctx(context)
{
}

// Ogranicza wartość kanału koloru do poprawnego zakresu 0-255.
int PointOperationHandler::clampToByte(int value)
{
    return std::max(0, std::min(255, value));
}

// Upewnia się, że obraz jest w formacie RGB, aby operacje na pikselach były spójne.
QImage PointOperationHandler::ensureRgbImage(const QImage& img)
{
    if (img.format() == QImage::Format_RGB32 ||
        img.format() == QImage::Format_ARGB32 ||
        img.format() == QImage::Format_ARGB32_Premultiplied) {
        return img;
    }

    return img.convertToFormat(QImage::Format_RGB32);
}

// Sprawdza, czy aktualny obraz istnieje przed wykonaniem operacji.
bool PointOperationHandler::ensureCurrentImage(const QString& message) const
{
    if (ctx.currentImage == nullptr || ctx.currentImage->isNull()) {
        QMessageBox::warning(ctx.messageParent, "Uwaga", message);
        return false;
    }

    return true;
}

// Sprawdza, czy dostępne są dwa obrazy potrzebne do operacji dwuobrazowych.
bool PointOperationHandler::ensureTwoImages() const
{
    if (!ensureCurrentImage(QStringLiteral("Najpierw otwórz pierwszy obraz!"))) {
        return false;
    }

    if (ctx.secondImage == nullptr || ctx.secondImage->isNull()) {
        QMessageBox::warning(ctx.messageParent, "Uwaga", "Najpierw zaimportuj drugi obraz.");
        return false;
    }

    return true;
}

// Wczytuje drugi obraz z dysku i zapisuje go w kontekście edytora.
void PointOperationHandler::importSecondImage()
{
    if (ctx.secondImage == nullptr || ctx.secondImagePath == nullptr) {
        return;
    }

    const QString path = QFileDialog::getOpenFileName(
        ctx.messageParent,
        "Importuj drugi obraz",
        QString(),
        "Obrazy Netpbm (*.pbm *.pgm *.ppm)");

    if (path.isEmpty()) {
        return;
    }

    try {
        *ctx.secondImage = loadNetpbm(path);
        *ctx.secondImagePath = path;

        // Aktualizuje nazwę drugiego pliku widoczną w panelu.
        if (ctx.pointOperationPage != nullptr) {
            ctx.pointOperationPage->setSecondImageName(QFileInfo(path).fileName());
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(ctx.messageParent, "Błąd", e.what());
    }
}

// Przywraca poprzedni stan obrazu z historii operacji.
void PointOperationHandler::undoLastOperation()
{
    if (ctx.imageHistory == nullptr || ctx.currentImage == nullptr) {
        return;
    }

    if (ctx.imageHistory->empty()) {
        QMessageBox::information(ctx.messageParent, "Cofnij", "Brak operacji do cofnięcia.");
        return;
    }

    *ctx.currentImage = ctx.imageHistory->back();
    ctx.imageHistory->pop_back();

    if (ctx.updateImagePreview) {
        ctx.updateImagePreview();
    }

    if (ctx.refreshHistogramIfNeeded) {
        ctx.refreshHistogramIfNeeded();
    }
}

// Zamienia obraz kolorowy na skalę szarości na podstawie ważonej sumy RGB.
void PointOperationHandler::applyDesaturation()
{
    if (!ensureCurrentImage()) {
        return;
    }

    if (ctx.pushHistory) {
        ctx.pushHistory();
    }

    QImage img = ensureRgbImage(*ctx.currentImage);

    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            const QColor color = img.pixelColor(x, y);

            // Liczy jasność piksela według standardowych wag dla RGB.
            const int gray = clampToByte(static_cast<int>(
                0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue()));

            img.setPixelColor(x, y, QColor(gray, gray, gray));
        }
    }

    *ctx.currentImage = img;

    if (ctx.updateImagePreview) {
        ctx.updateImagePreview();
    }

    if (ctx.refreshHistogramIfNeeded) {
        ctx.refreshHistogramIfNeeded();
    }
}

// Tworzy negatyw przez odwrócenie każdego kanału koloru względem 255.
void PointOperationHandler::applyNegative()
{
    if (!ensureCurrentImage()) {
        return;
    }

    if (ctx.pushHistory) {
        ctx.pushHistory();
    }

    QImage img = ensureRgbImage(*ctx.currentImage);

    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            const QColor color = img.pixelColor(x, y);
            img.setPixelColor(x, y, QColor(
                                        255 - color.red(),
                                        255 - color.green(),
                                        255 - color.blue()));
        }
    }

    *ctx.currentImage = img;

    if (ctx.updateImagePreview) {
        ctx.updateImagePreview();
    }

    if (ctx.refreshHistogramIfNeeded) {
        ctx.refreshHistogramIfNeeded();
    }
}

// Zwiększa lub zmniejsza jasność przez dodanie tej samej wartości do kanałów RGB.
void PointOperationHandler::applyBrightness(int value)
{
    if (!ensureCurrentImage()) {
        return;
    }

    if (ctx.pushHistory) {
        ctx.pushHistory();
    }

    QImage img = ensureRgbImage(*ctx.currentImage);

    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            const QColor color = img.pixelColor(x, y);
            img.setPixelColor(x, y, QColor(
                                        clampToByte(color.red() + value),
                                        clampToByte(color.green() + value),
                                        clampToByte(color.blue() + value)));
        }
    }

    *ctx.currentImage = img;

    if (ctx.updateImagePreview) {
        ctx.updateImagePreview();
    }

    if (ctx.refreshHistogramIfNeeded) {
        ctx.refreshHistogramIfNeeded();
    }
}

// Zmienia nasycenie w przestrzeni HSV bez modyfikowania odcienia i jasności.
void PointOperationHandler::applySaturation(int value)
{
    if (!ensureCurrentImage()) {
        return;
    }

    if (ctx.pushHistory) {
        ctx.pushHistory();
    }

    QImage img = ensureRgbImage(*ctx.currentImage);

    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            QColor hsv = img.pixelColor(x, y).toHsv();
            int h, s, v, a;
            hsv.getHsv(&h, &s, &v, &a);

            // Modyfikuje tylko składową nasycenia.
            s = clampToByte(s + value);

            QColor result;
            result.setHsv(h, s, v, a);
            img.setPixelColor(x, y, result.toRgb());
        }
    }

    *ctx.currentImage = img;

    if (ctx.updateImagePreview) {
        ctx.updateImagePreview();
    }

    if (ctx.refreshHistogramIfNeeded) {
        ctx.refreshHistogramIfNeeded();
    }
}

// Zmienia kontrast liniowo względem środka zakresu jasności, czyli 128.
void PointOperationHandler::applyLinearContrast(double factor)
{
    if (!ensureCurrentImage()) {
        return;
    }

    if (ctx.pushHistory) {
        ctx.pushHistory();
    }

    QImage img = ensureRgbImage(*ctx.currentImage);

    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            const QColor color = img.pixelColor(x, y);

            const int r = clampToByte(static_cast<int>(factor * (color.red() - 128) + 128));
            const int g = clampToByte(static_cast<int>(factor * (color.green() - 128) + 128));
            const int b = clampToByte(static_cast<int>(factor * (color.blue() - 128) + 128));

            img.setPixelColor(x, y, QColor(r, g, b));
        }
    }

    *ctx.currentImage = img;

    if (ctx.updateImagePreview) {
        ctx.updateImagePreview();
    }

    if (ctx.refreshHistogramIfNeeded) {
        ctx.refreshHistogramIfNeeded();
    }
}

// Wzmacnia ciemniejsze partie obrazu przez logarytmiczne przekształcenie jasności.
void PointOperationHandler::applyLogarithmicContrast(double c)
{
    if (!ensureCurrentImage()) {
        return;
    }

    if (ctx.pushHistory) {
        ctx.pushHistory();
    }

    // Przekształca pojedynczą wartość kanału funkcją logarytmiczną.
    auto logTransform = [c](int value) -> int {
        const double normalized = value / 255.0;
        const double result = c * std::log(1.0 + normalized);
        return clampToByte(static_cast<int>((result / std::log(2.0)) * 255.0));
    };

    QImage img = ensureRgbImage(*ctx.currentImage);

    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            const QColor color = img.pixelColor(x, y);
            img.setPixelColor(x, y, QColor(
                                        logTransform(color.red()),
                                        logTransform(color.green()),
                                        logTransform(color.blue())));
        }
    }

    *ctx.currentImage = img;

    if (ctx.updateImagePreview) {
        ctx.updateImagePreview();
    }

    if (ctx.refreshHistogramIfNeeded) {
        ctx.refreshHistogramIfNeeded();
    }
}

// Zmienia kontrast funkcją potęgową gamma, wpływając głównie na rozkład jasności.
void PointOperationHandler::applyPowerContrast(double gamma)
{
    if (!ensureCurrentImage()) {
        return;
    }

    if (ctx.pushHistory) {
        ctx.pushHistory();
    }

    // Przekształca pojedynczą wartość kanału według korekcji gamma.
    auto powerTransform = [gamma](int value) -> int {
        const double normalized = value / 255.0;
        const double result = std::pow(normalized, gamma);
        return clampToByte(static_cast<int>(result * 255.0));
    };

    QImage img = ensureRgbImage(*ctx.currentImage);

    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            const QColor color = img.pixelColor(x, y);
            img.setPixelColor(x, y, QColor(
                                        powerTransform(color.red()),
                                        powerTransform(color.green()),
                                        powerTransform(color.blue())));
        }
    }

    *ctx.currentImage = img;

    if (ctx.updateImagePreview) {
        ctx.updateImagePreview();
    }

    if (ctx.refreshHistogramIfNeeded) {
        ctx.refreshHistogramIfNeeded();
    }
}

// Zamienia obraz na skalę szarości, a potem stosuje przekształcenie liniowe a * gray + b.
void PointOperationHandler::applyMonochromeTransform(double a, double b)
{
    if (!ensureCurrentImage()) {
        return;
    }

    if (ctx.pushHistory) {
        ctx.pushHistory();
    }

    QImage img = ensureRgbImage(*ctx.currentImage);

    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            const QColor color = img.pixelColor(x, y);

            // Najpierw wyznacza poziom szarości, potem przekształca go liniowo.
            const int gray = clampToByte(static_cast<int>(
                0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue()));
            const int transformed = clampToByte(static_cast<int>(a * gray + b));

            img.setPixelColor(x, y, QColor(transformed, transformed, transformed));
        }
    }

    *ctx.currentImage = img;

    if (ctx.updateImagePreview) {
        ctx.updateImagePreview();
    }

    if (ctx.refreshHistogramIfNeeded) {
        ctx.refreshHistogramIfNeeded();
    }
}

// Dodaje do siebie dwa obrazy piksel po pikselu z obcięciem wyniku do zakresu 0-255.
void PointOperationHandler::applyImageSum()
{
    if (!ensureTwoImages()) {
        return;
    }

    QImage first = ensureRgbImage(*ctx.currentImage);
    QImage second = ensureRgbImage(*ctx.secondImage);

    // Operacja jest wykonywana tylko dla obrazów o tym samym rozmiarze.
    if (!ctx.ensureSameSize || !ctx.ensureSameSize(first, second)) {
        return;
    }

    if (ctx.pushHistory) {
        ctx.pushHistory();
    }

    for (int y = 0; y < first.height(); ++y) {
        for (int x = 0; x < first.width(); ++x) {
            const QColor c1 = first.pixelColor(x, y);
            const QColor c2 = second.pixelColor(x, y);

            first.setPixelColor(x, y, QColor(
                                          clampToByte(c1.red() + c2.red()),
                                          clampToByte(c1.green() + c2.green()),
                                          clampToByte(c1.blue() + c2.blue())));
        }
    }

    *ctx.currentImage = first;

    if (ctx.updateImagePreview) {
        ctx.updateImagePreview();
    }

    if (ctx.refreshHistogramIfNeeded) {
        ctx.refreshHistogramIfNeeded();
    }
}

// Liczy różnicę dwóch obrazów piksel po pikselu jako wartość bezwzględną kanałów.
void PointOperationHandler::applyImageDifference()
{
    if (!ensureTwoImages()) {
        return;
    }

    QImage first = ensureRgbImage(*ctx.currentImage);
    QImage second = ensureRgbImage(*ctx.secondImage);

    if (!ctx.ensureSameSize || !ctx.ensureSameSize(first, second)) {
        return;
    }

    if (ctx.pushHistory) {
        ctx.pushHistory();
    }

    for (int y = 0; y < first.height(); ++y) {
        for (int x = 0; x < first.width(); ++x) {
            const QColor c1 = first.pixelColor(x, y);
            const QColor c2 = second.pixelColor(x, y);

            first.setPixelColor(x, y, QColor(
                                          std::abs(c1.red() - c2.red()),
                                          std::abs(c1.green() - c2.green()),
                                          std::abs(c1.blue() - c2.blue())));
        }
    }

    *ctx.currentImage = first;

    if (ctx.updateImagePreview) {
        ctx.updateImagePreview();
    }

    if (ctx.refreshHistogramIfNeeded) {
        ctx.refreshHistogramIfNeeded();
    }
}

// Mnoży dwa obrazy kanał po kanale i normalizuje wynik przez podzielenie przez 255.
void PointOperationHandler::applyImageMultiply()
{
    if (!ensureTwoImages()) {
        return;
    }

    QImage first = ensureRgbImage(*ctx.currentImage);
    QImage second = ensureRgbImage(*ctx.secondImage);

    if (!ctx.ensureSameSize || !ctx.ensureSameSize(first, second)) {
        return;
    }

    if (ctx.pushHistory) {
        ctx.pushHistory();
    }

    for (int y = 0; y < first.height(); ++y) {
        for (int x = 0; x < first.width(); ++x) {
            const QColor c1 = first.pixelColor(x, y);
            const QColor c2 = second.pixelColor(x, y);

            const int r = clampToByte(c1.red() * c2.red() / 255);
            const int g = clampToByte(c1.green() * c2.green() / 255);
            const int b = clampToByte(c1.blue() * c2.blue() / 255);

            first.setPixelColor(x, y, QColor(r, g, b));
        }
    }

    *ctx.currentImage = first;

    if (ctx.updateImagePreview) {
        ctx.updateImagePreview();
    }

    if (ctx.refreshHistogramIfNeeded) {
        ctx.refreshHistogramIfNeeded();
    }
}