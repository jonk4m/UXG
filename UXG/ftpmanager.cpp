#include "ftpmanager.h"
#include <QNetworkAccessManager>

FtpManager::FtpManager()
{
    url = new QUrl("ftp://");
    m_manager = new QNetworkAccessManager();
    if(!connect(m_manager,&QNetworkAccessManager::finished,this,&FtpManager::manager_finished,Qt::DirectConnection)) //note this connection is invalid if FtpManager class was not a Q_OBJECT
        qDebug() << "issue with slot 0";


    process = new QProcess(this);
}

/*
 * NOTES
 * when I use the keysight command expert to send "*IDN?\n" over TCP, it says it sent to port 4880 even though the manual says to use port 5025
 * when I use packet sender with port 5025 I get the proper response with "*IDN?\n" so it turns out I was missing the "\n" the whole time
 * Making FtpManager a Q_Object simplifies connections
 */

void FtpManager::upload_table(){
    url->setHost("K-N5193A-90114");//169.254.24.85 K-N5193A-90114
    url->setPort(21);
    url->setPath("/BIN/PQEXPORT.CSV");
    url->setUserName("user");
    url->setPassword("keysight");
    qDebug() << "URL is : " + url->toString();
    if(!connect(m_manager, &QNetworkAccessManager::authenticationRequired, this, &FtpManager::auth))
         qDebug() << "1.1";
    if(!connect(m_manager, &QNetworkAccessManager::sslErrors, this, &FtpManager::auth))
         qDebug() << "1.2";
    if(!connect(m_manager, &QNetworkAccessManager::proxyAuthenticationRequired, this, &FtpManager::auth))
         qDebug() << "1.3";
    if(!connect(m_manager, &QNetworkAccessManager::encrypted, this, &FtpManager::auth))
         qDebug() << "1.4";
    if(!connect(m_manager, &QNetworkAccessManager::preSharedKeyAuthenticationRequired, this, &FtpManager::auth))
         qDebug() << "1.5";

    QNetworkReply *reply = m_manager->get(QNetworkRequest(*url));

    if(!connect(reply,&QNetworkReply::readyRead, this, &FtpManager::reply_ready_read))
        qDebug() << "1";
    if(!connect(reply,&QNetworkReply::errorOccurred, this, &FtpManager::reply_error_occured))
             qDebug() << "3";
    if(!connect(reply,&QNetworkReply::preSharedKeyAuthenticationRequired, this, &FtpManager::reply_preshare_auth_required))
             qDebug() << "4";
    if(!connect(reply,&QNetworkReply::redirected, this, &FtpManager::reply_redirected))
             qDebug() << "5";
    if(!connect(reply,&QNetworkReply::finished, this, &FtpManager::reply_finished))
             qDebug() << "6";
}

void FtpManager::reply_finished(){
    qDebug() << "reply finished";
    //qDebug() << sender()->readAll();
}

void FtpManager::manager_finished(QNetworkReply *reply){
    qDebug() << "manager finished";
    if (!reply->error())
    {
        /*window_fpcs.workingFile.close();
        window_fpcs.workingFile.deleteLater();  // delete object of file
        QFile *testFile = new QFile("C:/Users/abms1/Desktop/downloadedFile.txt");
        if(testFile->open(QFile::ReadWrite)){
            testFile->write(reply->readAll());
            testFile->close();
        }*/
        qDebug() << reply->readAll();
        reply->deleteLater();   // delete object of reply

    }else{
        qDebug() << "ERROR : ";
        qDebug() << reply->errorString();
    }
}

void FtpManager::reply_ready_read(){
    qDebug() << "reply ready read";
}

void FtpManager::reply_error_occured(){
     qDebug() << "reply error occured";
}

void FtpManager::reply_preshare_auth_required(){
     qDebug() << "reply preshare authentification required";
}

void FtpManager::reply_redirected(){
     qDebug() << "reply redirected";
}

void FtpManager::auth(){
     qDebug() << "NetworkAccessManager auth";
}

void FtpManager::download_table(){
     qDebug() << "downloading table";
}

void FtpManager::send_SCPI(){
     qDebug() << "sending SCPI";
}






void FtpManager::start_upload_process(){
    qDebug() << "Starting upload process";
    QStringList arguments;
    arguments << "-d" << "-i" << "-s:C:\\Users\\abms1\\Desktop\\ftpCommands2.txt" << "K-N5193A-90114";
    QString program = "ftp";
    process->start(program,arguments);
    connect(process,SIGNAL(started()),this,SLOT(process_started()));
    connect(process,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(process_finished()));
    connect(process,SIGNAL(errorOccurred(QProcess::ProcessError)),this,SLOT(process_finished()));
    connect(process,SIGNAL(readyReadStandardError()),this,SLOT(process_ready_read_error()));
    connect(process,SIGNAL(readyReadStandardOutput()),this,SLOT(process_ready_read_output()));
    qDebug() << "finished adding connections";
}

void FtpManager::process_started(){
    qDebug() << "Process has started successfully";
}

void FtpManager::process_finished(){
    qDebug() << "Process has finished successfully";
    qDebug() << process->exitCode();
}

void FtpManager::process_error_occured(){
    qDebug() << "Process has error occured";
    qDebug() << process->error();
}

void FtpManager::process_ready_read_error(){
    qDebug() << "Process has ready read error";
    qDebug() << process->readAllStandardError();
}

void FtpManager::process_ready_read_output(){
    qDebug() << "Process output ---------------------------------------------------------------------:";
    QString allRead = process->readAllStandardOutput();
    QStringList allReadParsed = allRead.split(QRegExp("[\r\n]"), Qt::SkipEmptyParts);
    for(QString item : allReadParsed){
        qDebug() << item;
    }
    qDebug() << "\n";
    /*switch(step){
    case 0:
        qDebug() << "reached step 0";
        //process->write("USER user\r\n"); //   \r\r\n ; \r\n ; \r ; \n ; \r\r ; nothing ; USER user ; user
        process->write("PASS keysight\r\n");
        process->closeWriteChannel();
        break;
    case 1:
        qDebug() << "reached step 1";
        process->write("bye");
        break;
    default:
        qDebug() << "Unknown step reached";
        break;
    }*/
}












