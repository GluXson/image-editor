#pragma once

#include <QObject>
#include <QImage>
#include "editorcontext.h"

class PointOperationHandler : public QObject
{
    Q_OBJECT

public:
    explicit PointOperationHandler(const EditorContext &context, QObject *parent = nullptr);

public slots:
    void importSecondImage();
    void undoLastOperation();

    void applyDesaturation();
    void applyNegative();
    void applyBrightness(int value);
    void applySaturation(int value);
    void applyLinearContrast(double factor);
    void applyLogarithmicContrast(double c);
    void applyPowerContrast(double gamma);
    void applyMonochromeTransform(double a, double b);

    void applyImageSum();
    void applyImageDifference();
    void applyImageMultiply();

private:
    EditorContext ctx;

    static int clampToByte(int value);
    static QImage ensureRgbImage(const QImage &img);
    bool ensureCurrentImage(const QString &message = QStringLiteral("Najpierw otwórz obraz!")) const;
    bool ensureTwoImages() const;
};