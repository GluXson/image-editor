#pragma once

#include <QObject>
#include <QImage>
#include <QString>
#include <vector>
#include "editorcontext.h"

class ConvolutionHandler : public QObject
{
    Q_OBJECT

public:
    explicit ConvolutionHandler(const EditorContext &context, QObject *parent = nullptr);

public slots:
    void applyConvolution(const QString &filterName, int kernelSize, double sigma, bool normalize);

private:
    EditorContext ctx;

    static int clampToByte(int value);
    static QImage ensureRgbImage(const QImage &img);
    std::vector<std::vector<double>> buildKernel(const QString &filterName, int kernelSize, double sigma, bool normalize) const;
    QImage convolveImage(const QImage &img, const std::vector<std::vector<double>> &kernel) const;
};