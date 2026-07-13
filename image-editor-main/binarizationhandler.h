#pragma once

#include <QObject>
#include <QImage>
#include <array>

#include "editorcontext.h"

class BinarizationPage;

class BinarizationHandler : public QObject
{
    Q_OBJECT

public:
    explicit BinarizationHandler(const EditorContext &context,
                                 BinarizationPage *page,
                                 QObject *parent = nullptr);

public slots:
    void refreshPage();
    void applyManualThreshold(int threshold);
    void applyOtsuThreshold();

private:
    EditorContext ctx;
    BinarizationPage *page = nullptr;

    static QImage toGrayscaleImage(const QImage &src);
    static std::array<int, 256> computeGrayHistogram(const QImage &gray);
    static QImage makeBinaryImage(const QImage &gray, int threshold);
    static int computeOtsuThreshold(const std::array<int, 256> &hist, int totalPixels);

    bool ensureImageAvailable() const;
};