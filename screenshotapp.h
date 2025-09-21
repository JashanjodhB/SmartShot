#ifndef SCREENSHOTAPP_H
#define SCREENSHOTAPP_H

#include <QMainWindow>
#include <QVector>
#include <QPixmap>

class QLabel;
class QPushButton;

class ScreenshotApp : public QMainWindow {
    Q_OBJECT
public:
    ScreenshotApp(QWidget *parent = nullptr);

private slots:
    void takeScreenshot();
    void redoScreenshot();
    void finishPDF();

private:
    QPushButton *take;
    QPushButton *redo;
    QPushButton *finish;
    QLabel *imageLabel;

    QVector<QPixmap> screenshots;
    QPixmap lastSC;
};

#endif
