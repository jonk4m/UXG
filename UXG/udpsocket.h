#ifndef UDPSOCKET_H
#define UDPSOCKET_H


#include <QTableWidget>
#include <QHostAddress>
#include <QUdpSocket>
#include <QMainWindow>
#include <QNetworkInterface>
class UdpSocket
{
public:
    bool isBound = false;
    QHostAddress UXGIP;
    quint16 UXGPort;
    QHostAddress NMEAIP;
    quint16 NMEAPort;


    enum possibleRecipients {UXG, NMEA};
    QUdpSocket *socket;
    UdpSocket(QMainWindow *window);
    void closeUdpSocket();
    void writeData(QString data, possibleRecipients recipient);
    QString readData();
    void setIPAddresses(QHostAddress address, int port, possibleRecipients recipient);
    QList<QString>* getIPAddressAndPort();
};

#endif // UDPSOCKET_H
