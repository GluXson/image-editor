#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "netpbm.h"
#include "pointoperationpage.h"
#include "histogrampage.h"
#include "convolutionpage.h"
#include "binarizationpage.h"
#include "cannypage.h"
#include "harrispage.h"
#include "editorcontext.h"
#include "pointoperationhandler.h"
#include "histogramhandler.h"
#include "convolutionhandler.h"
#include "binarizationhandler.h"
#include "cannyhandler.h"
#include "harrishandler.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QImageWriter>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , pointOperationPage(nullptr)
    , histogramPage(nullptr)
    , convolutionPage(nullptr)
    , binarizationPage(nullptr)
    , cannyPage(nullptr)
    , harrisPage(nullptr)
    , pointOperationHandler(nullptr)
    , histogramHandler(nullptr)
    , convolutionHandler(nullptr)
    , binarizationHandler(nullptr)
    , cannyHandler(nullptr)
    , harrisHandler(nullptr)
{
    ui->setupUi(this);

    ui->imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->imageLabel->setScaledContents(false);
    ui->imageLabel->setMinimumSize(1, 1);
    ui->imageLabel->setAlignment(Qt::AlignCenter);

    pointOperationPage = new PointOperationPage(this);
    histogramPage = new HistogramPage(this);
    convolutionPage = new ConvolutionPage(this);
    binarizationPage = new BinarizationPage(this);
    cannyPage = new CannyPage(this);
    harrisPage = new HarrisPage(this);

    ui->stackedWidget->addWidget(pointOperationPage);
    ui->stackedWidget->addWidget(histogramPage);
    ui->stackedWidget->addWidget(convolutionPage);
    ui->stackedWidget->addWidget(binarizationPage);
    ui->stackedWidget->addWidget(cannyPage);
    ui->stackedWidget->addWidget(harrisPage);

    {
        const QSignalBlocker blocker(ui->toolSelector);
        ui->toolSelector->clear();
        ui->toolSelector->addItem("Brak narzędzia");
        ui->toolSelector->addItem("Operacje punktowe");
        ui->toolSelector->addItem("Histogram");
        ui->toolSelector->addItem("Operacje sąsiedztwa");
        ui->toolSelector->addItem("Binaryzacja");
        ui->toolSelector->addItem("Canny");
        ui->toolSelector->addItem("Harris");
        ui->toolSelector->setCurrentIndex(0);
    }

    EditorContext ctx;
    ctx.pointOperationPage = pointOperationPage;
    ctx.histogramPage = histogramPage;
    ctx.binarizationPage = binarizationPage;
    ctx.convolutionPage = convolutionPage;
    ctx.cannyPage = cannyPage;
    ctx.harrisPage = harrisPage;
    ctx.currentImage = &currentImage;
    ctx.secondImage = &secondImage;
    ctx.secondImagePath = &secondImagePath;
    ctx.imageHistory = &imageHistory;
    ctx.messageParent = this;
    ctx.updateImagePreview = [this]() { updateImagePreview(); };
    ctx.refreshHistogramIfNeeded = [this]() { refreshHistogramIfNeeded(); };
    ctx.pushHistory = [this]() { pushHistory(); };
    ctx.ensureSameSize = [this](const QImage &a, const QImage &b) {
        return ensureSameSize(a, b);
    };
    ctx.undoLastOperation = [this]() {
        if (pointOperationHandler) {
            pointOperationHandler->undoLastOperation();
        }
    };

    pointOperationHandler = new PointOperationHandler(ctx, this);
    histogramHandler = new HistogramHandler(ctx, this);
    convolutionHandler = new ConvolutionHandler(ctx, this);
    binarizationHandler = new BinarizationHandler(ctx, binarizationPage, this);
    cannyHandler = new CannyHandler(ctx, this);
    harrisHandler = new HarrisHandler(ctx, this);

    connect(ui->toolSelector,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &MainWindow::on_toolSelector_currentIndexChanged);

    connect(pointOperationPage,
            &PointOperationPage::importSecondImageRequested,
            pointOperationHandler,
            &PointOperationHandler::importSecondImage);

    connect(ui->btnUndo,
            &QPushButton::clicked,
            pointOperationHandler,
            &PointOperationHandler::undoLastOperation);

    connect(pointOperationPage,
            &PointOperationPage::desaturateRequested,
            pointOperationHandler,
            &PointOperationHandler::applyDesaturation);

    connect(pointOperationPage,
            &PointOperationPage::negativeRequested,
            pointOperationHandler,
            &PointOperationHandler::applyNegative);

    connect(pointOperationPage,
            &PointOperationPage::brightnessRequested,
            pointOperationHandler,
            &PointOperationHandler::applyBrightness);

    connect(pointOperationPage,
            &PointOperationPage::saturationRequested,
            pointOperationHandler,
            &PointOperationHandler::applySaturation);

    connect(pointOperationPage,
            &PointOperationPage::linearContrastRequested,
            pointOperationHandler,
            &PointOperationHandler::applyLinearContrast);

    connect(pointOperationPage,
            &PointOperationPage::logarithmicContrastRequested,
            pointOperationHandler,
            &PointOperationHandler::applyLogarithmicContrast);

    connect(pointOperationPage,
            &PointOperationPage::powerContrastRequested,
            pointOperationHandler,
            &PointOperationHandler::applyPowerContrast);

    connect(pointOperationPage,
            &PointOperationPage::monoTransformRequested,
            pointOperationHandler,
            &PointOperationHandler::applyMonochromeTransform);

    connect(pointOperationPage,
            &PointOperationPage::sumImagesRequested,
            pointOperationHandler,
            &PointOperationHandler::applyImageSum);

    connect(pointOperationPage,
            &PointOperationPage::diffImagesRequested,
            pointOperationHandler,
            &PointOperationHandler::applyImageDifference);

    connect(pointOperationPage,
            &PointOperationPage::multiplyImagesRequested,
            pointOperationHandler,
            &PointOperationHandler::applyImageMultiply);

    connect(histogramPage,
            &HistogramPage::histogramDisplayChanged,
            histogramHandler,
            &HistogramHandler::refreshHistogramIfNeeded);

    connect(histogramPage,
            &HistogramPage::stretchHistogramRequested,
            histogramHandler,
            &HistogramHandler::applyHistogramStretch);

    connect(histogramPage,
            &HistogramPage::equalizeHistogramRequested,
            histogramHandler,
            &HistogramHandler::applyHistogramEqualization);

    connect(convolutionPage,
            &ConvolutionPage::applyRequested,
            convolutionHandler,
            &ConvolutionHandler::applyConvolution);

    connect(convolutionPage,
            &ConvolutionPage::backRequested,
            this,
            [this]() {
                ui->toolSelector->setCurrentIndex(0);
            });

    connect(binarizationPage,
            &BinarizationPage::manualThresholdRequested,
            binarizationHandler,
            &BinarizationHandler::applyManualThreshold);

    connect(binarizationPage,
            &BinarizationPage::otsuRequested,
            binarizationHandler,
            &BinarizationHandler::applyOtsuThreshold);

    connect(binarizationPage,
            &BinarizationPage::backRequested,
            this,
            [this]() {
                ui->toolSelector->setCurrentIndex(0);
            });

    connect(cannyPage,
            &CannyPage::applyRequested,
            cannyHandler,
            &CannyHandler::applyCanny);

    connect(harrisPage,
            &HarrisPage::applyRequested,
            harrisHandler,
            &HarrisHandler::applyHarris);

    updateImagePreview();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateImagePreview();
}

void MainWindow::on_toolSelector_currentIndexChanged(int index)
{
    ui->stackedWidget->setCurrentIndex(index);

    if (index == 2) {
        refreshHistogramIfNeeded();
    }

    if (index == 4 && binarizationHandler) {
        binarizationHandler->refreshPage();
    }

    if (convolutionPage) {
        convolutionPage->setImageLoaded(!currentImage.isNull());
    }

    if (binarizationPage) {
        binarizationPage->setImageLoaded(!currentImage.isNull());
    }

    if (cannyPage) {
        cannyPage->setImageLoaded(!currentImage.isNull());
    }

    if (harrisPage) {
        harrisPage->setImageLoaded(!currentImage.isNull());
    }
}

void MainWindow::pushHistory()
{
    if (!currentImage.isNull()) {
        imageHistory.push_back(currentImage);
    }
}

bool MainWindow::ensureSameSize(const QImage &img1, const QImage &img2)
{
    if (img1.size() != img2.size()) {
        QMessageBox::warning(this, "Uwaga", "Obrazy muszą mieć taki sam rozmiar.");
        return false;
    }

    return true;
}

void MainWindow::refreshHistogramIfNeeded()
{
    if (histogramHandler) {
        histogramHandler->refreshHistogramIfNeeded();
    }
}

void MainWindow::updateImagePreview()
{
    if (currentImage.isNull()) {
        ui->imageLabel->clear();
        ui->imageLabel->setText("Otwórz plik PBM PGM PPM JPG PNG");

        if (convolutionPage) {
            convolutionPage->setImageLoaded(false);
        }

        if (binarizationPage) {
            binarizationPage->setImageLoaded(false);
        }

        if (cannyPage) {
            cannyPage->setImageLoaded(false);
        }

        if (harrisPage) {
            harrisPage->setImageLoaded(false);
        }

        if (pointOperationPage) {
            pointOperationPage->setSecondImageName(QString());
        }

        return;
    }

    ui->imageLabel->setText(QString());

    QSize targetSize = ui->imageLabel->contentsRect().size();

    if (targetSize.width() <= 0 || targetSize.height() <= 0) {
        targetSize = ui->imageLabel->size();
    }

    if (targetSize.width() <= 0 || targetSize.height() <= 0) {
        return;
    }

    const QPixmap pixmap = QPixmap::fromImage(currentImage);
    const QPixmap scaledPixmap = pixmap.scaled(
        targetSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        );

    ui->imageLabel->setPixmap(scaledPixmap);

    if (convolutionPage) {
        convolutionPage->setImageLoaded(true);
    }

    if (binarizationPage) {
        binarizationPage->setImageLoaded(true);
    }

    if (cannyPage) {
        cannyPage->setImageLoaded(true);
    }

    if (harrisPage) {
        harrisPage->setImageLoaded(true);
    }

    if (pointOperationPage) {
        pointOperationPage->setSecondImageName(
            secondImagePath.isEmpty() ? QString() : QFileInfo(secondImagePath).fileName());
    }
}

void MainWindow::openFile(const QString &filter)
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        "Otwórz obraz",
        QString(),
        filter
        );

    if (path.isEmpty()) {
        return;
    }

    try {
        const QString lowerPath = path.toLower();

        if (lowerPath.endsWith(".pbm") || lowerPath.endsWith(".pgm") || lowerPath.endsWith(".ppm")) {
            currentImage = loadNetpbm(path);
        } else {
            QImage loaded;
            if (!loaded.load(path)) {
                QMessageBox::critical(this, "Błąd", "Nie udało się otworzyć pliku.");
                return;
            }
            currentImage = loaded;
        }

        secondImage = QImage();
        secondImagePath.clear();
        imageHistory.clear();

        updateImagePreview();
        refreshHistogramIfNeeded();

        if (binarizationHandler) {
            binarizationHandler->refreshPage();
        }

        statusBar()->showMessage("Wczytano " + path, 3000);
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Błąd", e.what());
    }
}

void MainWindow::saveFile(const QString &filter, bool ascii)
{
    if (currentImage.isNull()) {
        QMessageBox::warning(this, "Uwaga", "Najpierw otwórz obraz!");
        return;
    }

    const QString path = QFileDialog::getSaveFileName(
        this,
        "Zapisz obraz",
        QString(),
        filter
        );

    if (path.isEmpty()) {
        return;
    }

    try {
        const QString lowerPath = path.toLower();

        if (lowerPath.endsWith(".pbm") || lowerPath.endsWith(".pgm") || lowerPath.endsWith(".ppm")) {
            if (!saveNetpbm(currentImage, path, ascii)) {
                QMessageBox::critical(this, "Błąd", "Nie udało się zapisać pliku.");
                return;
            }
        } else {
            if (!currentImage.save(path)) {
                QMessageBox::critical(this, "Błąd", "Nie udało się zapisać pliku.");
                return;
            }
        }

        statusBar()->showMessage("Zapisano " + path, 3000);
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Błąd", e.what());
    }
}

void MainWindow::on_actionOpenPBM_triggered()
{
    openFile("PBM (*.pbm)");
}

void MainWindow::on_actionOpenPGM_triggered()
{
    openFile("PGM (*.pgm)");
}

void MainWindow::on_actionOpenPPM_triggered()
{
    openFile("PPM (*.ppm)");
}

void MainWindow::on_actionOpenJPG_triggered()
{
    openFile("JPG (*.jpg *.jpeg)");
}

void MainWindow::on_actionOpenPNG_triggered()
{
    openFile("PNG (*.png)");
}

void MainWindow::on_actionSavePBM_triggered()
{
    saveFile("PBM (*.pbm)", false);
}

void MainWindow::on_actionSavePGM_triggered()
{
    saveFile("PGM (*.pgm)", false);
}

void MainWindow::on_actionSavePPM_triggered()
{
    saveFile("PPM (*.ppm)", false);
}

void MainWindow::on_actionSaveJPG_triggered()
{
    saveFile("JPG (*.jpg *.jpeg)", false);
}

void MainWindow::on_actionSavePNG_triggered()
{
    saveFile("PNG (*.png)", false);
}