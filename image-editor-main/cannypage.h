#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class CannyPage;
}
QT_END_NAMESPACE

class CannyPage : public QWidget
{
    Q_OBJECT

public:
    explicit CannyPage(QWidget *parent = nullptr);
    ~CannyPage();

    void setImageLoaded(bool loaded);

signals:
    void applyRequested(int kernelSize,
                        double sigma,
                        int lowThreshold,
                        int highThreshold,
                        bool useL2Gradient);

private slots:
    void onApplyClicked();
    void onAnyParameterChanged();
    void onPanelInfoClicked();

private:
    Ui::CannyPage *ui;
    bool imageLoaded = false;
    bool syncInProgress = false;

    void setupUiState();
    void updateInfoLabel();
    void updateTitles();
};