#ifndef RotorControl_H
#define RotorControl_H


#include <QSerialPort>
#include <QMainWindow>
#include <QSerialPortInfo>
#include <QTimer>
class RotorControl: public QObject
{
Q_OBJECT

public:
    QTimer *elResendDataTimer;
    QTimer *azResendDataTimer;
    QMainWindow *window;
    QSerialPort *az;
    QSerialPort *el;
    bool elSpeedsCorrectForTest=false;
    bool azSpeedsCorrectForTest=false;
    bool simpleTestStarted = false;
    bool advancedTestStarted = false;
    bool cantKeepUp=false;
    bool droneTestStarted = false;
    double elevationHeading;
    double azimuthHeading;
    double elevationPosition;
    double azimuthPosition;
    double maxAzimuth = -1;
    double minAzimuth = -1;
    double maxElevation = -1;
    double minElevation = -1;
    double azResolution = -1;
    double elResolution = -1;
    double refreshRate= 1.0;
    bool stoppingMotion = false;
    bool elevationHeadingCreated= false;
    bool azimuthHeadingCreated = false;
    bool azimuthGoingUp=true;
    bool lastAzimuthRun=false;
    QByteArray elLastCommand;
    QByteArray azLastCommand;
    QList<QString> positionList;
    int positionListIndex = 0;
    int dummyVariable=0;
//    QByteArray elRead;
//    QByteArray azRead;

    enum controllerParameter {notReady, maxSpeed, minSpeed, ramp, position, elevationMode, pulseDivider, CWLimit, CCWLimit};
    struct ParameterAndWriteNumber{
        public:
         controllerParameter parameter;
         double writeNumber;
         ParameterAndWriteNumber(controllerParameter parameter, double writeNumber){
             this->parameter = parameter;
             this->writeNumber = writeNumber;
         }
    };
    RotorControl(QMainWindow *window);

    bool findSerialPorts();

    void getPosition();

    void write(QByteArray command, bool isElevation);

    void getMaxMinAndRampValues(bool isElevation);

    void moveRotor(QString moveValue, bool isElevation, bool isPosition);

    void changeMotorSpeed(int motorSpeed, controllerParameter motor, bool isElevation);

    void closeSerialPorts();

    int readSerial(QByteArray message);

    QString createRotorCommand(QString moveValue);

    void stopMotion();

    ParameterAndWriteNumber serialRead(bool isElevation);

    ParameterAndWriteNumber parseSerialRead(controllerParameter parameter, QByteArray serialRead);

    void startSimpleTest(double maxAzimuth, double minAzimuth, double maxElevation, double minElevation, double azResolution,double elResolution);

    bool headingEqualsPosition();

    bool manageSimpleGridTest();

    void startAdvancedTest(QList<QString> positionList);

    bool manageAdvancedTest();

    void setHeadingsToCurrentPosition();

    bool manageDroneTest(int az, int azSpeed, int el, int elSpeed, double refreshRate);

    QString fixHeadings(QString moveValue);

    void setElevationMode();

    bool setModeHelper(QByteArray command, QSerialPort *serialPort);

    ParameterAndWriteNumber parseReceivedRotorData(QByteArray receivedData);

    void timerTimeout(bool isElevation);



signals:
     void userMessage(QString message);

};




#endif // RotorControl_H
