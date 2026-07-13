#pragma once

#include <QImage>
#include <QString>
#include <QWidget>

#include <functional>
#include <vector>

class PointOperationPage;
class HistogramPage;
class BinarizationPage;
class ConvolutionPage;
class CannyPage;
class HarrisPage;

struct EditorContext
{
    PointOperationPage *pointOperationPage = nullptr;
    HistogramPage *histogramPage = nullptr;
    BinarizationPage *binarizationPage = nullptr;
    ConvolutionPage *convolutionPage = nullptr;
    CannyPage *cannyPage = nullptr;
    HarrisPage *harrisPage = nullptr;

    QImage *currentImage = nullptr;
    QImage *secondImage = nullptr;
    QString *secondImagePath = nullptr;
    std::vector<QImage> *imageHistory = nullptr;

    QWidget *messageParent = nullptr;

    std::function<void()> updateImagePreview;
    std::function<void()> refreshHistogramIfNeeded;
    std::function<void()> pushHistory;
    std::function<void()> undoLastOperation;
    std::function<bool(const QImage &, const QImage &)> ensureSameSize;
};