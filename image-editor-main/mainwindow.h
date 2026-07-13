#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QImage>
#include <QMainWindow>
#include <QString>
#include <vector>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class PointOperationPage;
class HistogramPage;
class ConvolutionPage;
class BinarizationPage;
class CannyPage;
class HarrisPage;

class PointOperationHandler;
class HistogramHandler;
class ConvolutionHandler;
class BinarizationHandler;
class CannyHandler;
class HarrisHandler;

class QResizeEvent;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void on_toolSelector_currentIndexChanged(int index);

    void on_actionOpenPBM_triggered();
    void on_actionOpenPGM_triggered();
    void on_actionOpenPPM_triggered();
    void on_actionOpenJPG_triggered();
    void on_actionOpenPNG_triggered();

    void on_actionSavePBM_triggered();
    void on_actionSavePGM_triggered();
    void on_actionSavePPM_triggered();
    void on_actionSaveJPG_triggered();
    void on_actionSavePNG_triggered();

private:
    void updateImagePreview();
    void openFile(const QString &filter);
    void saveFile(const QString &filter, bool ascii);
    void pushHistory();
    bool ensureSameSize(const QImage &img1, const QImage &img2);
    void refreshHistogramIfNeeded();

private:
    Ui::MainWindow *ui;

    QImage currentImage;
    QImage secondImage;
    QString secondImagePath;
    std::vector<QImage> imageHistory;

    PointOperationPage *pointOperationPage;
    HistogramPage *histogramPage;
    ConvolutionPage *convolutionPage;
    BinarizationPage *binarizationPage;
    CannyPage *cannyPage;
    HarrisPage *harrisPage;

    PointOperationHandler *pointOperationHandler;
    HistogramHandler *histogramHandler;
    ConvolutionHandler *convolutionHandler;
    BinarizationHandler *binarizationHandler;
    CannyHandler *cannyHandler;
    HarrisHandler *harrisHandler;
};

#endif // MAINWINDOW_H