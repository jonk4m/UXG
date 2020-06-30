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

#include <QProcess>

class FtpManager: public QObject
{
    Q_OBJECT
public:

    FtpManager();

    QUrl *url;
    QNetworkAccessManager *m_manager;

    void upload_table();
    void download_table();
    void send_SCPI();

    QProcess *process;
    void start_upload_process();
    int step = 0;

private slots:

    void manager_finished(QNetworkReply*);
    void auth();
    void reply_ready_read();
    void reply_error_occured();
    void reply_preshare_auth_required();
    void reply_redirected();
    void reply_finished();

    void process_started();
    void process_finished();
    void process_error_occured();
    void process_ready_read_error();
    void process_ready_read_output();
};

#endif // FTPMANAGER_H
