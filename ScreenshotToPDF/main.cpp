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
#include <QHttpServer>
#include <QJsonObject>
#include <QJsonArray>
#include <QBuffer>
#include <QHostAddress>
#include <QDebug>
#include <QCoreApplication>
#include <QTcpServer>
#include <QHttpServerResponse>


//Screenshot Struct to hold basic info
struct Screenshot {
    QPixmap pixmap;
    QString id;
    QDateTime timestamp;
};






//Overlay to show what the selected area is
class SCOverlay : public QDialog {
    Q_OBJECT


public:
    SCOverlay(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setWindowState(Qt::WindowFullScreen);
        rubberBand = nullptr;
    }
    //The selected area
    QRect selectedRect() const {
        return selection;
    }



protected:

    //start selecting
    void mousePressEvent(QMouseEvent *e) override {
        origin=e->pos();
        if (!rubberBand)
            rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
        rubberBand->setGeometry(QRect(origin, QSize()));
        rubberBand->show();
    }

    //update positions and rectangle
    void mouseMoveEvent(QMouseEvent *e) override {
        if (rubberBand)
            rubberBand->setGeometry(QRect(origin, e->pos()).normalized());
    }

    //finish selecting
    void mouseReleaseEvent(QMouseEvent *e) override {
        if (rubberBand) {
            selection = rubberBand->geometry();
            rubberBand->hide();
        }
        accept();
    }

    //selected area
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), QColor(0, 0, 0, 100));
    }



private:
    QRubberBand *rubberBand;
    QPoint origin;
    QRect selection;
};






//Main app
class ScreenshotApp: public QMainWindow {
    Q_OBJECT

public:
    ScreenshotApp(QWidget *parent = nullptr): QMainWindow(parent){
        QWidget *central = new QWidget(this);
        QVBoxLayout *layout= new QVBoxLayout(central);


        //Main buttons in menu, not necessary for using through oneNote extension
        take= new QPushButton("Take a Screenshot",this);
        redo= new QPushButton("Retake the Screenshot",this);
        finish= new QPushButton("Done",this);

        //Disabled at start, when there are no SCs in list
        redo->setEnabled(false);
        finish->setEnabled(false);

        image= new QLabel(this);
        image->setAlignment(Qt::AlignCenter);

        layout->addWidget(redo);
        layout->addWidget(take);
        layout->addWidget(finish);
        layout->addWidget(image);

        setCentralWidget(central);
        //connecting functions and buttons
        connect(take, &QPushButton::clicked, this, &ScreenshotApp::takeScreenShot);
        connect(redo, &QPushButton::clicked, this, &ScreenshotApp::redoScreenShot);
        connect(finish, &QPushButton::clicked, this, &ScreenshotApp::screenShotToPDF);
    }


    //returns last screen shot taken
    Screenshot getLatestScreenshot() const {
        if (screenshots.isEmpty()) {
            return Screenshot();
        }
        return screenshots.last();
    }

    //Get specific screenshot
    Screenshot getScreenshotById(const QString &id) const {
        for (const auto &sc : screenshots) {
            if (sc.id == id) {
                return sc;
            }
        }
        return Screenshot();
    }

    //Returns list of all screenshots in list
    QVector<Screenshot> getAllScreenshots() const {
        return screenshots;
    }





public slots:
    void takeScreenShot(){
        this->hide();
        QApplication::processEvents();

        //call overlay
        SCOverlay overlay;
        if(overlay.exec()==QDialog::Accepted){
            QRect rect=overlay.selectedRect();
            QScreen *screen= QGuiApplication::primaryScreen();
            if(!screen || rect.isNull()){
                this->show();
                return;
            }
            QPixmap pixmap= screen->grabWindow(0,rect.x(),rect.y(),rect.width(),rect.height());

            //turn into screenshot struct
            Screenshot sc;
            sc.pixmap = pixmap;
            sc.id = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
            sc.timestamp = QDateTime::currentDateTime();


            //add to list
            screenshots.push_back(sc);

            image->setPixmap(pixmap.scaled(400,300,Qt::KeepAspectRatio));


            //enable redo and finish buttons, since theres atleast 1 image in list
            redo->setEnabled(true);
            finish->setEnabled(true);
        }
        this->show();
    }

private slots:
    //redo the last screenshot taken
    void redoScreenShot(){
        if(!screenshots.isEmpty()){
            screenshots.pop_back();
        }
        takeScreenShot();
    }


    //Convert the screenshots into a pdf
    void screenShotToPDF(){
        //empty check
        if(screenshots.isEmpty()){
            return;
        }

        //What to name the file to be saved
        QString filename= QFileDialog::getSaveFileName(this,"Save PDF","","PDF Files (*.pdf)");
        if(filename.isEmpty()){
            return;
        }


        //write the file to storage
        QPdfWriter writer(filename);
        writer.setPageSize(QPageSize(QPageSize::A4));
        QPainter painter(&writer);

        for(int i=0;i<screenshots.size();i++){
            QPixmap scaled= screenshots.at(i).pixmap.scaled(writer.width(),writer.height(),Qt::KeepAspectRatio);
            painter.drawPixmap(0,0,scaled);
            if(i!= screenshots.size()-1){
                writer.newPage();
            }
        }
        painter.end();


        //Reset variables
        screenshots.clear();
        redo->setEnabled(false);
        finish->setEnabled(false);
        image->clear();
    }

private:
    QPushButton *take;
    QPushButton *redo;
    QPushButton *finish;
    QLabel *image;

    QVector<Screenshot> screenshots;
};






//Server to connect with javaScript for OneNote add in logic
class HttpServer : public QObject {
    Q_OBJECT

public:
    explicit HttpServer(ScreenshotApp *app,QObject *parent=nullptr):QObject(parent),screenshotApp(app){
        server = new QHttpServer(this);
        setupRoutes();
    }

    //Connect to local port 8080
    void start(int port = 8080) {
        auto tcpServer = std::make_unique<QTcpServer>();

        if (!tcpServer->listen(QHostAddress::LocalHost, port) || !server->bind(tcpServer.get())) {
            qDebug() << "Failed to listen on port:" << port;
            return;
        }

        quint16 serverPort = tcpServer->serverPort();
        tcpServer.release();
        qDebug() << "Server started on http://localhost:" << serverPort;
    }

private:
    QHttpServer *server;
    ScreenshotApp *screenshotApp;
    void setupRoutes();
};







void HttpServer::setupRoutes() {

    //CORS headers to allow for QT C++ and JS connection across ports
    auto addCorsHeaders = [](QHttpServerResponse response) {
        QHttpHeaders headers = response.headers();
        headers.append(QHttpHeaders::WellKnownHeader::AccessControlAllowOrigin, "*");
        headers.append(QHttpHeaders::WellKnownHeader::AccessControlAllowMethods, "GET, POST, PUT, DELETE, OPTIONS");
        headers.append(QHttpHeaders::WellKnownHeader::AccessControlAllowHeaders, "Content-Type, Authorization");
        response.setHeaders(std::move(headers));
        return response;
    };

    // status page to check if running correctly
    server->route("/status", QHttpServerRequest::Method::Options, [addCorsHeaders]() {
        return addCorsHeaders(QHttpServerResponse(QHttpServerResponse::StatusCode::NoContent));
    });


    //capture page to call capture functions
    server->route("/capture", QHttpServerRequest::Method::Options, [addCorsHeaders]() {
        return addCorsHeaders(QHttpServerResponse(QHttpServerResponse::StatusCode::NoContent));
    });


    //screenshots page to return all screenshots
    server->route("/screenshots", QHttpServerRequest::Method::Options, [addCorsHeaders]() {
        return addCorsHeaders(QHttpServerResponse(QHttpServerResponse::StatusCode::NoContent));
    });


    //return one specific screenshot
    server->route("/screenshot/<arg>", QHttpServerRequest::Method::Options, [addCorsHeaders](const QString &id) {
        Q_UNUSED(id);
        return addCorsHeaders(QHttpServerResponse(QHttpServerResponse::StatusCode::NoContent));
    });



    //Defining behavior and info
    server->route("/status", [addCorsHeaders]() {
        return addCorsHeaders(QHttpServerResponse(QJsonObject{
            {"status", "ok"},
            {"message", "Qt Server Running"}
        }));
    });

    server->route("/capture", QHttpServerRequest::Method::Post, [this, addCorsHeaders](const QHttpServerRequest &request) {
        Q_UNUSED(request);
        QMetaObject::invokeMethod(screenshotApp, "takeScreenShot", Qt::QueuedConnection);
        return addCorsHeaders(QHttpServerResponse(QJsonObject{
            {"success", true},
            {"message", "Screenshot captured"}
        }));
    });

    server->route("/screenshots", QHttpServerRequest::Method::Get, [this, addCorsHeaders]() {
        QJsonArray screenshotsArray;
        auto allScreenshots = screenshotApp->getAllScreenshots();

        for (const auto &sc : allScreenshots) {
            screenshotsArray.append(QJsonObject{
                {"id", sc.id},
                {"timestamp", sc.timestamp.toString(Qt::ISODate)},
                {"width", sc.pixmap.width()},
                {"height", sc.pixmap.height()}
            });
        }

        return addCorsHeaders(QHttpServerResponse(QJsonObject{
            {"success", true},
            {"screenshots", screenshotsArray}
        }));
    });

    server->route("/screenshot/<arg>", QHttpServerRequest::Method::Get, [this, addCorsHeaders](const QString &id) {
        Screenshot sc = screenshotApp->getScreenshotById(id);

        if (sc.pixmap.isNull()) {
            return addCorsHeaders(QHttpServerResponse(
                QJsonObject{{"error", "Screenshot not found"}},
                QHttpServerResponse::StatusCode::NotFound
                ));
        }

        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        sc.pixmap.save(&buffer, "PNG");

        return addCorsHeaders(QHttpServerResponse("image/png", byteArray));
    });

    server->route("/export-pdf", QHttpServerRequest::Method::Post, [this, addCorsHeaders](const QHttpServerRequest &request) {
        Q_UNUSED(request);
        QMetaObject::invokeMethod(screenshotApp, "screenShotToPDF", Qt::QueuedConnection);
        return addCorsHeaders(QHttpServerResponse(QJsonObject{
            {"success", true},
            {"message", "Sent to pdf"}
        }));
    });
}


int main(int argc, char *argv[]){
    QApplication a(argc, argv);

    ScreenshotApp window;
    window.setWindowTitle("Qt Server");

    //Uncomment these lines to show QT interface instead of only linked with oneNote Addin
    //window.resize(600, 600);
    //window.show();

    HttpServer server(&window);
    server.start(8080);

    return a.exec();
}

#include "main.moc"
