#ifndef FTPMANAGER_H
#define FTPMANAGER_H

#include <QMainWindow>
#include <QObject>
#include <QWidget>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QtNetwork>
#include <QDebug>
#include <QtCore>
#include <QFile>
#include <QProcess>
#include <QFileDialog>
#include <QTcpSocket>
#include <QGuiApplication>

class FtpManager: public QObject
{
    Q_OBJECT
public:

    FtpManager(QMainWindow *window);

    QProcess *process;
    QTcpSocket *tcpSocket;
    QString uploadingFileOrFolderName; //needs to be preserved across socket signals (see the finished slot for tcpSocket)

    enum state{
        initialized = 0,
        uploading = 1,
        downloading = 2,
        sending_scpi = 3,
        uploadingPDW = 4
    };

    enum DownloadState {
        finished = 0,
        exportingTable = 1,
        settingCurrentTable = 2
    };

    enum UXGSetup {Phase1, Phase2};
        UXGSetup setup = Phase1;
        void UXGSetup();
        bool UXGSetupFinished=true;

    state current_state;
    bool waitingForFPCSConversion = false;
    bool  waitingForFPCSFileList;
    bool waitingForPdwUpload = false;
    bool downloadingAllFiles = false;
    DownloadState downloadState = finished;
    QString hostName = "169.254.24.85"; //default hostname

    void start_process(QString);
    void send_SCPI(QString);
    void connect(quint16);
    void closeTcpSocket();
    void abortTcpSocket();
    void playPDW(QString fileName, bool isContinuous);

private slots:

    void process_started();
    void process_finished();
    void process_error_occured();
    void process_ready_read_error();
    void process_ready_read_output();

    void socket_connected();
    void socket_disconnected();
    void socket_errorOccurred(QAbstractSocket::SocketError*);
signals:
     void userMessage(QString message);
     void reopen_file();

};

#endif // FTPMANAGER_H
