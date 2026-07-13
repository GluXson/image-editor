#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class PointOperationPage;
}
QT_END_NAMESPACE

class PointOperationPage : public QWidget
{
    Q_OBJECT

public:
    explicit PointOperationPage(QWidget *parent = nullptr);
    ~PointOperationPage();

    void setSecondImageName(const QString &fileName);

signals:
    void importSecondImageRequested();

    void desaturateRequested();
    void negativeRequested();

    void brightnessRequested(int value);
    void saturationRequested(int value);

    void linearContrastRequested(double factor);
    void logarithmicContrastRequested(double c);
    void powerContrastRequested(double gamma);

    void monoTransformRequested(double a, double b);

    void sumImagesRequested();
    void diffImagesRequested();
    void multiplyImagesRequested();

private slots:
    void on_btnImportSecondImage_clicked();
    void on_btnDesaturate_clicked();
    void on_btnNegative_clicked();
    void on_btnApplyBrightness_clicked();
    void on_btnApplySaturation_clicked();
    void on_btnApplyContrast_clicked();
    void on_btnApplyMono_clicked();
    void on_btnSum_clicked();
    void on_btnDiff_clicked();
    void on_btnMultiply_clicked();

private:
    Ui::PointOperationPage *ui;
};