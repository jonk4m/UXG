#ifndef UDPSOCKET_H
#define UDPSOCKET_H


#include <QTableWidget>
#include <QHostAddress>
#include <QUdpSocket>
#include <QMainWindow>
#include <QNetworkInterface>
class UdpSocket: public QObject
{
Q_OBJECT
public:
    bool isBound = false;
    QHostAddress NMEAIP;
    quint16 NMEAPort;

    QUdpSocket *socket;
    UdpSocket(QMainWindow *window);
    void closeUdpSocket();
    void writeData(QString data);
    QString readData();
    void setIPAddress(QHostAddress address, int port);
    QList<QString>* getIPAddressAndPort();
};

#endif // UDPSOCKET_H
