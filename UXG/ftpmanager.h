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

class FtpManager: public QObject
{
    Q_OBJECT
public:

    FtpManager(QMainWindow *window);

    QProcess *process;
    QTcpSocket *tcpSocket;

    enum state{
        initialized = 0,
        uploading = 1,
        downloading = 2,
        sending_scpi = 3
    };

    state current_state;
    void start_process(QString*);
    void send_SPCI(QString*);

private slots:
    void process_started();
    void process_finished();
    void process_error_occured();
    void process_ready_read_error();
    void process_ready_read_output();

    void displayError(QAbstractSocket::SocketError socketError);

};

#endif // FTPMANAGER_H
