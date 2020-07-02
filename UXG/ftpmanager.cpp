#include "ftpmanager.h"
#include <QNetworkAccessManager>

FtpManager::FtpManager(QMainWindow *window)
{
    process = new QProcess(window);
    current_state = state::initialized;
    tcpSocket = new QTcpSocket(window);
    QAbstractSocket::connect(tcpSocket,SIGNAL(readyRead()),window,SLOT(on_socket_readyRead()));
}

/*
 * This method downloads or uploads files from or to the UXG
 * notes that a fileOrFolderName parameter of "*" means that all files will be downloaded
 */
void FtpManager::start_process(QString fileOrFolderName){
    qDebug() << "Starting upload process";
    QStringList arguments;
    QString program = "ftp";

    if(current_state == state::uploading){
        qDebug() << "Name is : ";
        qDebug() << fileOrFolderName;
        //TODO set the filename in the text document of the ftp commands to match the input parameter
        QString commandPath = QDir::currentPath() + "/fileFolder/uploadFtpCommands.txt";
        QFile commandFile(commandPath);
        if(!commandFile.open(QIODevice::ReadWrite | QIODevice::Text))
            qDebug() << "Failed to open uploadFtpCommands.txt";
        qDebug() << commandFile.readLine();
        qDebug() << commandFile.readLine();
        qDebug() << commandFile.readLine();
        qDebug() << commandFile.readLine();
        QString textLine = "mput " + fileOrFolderName + "\n";
        commandFile.write(textLine.toUtf8());
        commandFile.write("bye");
        commandFile.flush();
        commandFile.close();

        QString commandArg = "-s:" + commandPath;
        arguments << "-d" << "-i" << commandArg << "K-N5193A-90114";

    }else if(current_state == state::downloading){
        //Download all files from the UXG
        QString commandPath = QDir::currentPath() + "/fileFolder/downloadFtpCommands.txt";
        QFile commandFile(commandPath);
        qDebug() << commandFile.readLine();
        qDebug() << commandFile.readLine();
        qDebug() << commandFile.readLine();
        qDebug() << commandFile.readLine();
        qDebug() << commandFile.readLine();
        QString textLine = "mget " + fileOrFolderName + "\n";
        commandFile.write(textLine.toUtf8());
        commandFile.write("bye");
        commandFile.flush();
        commandFile.close();

        QString commandArg = "-s:" + QDir::currentPath() + "/fileFolder/downloadFtpCommands.txt"; //note that the downloads folder must be added upon deployment
        arguments << "-d" << "-i" << commandArg << "K-N5193A-90114";

    }else{
        qDebug() << "Error in FtpManager state, should be either uploading or downloading";
    }


    process->start(program,arguments);
    QProcess::connect(process,SIGNAL(started()),this,SLOT(process_started()));
    QProcess::connect(process,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(process_finished()));
    QProcess::connect(process,SIGNAL(errorOccurred(QProcess::ProcessError)),this,SLOT(process_finished()));
    QProcess::connect(process,SIGNAL(readyReadStandardError()),this,SLOT(process_ready_read_error()));
    QProcess::connect(process,SIGNAL(readyReadStandardOutput()),this,SLOT(process_ready_read_output()));
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
}

void FtpManager::send_SPCI(QString message){
    if(tcpSocket->state() == QAbstractSocket::ConnectedState){
        QString str = message + "\n";
        tcpSocket->write(str.toUtf8());
    }else if(tcpSocket->state() == QAbstractSocket::HostLookupState){
        qDebug() << "Socket cannot send command because it is in the host lookup state";
    }else if(tcpSocket->state() == QAbstractSocket::ListeningState){
        qDebug() << "Socket cannot send command because it is currently listening to the connection";
    }else if(tcpSocket->state() == QAbstractSocket::UnconnectedState){
        qDebug() << "Socket cannot send command because it is unconnected";
    }else if(tcpSocket->state() == QAbstractSocket::ClosingState){
        qDebug() << "Socket cannot send command because it is in the process of closing it's connection";
    }else if(tcpSocket->state() == QAbstractSocket::ConnectingState){
        qDebug() << "Socket cannot send command because it is currently in the process of connecting";
    }
}

void FtpManager::connect(QString host, quint16 port){
    if(tcpSocket->state() == QAbstractSocket::ConnectedState){
        qDebug() << "Socket already connected";
    }else{
        tcpSocket->connectToHost(host,port,QIODevice::ReadWrite);
        QAbstractSocket::connect(tcpSocket,SIGNAL(connected()),this,SLOT(socket_connected()));
        QAbstractSocket::connect(tcpSocket,SIGNAL(disconnected()),this,SLOT(socket_disconnected()));
        QAbstractSocket::connect(tcpSocket,SIGNAL(errorOccurred(QAbstractSocket::SocketError*)),this,SLOT(socket_errorOccurred(QAbstractSocket::SocketError*)));
    }
}

void FtpManager::closeTcpSocket(){
    tcpSocket->close();
}

void FtpManager::socket_connected(){
    qDebug() << "Socket connected";
}

void FtpManager::socket_disconnected(){
    qDebug() << "Socket disconnected";
}


void FtpManager::socket_errorOccurred(QAbstractSocket::SocketError *error){
    qDebug() << "Socket error occurred : ";
    qDebug() << error;
}








