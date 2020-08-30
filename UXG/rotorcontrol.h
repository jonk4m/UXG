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
    //these timers will resend data if the controller ignores a command
    QTimer *elResendDataTimer;
    QTimer *azResendDataTimer;
    //Used to connect signals from this class to slots in MainWindow
    QMainWindow *window;
    //holds the serial ports if/when they are found
    QSerialPort *az;
    QSerialPort *el;
    //When using "recommended Rotor Speeds," This lets the program know that the controller is ready after a test has been started
    bool elSpeedsCorrectForTest=false;
    bool azSpeedsCorrectForTest=false;
    //a "simple" test is one that only takes resolution, min/max elevation, and min/azimuth and creates a grid
    bool simpleTestStarted = false;
    //an "advanced" test is one that explicitly dictates every position in a .txt file
    bool advancedTestStarted = false;
    //Used for "drone" or "follow" test.  If the rotor cannot keep up to the demands of the UDP stream, a small window will display saying "Can't keep Up"
    bool cantKeepUp=false;
    //true if a "drone" or "follow" test is started
    bool droneTestStarted = false;
    //holds the value that the rotor is heading too
    double elevationHeading;
    double azimuthHeading;
    //holds current rotor position, updated every 200 ms
    double elevationPosition;
    double azimuthPosition;
    //when a simple test is started, these variables hold the values inputted by the user
    double maxAzimuth = -1;
    double minAzimuth = -1;
    double maxElevation = -1;
    double minElevation = -1;
    double azResolution = -1;
    double elResolution = -1;
    //the refresh rate of the UDP stream. Used in a calculation to determine whether the rotor can keep up
    double refreshRate= 1.0;
    //true if the "stop Motion" button is pressed
    bool stoppingMotion = false;
    //Used in setup code to determine the original heading of the rotor
    bool elevationHeadingCreated= false;
    bool azimuthHeadingCreated = false;
    //used in the simple test to know which direction azimuth is moving
    bool azimuthGoingUp=true;
    //used in simple test to know when max elevation is hit and the azimuth has to sweep one more time
    bool lastAzimuthRun=false;
    //records the previous position received by UDP for a cant keep up calculation
    double lastDroneAzPosition=0;
    double lastDroneElPosition=0;
    //holds the last command sent to the rotor in case it needs to be resent
    QByteArray elLastCommand;
    QByteArray azLastCommand;
    //holds the list of positions required during an advanced test
    QList<QString> positionList;
    //index of the position list
    int positionListIndex = 0;
    //sometimes on program startup the rotor would move so once this variable reaches 5 (1 second after program starts) then rotor commands can be sent and received
    int dummyVariable=0;
    //when checking if speed has been returned, this boolean checks whether elevation speed was requested or azimuth speed
    bool isElevationSpeedRequested=true;
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

    bool manageDroneTest(double az, double el, double refreshRate);

    QString fixHeadings(QString moveValue);

    void setElevationMode(bool isElevation);

    bool setModeHelper(QByteArray command, QSerialPort *serialPort, bool isElevation);

    ParameterAndWriteNumber parseReceivedRotorData(QByteArray receivedData);

    void timerTimeout(bool isElevation);

    void rampTimerTimeout();

    void minSpeedTimerTimeout();

    void maxSpeedTimerTimeout();

signals:
     void userMessage(QString message);

private:
     //timers to that are set when speed is requested, and if it timeouts, the program requests the speed again.
     QTimer *maxSpeedTimer;
     QTimer *minSpeedTimer;
     QTimer *rampTimer;

};




#endif // RotorControl_H
