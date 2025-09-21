#include <QApplication>
#include <QMainWindow>
#include <QScreen>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPixmap>
#include <QLabel>
#include <QVector>
#include <QDateTime>
#include <QFileDialog>
#include <QPdfWriter>
#include <QPainter>
#include <QScrollArea>
#include <QMouseEvent>
#include <QRubberBand>
#include <QPainter>

class SCOverlay : public QDialog {
    Q_OBJECT
public:
    SCOverlay(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setWindowState(Qt::WindowFullScreen);
        rubberBand = nullptr;
    }

    QRect selectedRect() const { return selection; }

protected:
    void mousePressEvent(QMouseEvent *e) override {
        origin = e->pos();
        if (!rubberBand)
            rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
        rubberBand->setGeometry(QRect(origin, QSize()));
        rubberBand->show();
    }

    void mouseMoveEvent(QMouseEvent *e) override {
        if (rubberBand)
            rubberBand->setGeometry(QRect(origin, e->pos()).normalized());
    }

    void mouseReleaseEvent(QMouseEvent *e) override {
        if (rubberBand) {
            selection = rubberBand->geometry();
            rubberBand->hide();
        }
        accept(); // closes dialog and makes exec() return
    }

    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), QColor(0, 0, 0, 100)); // dim background
    }

private:
    QRubberBand *rubberBand;
    QPoint origin;
    QRect selection;
};



class ScreenshotApp: public QMainWindow {
    Q_OBJECT

public:
    ScreenshotApp(QWidget *parent = nullptr): QMainWindow(parent){
        QWidget *central = new QWidget(this);
        QVBoxLayout *layout= new QVBoxLayout(central);

        take= new QPushButton("Take a Screenshot",this);
        redo= new QPushButton("Retake the Screenshot",this);
        finish= new QPushButton("Done",this);

        redo->setEnabled(false);
        finish->setEnabled(false);

        image= new QLabel(this);
        image->setAlignment(Qt::AlignCenter);

        layout->addWidget(redo);
        layout->addWidget(take);
        layout->addWidget(finish);
        layout->addWidget(image);

        setCentralWidget(central);

        connect(take, &QPushButton::clicked, this, &ScreenshotApp::takeScreenShot);
        connect(redo, &QPushButton::clicked, this, &ScreenshotApp::redoScreenShot);
        connect(finish, &QPushButton::clicked, this, &ScreenshotApp::screenShotToPDF);


    }
private slots:
    void takeScreenShot(){
        this->hide();
        QApplication::processEvents();

        SCOverlay overlay;
        if(overlay.exec()==QDialog::Accepted){
            QRect rect=overlay.selectedRect();
            QScreen *screen= QGuiApplication::primaryScreen();
            if(!screen || rect.isNull()){
                this->show();
                return;
            }
            QPixmap pixmap= screen->grabWindow(0,rect.x(),rect.y(),rect.width(),rect.height());
            lastSC=pixmap;
            screenshots.push_back(lastSC);

            image->setPixmap(pixmap.scaled(400,300,Qt::KeepAspectRatio));

            redo->setEnabled(true);
            finish->setEnabled(true);
        }
        this->show();



    }
    void redoScreenShot(){
        if(!screenshots.isEmpty()){
            screenshots.pop_back();
        }
        takeScreenShot();
    }
    void screenShotToPDF(){
        if(screenshots.isEmpty()){
            return;
        }
        QString filename= QFileDialog::getSaveFileName(this,"Save PDF","","PDF Files (*.pdf)");
        if(filename.isEmpty()){
            return;
        }
        QPdfWriter writer(filename);
        writer.setPageSize(QPageSize(QPageSize::A4));
        QPainter painter(&writer);

        for(int i=0;i<screenshots.size();i++){
            QPixmap scaled= screenshots.at(i).scaled(writer.width(),writer.height(),Qt::KeepAspectRatio);
            painter.drawPixmap(0,0,scaled);
            if(i!= screenshots.size()-1){
                writer.newPage();
            }
        }
        painter.end();
        screenshots.clear();

        redo->setEnabled(false);
        finish->setEnabled(false);
        image->setEnabled(false);
    }
private:
    QPushButton *take;
    QPushButton *redo;
    QPushButton *finish;
    QLabel *image;

    QVector<QPixmap> screenshots;
    QPixmap lastSC;
};


int main(int argc, char *argv[]){
    QApplication a(argc, argv);
    ScreenshotApp window;
    window.resize(600,500);
    window.show();
    return a.exec();
}
#include "main.moc"
