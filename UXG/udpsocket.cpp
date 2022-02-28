#include "udpsocket.h"

UdpSocket::UdpSocket(QMainWindow *window)
{
    socket = new QUdpSocket(window);


    QMainWindow::connect(socket, SIGNAL(readyRead()), window, SLOT(UdpRead()));
}
//Finds the current IPv4 address of the ethernet port on the computer and binds the UDP socket to it on port 5005
QList<QString>* UdpSocket::getIPAddressAndPort(){
    bool ethernetFound=false;
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
        if(address.protocol()==QAbstractSocket::IPv6Protocol&&address.scopeId().startsWith("ethernet")){
            ethernetFound=true;

        }
        else if (ethernetFound&&address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost)) {

            socket->bind(QHostAddress(address.toString()),5005);
            isBound=true;
            qDebug() << address.toString();
        }
    }
    QString IP = socket->localAddress().toString();
    QString port = QString::number(5005);
    QList<QString> *list = new QList<QString>();
    list->append(IP);
    list->append(port);
    return list;
}

void UdpSocket::closeUdpSocket(){
    socket->close();
}

void UdpSocket::writeData(QString data){
        QByteArray writeData = data.toUtf8();
        writeData+="\n";
        socket->writeDatagram(writeData,NMEAIP,NMEAPort);
 }

QString UdpSocket::readData(){
    QByteArray buffer;
    buffer.resize(socket->pendingDatagramSize());
    QHostAddress sender;
    quint16 senderPort;

    socket->readDatagram(buffer.data(), buffer.size(), &sender, &senderPort);
    return QString(buffer);
}

void UdpSocket::setIPAddress(QHostAddress address, int port){

        NMEAIP = address;
        NMEAPort=port;

}
