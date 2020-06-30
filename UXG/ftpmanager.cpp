#include "ftpmanager.h"
#include <QNetworkAccessManager>

FtpManager::FtpManager(QMainWindow *window)
{
    process = new QProcess(this);
    current_state = state::initialized;
    tcpSocket = new QTcpSocket(window);
}

/*
 * NOTES
 * when I use the keysight command expert to send "*IDN?\n" over TCP, it says it sent to port 4880 even though the manual says to use port 5025
 * when I use packet sender with port 5025 I get the proper response with "*IDN?\n" so it turns out I was missing the "\n" the whole time
 * Making FtpManager a Q_Object simplifies connections
 */

void FtpManager::start_process(QString *fileOrFolderName){
    qDebug() << "Starting upload process";
    QStringList arguments;
    QString program = "ftp";

    if(current_state == state::uploading){
        qDebug() << "Name is : ";
        qDebug() << *fileOrFolderName;
        //TODO set the filename in the text document of the ftp commands to match the input parameter
        QString commandPath = QDir::currentPath() + "/fileFolder/uploadFtpCommands.txt";
        QFile commandFile(commandPath);
        if(!commandFile.open(QIODevice::ReadWrite | QIODevice::Text))
            qDebug() << "Failed to open uploadFtpCommands.txt";
        qDebug() << commandFile.readLine();
        qDebug() << commandFile.readLine();
        qDebug() << commandFile.readLine();
        qDebug() << commandFile.readLine();
        QString textLine = "mput " + *fileOrFolderName + "\n";
        commandFile.write(textLine.toUtf8());
        commandFile.write("bye");
        commandFile.flush();
        commandFile.close();

        QString commandArg = "-s:" + commandPath; //note that the uploads folder must be added upon deployment
        arguments << "-d" << "-i" << commandArg << "K-N5193A-90114";   //C:\\Users\\abms1\\Desktop\\uploadFtpCommands.txt
        //Make sure to copy FTP instruction text files to the deployed version folder next to the executable
    }else if(current_state == state::downloading){
        //Download all files from the UXG
        QString commandArg = "-s:" + QDir::currentPath() + "/fileFolder/downloadFtpCommands.txt"; //note that the downloads folder must be added upon deployment
        arguments << "-d" << "-i" << commandArg << "K-N5193A-90114";
    }else{
        qDebug() << "Error in FtpManager state, should be either uploading or downloading";
    }
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
    qDebug() << "Process output -----------------:";
    QString allRead = process->readAllStandardOutput();
    QStringList allReadParsed = allRead.split(QRegExp("[\r\n]"), Qt::SkipEmptyParts);
    for(QString item : allReadParsed){
        qDebug() << item;
    }
    qDebug() << "\n";
}


void FtpManager::send_SPCI(QString *message, QString *host, quint16 *port){
    if(tcpSocket->state() != QAbstractSocket::ConnectedState){
        //connect to server

    }else{
        qDebug() << "Socket connection is either busy or not connected";
    }
}










