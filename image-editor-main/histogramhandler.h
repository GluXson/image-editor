#pragma once

#include <QObject>
#include <QImage>
#include <array>

#include "editorcontext.h"

class HistogramHandler : public QObject
{
    Q_OBJECT

public:
    explicit HistogramHandler(const EditorContext &context, QObject *parent = nullptr);

public slots:
    void refreshHistogramIfNeeded();
    void applyHistogramStretch();
    void applyHistogramEqualization();

private:
    EditorContext ctx;

    static int clampToByte(int value);
    static QImage ensureRgbImage(const QImage &img);

    std::array<int, 256> computeHistogramGray(const QImage &img) const;
    std::array<int, 256> computeHistogramChannel(const QImage &img, int channel) const;
};