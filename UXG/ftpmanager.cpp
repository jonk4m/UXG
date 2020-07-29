#include "ftpmanager.h"
#include <QNetworkAccessManager>

FtpManager::FtpManager(QMainWindow *window)
{
    process = new QProcess(window);
    current_state = state::initialized;
    tcpSocket = new QTcpSocket(window);
    QAbstractSocket::connect(tcpSocket,SIGNAL(readyRead()),window,SLOT(on_socket_readyRead()));
    waitingForFPCSFileList = false;
}

/*
 * This method downloads or uploads files from or to the UXG
 * notes that a fileOrFolderName parameter of "*" means that all files will be downloaded
 * the fileOrFolderName parameter includes the current directory when uploading
 */
void FtpManager::start_process(QString fileOrFolderName){
    qDebug() << "Starting process";
    QStringList arguments;
    QString program = "ftp";

    if(current_state == state::uploading){
        qDebug() << "Name is : ";
        qDebug() << fileOrFolderName;
        //set the filename in the text document of the ftp commands to match the input parameter
        QString commandPath = QDir::currentPath() + "/fileFolder/uploadFtpCommands.txt";
        QFile commandFile(commandPath);
        if(!commandFile.open(QIODevice::ReadWrite | QIODevice::Text))
            qDebug() << "Failed to open uploadFtpCommands.txt";
        commandFile.resize(0); //clears the entire file
        commandFile.write(QString("user\n").toUtf8());
        commandFile.write(QString("keysight\n").toUtf8());
        commandFile.write(QString("lcd fileFolder/uploads\n").toUtf8());
        commandFile.write(QString("cd /USER/BIN\n").toUtf8());
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
        if(!commandFile.open(QIODevice::ReadWrite | QIODevice::Text))
            qDebug() << "Failed to open downloadFtpCommands.txt";
        commandFile.resize(0); //clears the entire file
        commandFile.write(QString("user\n").toUtf8());
        commandFile.write(QString("keysight\n").toUtf8());
        commandFile.write(QString("lcd\n").toUtf8());
        commandFile.write(QString("lcd fileFolder/downloads\n").toUtf8());
        commandFile.write(QString("cd /USER/BIN\n").toUtf8());
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
    QProcess::connect(process,SIGNAL(readyReadStandardError()),this,SLOT(process_ready_read_error()));
    QProcess::connect(process,SIGNAL(readyReadStandardOutput()),this,SLOT(process_ready_read_output()));
    if(current_state == state::downloading)
        process->waitForFinished(); //blocking function to allow for the callback to not encounter a racing error
}

void FtpManager::process_started(){
    qDebug() << "Process has started successfully";
}

/*
 * The ftp process has finished downloading or uploading a file
 */
void FtpManager::process_finished(){
    qDebug() << "Process has finished successfully";
    qDebug() << process->exitCode();
    if(current_state == state::uploading){
        /* now tell the UXG to IMPORT the CSV file (the one you just put on the UXG using ftp) into the current table,
         * then STORE that current table into a fpcs file of the same name as the csv
         * :MEMory:IMPort:ASCii:FPCSetup "<ExistingFilename.csv>"    behaves the same as the "import from file" softbutton on UXG. This populates the current table values with those from the CSV file
         * SOURce:PULM:STReam:FPCSetup:STORe "   behaves the same as the "store to file" softbutton on UXG. This creates a fpcs file out of the current table values
         * SOURce:PULM:STReam:FPCSetup:SELect    This sets the current table file to be the fpcs file we just created
         */
        QString scpiCommand = ":MEMory:IMPort:ASCii:FPCSetup ";
        scpiCommand.append('"');
        scpiCommand.append(uploadingFileOrFolderName);
        scpiCommand.append('"');
        qDebug() << "sending: " << scpiCommand;
        send_SCPI(scpiCommand);

        scpiCommand.clear();
        scpiCommand = "SOURce:PULM:STReam:FPCSetup:STORe ";
        scpiCommand.append('"');
        scpiCommand.append(uploadingFileOrFolderName.remove(".csv"));  //no need to append the .fpcs here, uxg does it automatically
        scpiCommand.append('"');
        qDebug() << "sending: " << scpiCommand;
        send_SCPI(scpiCommand);

        scpiCommand.clear();
        scpiCommand = "SOURce:PULM:STReam:FPCSetup:SELect "; //Note this command will be rejected if the UXG is not in PDW Streaming Mode
        scpiCommand.append('"');
        scpiCommand.append(uploadingFileOrFolderName);
        scpiCommand.append('"');
        qDebug() << "sending: " << scpiCommand;
        send_SCPI(scpiCommand);

        current_state = state::initialized;
    }
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

void FtpManager::send_SCPI(QString message){
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

void FtpManager::connect(quint16 port){
    if(tcpSocket->state() == QAbstractSocket::ConnectedState){
        qDebug() << "Socket already connected";
    }else{
        tcpSocket->connectToHost(hostName,port,QIODevice::ReadWrite);
        QAbstractSocket::connect(tcpSocket,SIGNAL(connected()),this,SLOT(socket_connected()));
        QAbstractSocket::connect(tcpSocket,SIGNAL(disconnected()),this,SLOT(socket_disconnected()));
        QAbstractSocket::connect(tcpSocket,SIGNAL(errorOccurred(QAbstractSocket::SocketError*)),this,SLOT(socket_errorOccurred(QAbstractSocket::SocketError*)));
    }
}

void FtpManager::closeTcpSocket(){
    tcpSocket->close();
}

void FtpManager::abortTcpSocket(){
    tcpSocket->abort();
}

void FtpManager::socket_connected(){
    qDebug() << "Socket connected";
    send_SCPI("DISPlay:REMote ON");
    send_SCPI("INSTrument STReaming");
    UXGSetup();
}

void FtpManager::socket_disconnected(){
    qDebug() << "Socket disconnected";
}

void FtpManager::socket_errorOccurred(QAbstractSocket::SocketError *error){
    qDebug() << "Socket error occurred : ";
    qDebug() << error;
}

//returns true when done with the second phase
void FtpManager::UXGSetup(){
    switch (setup){
    case UXGSetup::Phase1 :{
        send_SCPI(":OUTPut OFF");
        send_SCPI(":OUTPut:MODulation OFF");
        send_SCPI(":INSTrument:SELect STReaming");
        send_SCPI("*OPC?");
        setup = UXGSetup::Phase2;

        break;
    }
    case UXGSetup::Phase2 :{
        send_SCPI(":OUTPut OFF");
        send_SCPI(":STReam:STATe OFF");
        send_SCPI(":STReam:SOURce FILE");
        send_SCPI(":STReam:TRIGger:PLAY:SOURce BUS");
        send_SCPI(":POWer:ATTenuation:BYPass 0");
        send_SCPI(":POWer -100");
        send_SCPI(":STReam:SETup:TIME:AUTO OFF");
        send_SCPI(":STReam:SETup:TIME 0");
        //send_SCPI(":STReam:TRIGger:PLAY:FILE:TYPE SINGle");
        send_SCPI(":STReam:STATe OFF");
        send_SCPI("*OPC?");
        setup = UXGSetup::Phase1;
        UXGSetupFinished=true;
        break;
    }

    }
}








