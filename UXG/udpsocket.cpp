#include "udpsocket.h"

UdpSocket::UdpSocket(QMainWindow *window)
{
    socket = new QUdpSocket(window);

//    socket->bind(QHostAddress("169.254.83.245"),50344);
    QMainWindow::connect(socket, SIGNAL(readyRead()), window, SLOT(UdpRead()));
}

QList<QString>* UdpSocket::getIPAddressAndPort(){
    bool ethernetFound=false;
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
        if(address.protocol()==QAbstractSocket::IPv6Protocol&&address.scopeId().startsWith("ethernet")){
            ethernetFound=true;

        }
        else if (ethernetFound&&address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost)) {

            socket->bind(QHostAddress(address.toString()),100);
            isBound=true;
            qDebug() << address.toString();
        }
    }
    QString IP = socket->localAddress().toString();
    QString port = QString::number(100);
    QList<QString> *list = new QList<QString>();
    list->append(IP);
    list->append(port);
    return list;
}

void UdpSocket::closeUdpSocket(){
    socket->close();
}

void UdpSocket::writeData(QString data, possibleRecipients recipient){
    switch(recipient){
    case(UXG):{
        QByteArray writeData = data.toUtf8();
        writeData+="\n";
        socket->writeDatagram(writeData,UXGIP,UXGPort);
        break;
    }
    case(NMEA):{
        QByteArray writeData = data.toUtf8();
        writeData+="\n";
        socket->writeDatagram(writeData,NMEAIP,NMEAPort);
        break;
    }


    }
}

QString UdpSocket::readData(){
    QByteArray buffer;
    buffer.resize(socket->pendingDatagramSize());
    QHostAddress sender;
    quint16 senderPort;

    socket->readDatagram(buffer.data(), buffer.size(), &sender, &senderPort);
    return QString(buffer);
}

void UdpSocket::setIPAddresses(QHostAddress address, int port, possibleRecipients recipient){

    switch(recipient){
    case(UXG):{
        UXGIP = address;
        UXGPort=port;
        break;
    }
    case(NMEA):{
        NMEAIP = address;
        NMEAPort=port;
        break;
    }

    }

}
