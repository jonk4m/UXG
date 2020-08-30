#include "rotorcontrol.h"

RotorControl::RotorControl(QMainWindow *window)
{
    //userMessage is a custom signal that helps output console message outside of mainwindow.cpp
    QMainWindow::connect(this, SIGNAL(userMessage(QString)), window, SLOT(output_to_console(QString)));
    //initialize timers
    elResendDataTimer= new QTimer(window);
    azResendDataTimer = new QTimer(window);
    QMainWindow::connect(elResendDataTimer, SIGNAL(timeout()), window, SLOT(resendTimerTimeout()));
    QMainWindow::connect(azResendDataTimer, SIGNAL(timeout()), window, SLOT(resendTimerTimeout()));
    az = new QSerialPort(window);
    el = new QSerialPort(window);
    QMainWindow::connect(el, SIGNAL(readyRead()), window, SLOT(serialRead()));
    QMainWindow::connect(az, SIGNAL(readyRead()), window, SLOT(serialRead()));
    maxSpeedTimer = new QTimer(this);
    minSpeedTimer = new QTimer(this);
    rampTimer = new QTimer(this);
    this->window=window;

}

/*
 * finds the serial ports for elevation and azimuth and sets them accordingly. Checks the elevation mode of each port.
 * The elevation port should have elevation mode set and azimuth port should not have elevation mode set
 * both communication lines are 4800 baud, 8bit data, no parity, one stop, and no flow control
 * returns true if 1 or 2 ports are found, otherwise it returns false
 * */
bool RotorControl::findSerialPorts(){
    emit userMessage("Finding Serial Ports");
    closeSerialPorts();
    QSerialPort *serialPort = new QSerialPort();
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();//creates a list of current serial ports
    QString elPort;
    QString azPort;
    if(ports.isEmpty()){
        //no ports are found so return false
        return false;
    }
    for(const QSerialPortInfo &port : ports){
        QString s = port.portName();
        serialPort->setPortName(s);
        serialPort->setBaudRate(QSerialPort::Baud4800);
        serialPort->setDataBits(QSerialPort::Data8);
        serialPort->setParity(QSerialPort::NoParity);
        serialPort->setStopBits(QSerialPort::OneStop);
        serialPort->setFlowControl(QSerialPort::NoFlowControl);
        serialPort->open(QIODevice::ReadWrite);

        bool receivedData=false;
        QByteArray data = "";
        while(!receivedData){          
            serialPort->write("RT0;"); //asks port whether or not it is in elevation mode

            while(serialPort->waitForReadyRead(100)){
                //message is received in parts so this adds to the buffer until it is completed
                data+=serialPort->readAll();
                receivedData=true;
            }
        }

        if(data=="T\0010;"){//not in elevation mode
            azPort = s;
        }else if(data=="T\0018;"){//is in elevation mode
            elPort = s;
        }
        serialPort->close();
    }
    az->setPortName(azPort);
    az->setBaudRate(QSerialPort::Baud4800);
    az->setDataBits(QSerialPort::Data8);
    az->setParity(QSerialPort::NoParity);
    az->setStopBits(QSerialPort::OneStop);
    az->setFlowControl(QSerialPort::NoFlowControl);
    az->open(QIODevice::ReadWrite);


    el->setPortName(elPort);
    el->setBaudRate(QSerialPort::Baud4800);
    el->setDataBits(QSerialPort::Data8);
    el->setParity(QSerialPort::NoParity);
    el->setStopBits(QSerialPort::OneStop);
    el->setFlowControl(QSerialPort::NoFlowControl);
    el->open(QIODevice::ReadWrite);
    //asks for speeds
    getMaxMinAndRampValues(true);
    emit userMessage("Serial Ports Found");
    return true;
}

/*Must be used when only one USB is plugged in.  If isElevation is true, then set the port to elevation mode.
 * Else turn off elevation mode
 * */
void RotorControl::setElevationMode(bool isElevation){
    closeSerialPorts();
    QSerialPort *serialPort = new QSerialPort();
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();//creates a list of serial ports
    QString elPort;
    for(const QSerialPortInfo &port : ports){
        QString s = port.portName();
        serialPort->setPortName(s);
        serialPort->setBaudRate(QSerialPort::Baud4800);
        serialPort->setDataBits(QSerialPort::Data8);
        serialPort->setParity(QSerialPort::NoParity);
        serialPort->setStopBits(QSerialPort::OneStop);
        serialPort->setFlowControl(QSerialPort::NoFlowControl);
        serialPort->open(QIODevice::ReadWrite);

        bool isOnCorrectElevationMode=false;
        QByteArray command;
        if(isElevation){
            command = "WT08;";
        }else{
            command = "WT00;";
        }
        while(!isOnCorrectElevationMode){
            serialPort->write(command);
            QByteArray data = "";
            while(serialPort->waitForReadyRead(500)){
                data+=serialPort->readAll();

            }
            isOnCorrectElevationMode = setModeHelper("RT0;", serialPort, isElevation);
        }
    }
    serialPort->close();
}

//Helps setElevationMode to split up the code a bit, checks whether the elevation setting is correct
bool RotorControl::setModeHelper(QByteArray command, QSerialPort *serialPort, bool isElevation){
    serialPort->write(command);
    QString data = "";
    while(serialPort->waitForReadyRead(500)){
        data+=serialPort->readAll();

    }
    if(data == "T\0018;"&&isElevation){
        return true;
    }else if(data == "T\0010;"&&!isElevation){
        return true;
    }
    return false;
}




//writes to the serial ports to read the position
void RotorControl::getPosition(){
    //requests position with decimal
    el->write("BI0;");
    az->write("BI0;");
}

/*
 * takes three parameters, moveValue: which is taken from the ui lineEdit, isElevation: which
 * checks if the elevation radio button is check, and isPosition, which takes if the position
 * radio button is checked.  For example: if isElevation is true and isPosition is false, then the
 * elevation is moved by an offset of moveValue. So if the elevation is at 120 and moveValue is 5
 * then elevation will move to 125
 * */
void RotorControl::moveRotor(QString moveValue, bool isElevation, bool isPosition){
    if(!moveValue.isEmpty()){
        //moves to absolute position
        if(isPosition){
            QString commandValue = RotorControl::createRotorCommand(moveValue);//takes the string from the line edit and turns it
                                                                               //into a string usable for the controller
            QString serialWrite = "AP0" + commandValue+ "\r;";
            if(isElevation){
                elevationHeading= commandValue.toDouble();
                el->write(serialWrite.toUtf8());

            }else{
                azimuthHeading= commandValue.toDouble();
                az->write(serialWrite.toUtf8());
            }
        //moves to an offset of current position
        }else{
            if(isElevation){
                elevationHeading = elevationHeading + moveValue.toDouble();
                QString offsetString = QString::number(elevationHeading);
                QString commandValue = RotorControl::createRotorCommand(offsetString);
                QString serialWrite = "AP0" + commandValue + "\r;";
                el->write(serialWrite.toUtf8());
            }else{
                azimuthHeading = azimuthHeading + moveValue.toDouble();
                QString offsetString = QString::number(azimuthHeading);
                QString commandValue = RotorControl::createRotorCommand(offsetString);
                QString serialWrite = "AP0" + commandValue + "\r;";
                az->write(serialWrite.toUtf8());
            }
        }
    }
}

/*when a motor speed slider is released, this method is called.  It writes
 * to the controller what the new motorspeed is.  The parameters are the new motor speed,
 * and enum of which motor speed is being changed: maxSpeed, minSpeed, or ramp, and whether
 * we are changing the elevation or azimuth
 * */
void RotorControl::changeMotorSpeed(int motorSpeed, controllerParameter motor, bool isElevation){

    switch(motor){
    case controllerParameter::maxSpeed :{
        std::string maxSpeed = "WG0" + std::to_string(motorSpeed) + ";";
        if(isElevation){
            el->write(maxSpeed.c_str());
        }else{
            az->write(maxSpeed.c_str());
        }
        break;
    }
    case controllerParameter::minSpeed :{
        std::string minSpeed = "WF0" + std::to_string(motorSpeed) + ";";
        if(isElevation){
            el->write(minSpeed.c_str());
        }else{
            az->write(minSpeed.c_str());
        }
        break;
    }
    case controllerParameter::ramp : {
        std::string ramp = "WN0" + std::to_string(motorSpeed) + ";";
        if(isElevation){
            el->write(ramp.c_str());
        }else{
            az->write(ramp.c_str());
        }
        break;
    }
    }

}

//closes serial ports, called on ui closing.
void RotorControl::closeSerialPorts(){
    az->close();
    el->close();
}

//the length of the integer part of the command is always 3 bytes
//so this function just adds some extra zeroes on the front if necessary
QString RotorControl::createRotorCommand(QString moveValue){
    QString fixedValue = fixHeadings(moveValue);//the rotor can only take values up to a third of a degree
                                                //this function takes moveValue and rounds it to the closest value the rotor can use
    if(fixedValue.toDouble()<0){
        return "";
    }

    if(fixedValue.length()==1){
        QString returnString = "00" + fixedValue;
        return returnString;
    }

    if(fixedValue.contains('.')){
        if(fixedValue.length()==5){
            return fixedValue;
        }else if(fixedValue.length()==4){
            QString returnString = "0" + fixedValue;
            return returnString;
        }else if(fixedValue.length()==3){
            QString returnString = "00" + fixedValue;
            return returnString;
        }else if(fixedValue.length()==2){
            QString returnString = "000" + fixedValue;
            return returnString;
        }
    }else{
        if(fixedValue.length()==3){
            return fixedValue;
        }else if(fixedValue.length()==2){
            QString returnString = "0" + fixedValue;
            return returnString;
        }
    }

    return "";
}
//stops all motion of the rotor
void RotorControl::stopMotion(){
    el->write(";");
    az->write(";");
}

/* Used when new information is received over serial. Based on the received value, returns an enum
 * that holds the type of information received
 * */
RotorControl::ParameterAndWriteNumber RotorControl::parseReceivedRotorData(QByteArray receivedData){

    receivedData.chop(1);
    if(receivedData.endsWith('T')){
        ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::elevationMode, -1);
        return returnValue;
    }else if(receivedData.endsWith('K')){
        ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::pulseDivider, -1);
        return returnValue;
    }else if(receivedData.endsWith('I')){
        ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::CWLimit, -1);
        return returnValue;
    }else if(receivedData.endsWith('H')){
        ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::CCWLimit, -1);
        return returnValue;
    }
    else if(receivedData.endsWith('G')){
        ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::maxSpeed, -1);
        return returnValue;
    }
    else if(receivedData.endsWith('F')){
        ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::minSpeed, -1);
        return returnValue;
    }
    else if(receivedData.endsWith('N')){
        ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::ramp, -1);
        return returnValue;
    }else{
        ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::notReady, -1);
        return returnValue;
    }

}

/*returns a simple struct that consists of an enum type and an int.  The user who calls this function would
 * probably have to use a case statement to check which enum type is being returned to figure what item on the gui
 * is being changed.  For example if the return struct has the enum type maxSpeed, then the maxSpeed slider will get
 * the int value that goes along with the struct.  If the enum was position, then the LCD display gets updated
 * */
RotorControl::ParameterAndWriteNumber RotorControl::serialRead(bool isElevation){
    QSerialPort *serial;
    if(isElevation){
        serial= el;
    }else{
        serial= az;
    }
    if(true){
        QByteArray elevation = serial->peek(100);//check what is in the buffer without clearing it
        QString elString = QString(elevation);
        if(elevation.endsWith(';')){//everything received from the rotor ends in ';'
            if(elevation.startsWith('G')){
                QByteArray maxSpeed = serial->read(4);
                if(!maxSpeed.endsWith(';')){
                    maxSpeed = maxSpeed+serial->read(1);
                }
                return parseSerialRead(controllerParameter::maxSpeed, maxSpeed);
            }else if(elevation.startsWith('F')){
                QByteArray minSpeed = serial->read(4);
                if(!minSpeed.endsWith(';')){
                    minSpeed = minSpeed+serial->read(1);
                }
                return parseSerialRead(controllerParameter::minSpeed, minSpeed);
            }else if(elevation.startsWith('N')){
                return parseSerialRead(controllerParameter::ramp, serial->read(4));
            }else if(elevation.startsWith('\006')){
                if(isElevation){
                    elResendDataTimer->stop();
                }else{
                    azResendDataTimer->stop();
                }

                return parseReceivedRotorData(serial->read(3));

            }else if(elevation.startsWith('T')){
                return parseSerialRead(controllerParameter::elevationMode, serial->read(4));
            }else if(elevation.startsWith('I')){
                return parseSerialRead(controllerParameter::CWLimit, serial->read(4));
            }else if(elevation.startsWith('H')){
                return parseSerialRead(controllerParameter::CCWLimit, serial->read(4));
            }else if(elevation.startsWith('K')){
                return parseSerialRead(controllerParameter::pulseDivider, serial->read(7));
            } else{
                ParameterAndWriteNumber returnValue = parseSerialRead(controllerParameter::position, serial->readAll());
                if(isElevation){
                    elevationPosition=returnValue.writeNumber;
                    if(!elevationHeadingCreated&&elevationPosition!=0){
                        elevationHeading=elevationPosition;
                        elevationHeadingCreated=true;
                    }
                }else{
                    azimuthPosition=returnValue.writeNumber;
                    if(!azimuthHeadingCreated&&azimuthPosition != 0){
                        azimuthHeading=azimuthPosition;
                        azimuthHeadingCreated=true;
                    }
                }

                return returnValue;
            }
        }
    }
    QByteArray notReady = "-1";
    return parseSerialRead(controllerParameter::notReady,notReady);
}

/*Takes in the serial that is read and the enum type controllerParameter to parse the string into an in that
 * can be displayed on the gui. Called from serialRead to keep things clean
 * */
RotorControl::ParameterAndWriteNumber RotorControl::parseSerialRead(controllerParameter parameter, QByteArray serialRead){
    serialRead.chop(1);
    switch(parameter){
    case(controllerParameter::maxSpeed) :{
        if(serialRead.endsWith('0')){
            ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::maxSpeed, 10);

            return returnValue;
        }else{
            ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::maxSpeed, serialRead.right(1).toInt());
            return returnValue;
        }
        maxSpeedTimer->stop();
    }
    case(controllerParameter::minSpeed) :{
        if(serialRead.endsWith('0')){
            ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::minSpeed, 10);
            return returnValue;
        }else{
            ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::minSpeed, serialRead.right(1).toInt());
            return returnValue;
        }
        minSpeedTimer->stop();
    }
    case(controllerParameter::ramp) :{
        ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::ramp, serialRead.right(1).toInt());
        rampTimer->stop();
        return returnValue;
    }
    case(controllerParameter::position) :{
        ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::position, serialRead.toDouble());
        return returnValue;
    }
    case(controllerParameter::elevationMode):{
        ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::elevationMode, serialRead.right(1).toInt());
        return returnValue;
    }
    case(controllerParameter::CWLimit):{
        ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::CWLimit, serialRead.right(3).toInt());
        return returnValue;
    }
    case(controllerParameter::CCWLimit):{
        ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::CCWLimit, serialRead.right(3).toInt());
        return returnValue;
    }
    case(controllerParameter::pulseDivider):{
        ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::pulseDivider, serialRead.right(4).toInt());
        return returnValue;
    }
    case(controllerParameter::notReady):{
        ParameterAndWriteNumber returnValue = ParameterAndWriteNumber(controllerParameter::notReady, -1);
        return returnValue;
    }
    }
}
//writes to the RT-21 saying that we want to read the max speed, min speed, and ramp value of the rotor
void RotorControl::getMaxMinAndRampValues(bool isElevation){
    if(isElevation){
        el->write("RG0;");
        el->write("RF0;");
        el->write("RN0;");
        isElevationSpeedRequested=true;

    }else{
        az->write("RG0;");
        az->write("RF0;");
        az->write("RN0;");
        isElevationSpeedRequested=false;
    }
    rampTimer->start(200);
    minSpeedTimer->start(200);
    maxSpeedTimer->start(200);

}

/* sets the correct fields for the simple test and moves the rotor to its first position
 * */

void RotorControl::startSimpleTest(double minAzimuth, double maxAzimuth, double minElevation, double maxElevation, double azResolution,double elResolution){
    this->maxAzimuth=maxAzimuth;
    this->minAzimuth=minAzimuth;
    this->maxElevation=maxElevation;
    this->minElevation=minElevation;
    this->azResolution=azResolution;
    this->elResolution = elResolution;
    QString min = QString::number(this->minAzimuth);
    moveRotor(min, false,true);
    min = QString::number(this->minElevation);
    moveRotor(min, true,true);
    emit userMessage("Starting Simple Test");
}

//Takes in a raw value that will be commanded to the rotor and rounds it to the nearest value
//that the controller can input
QString RotorControl::fixHeadings(QString moveValue){



    if(moveValue.contains('.')){
        bool roundUp =false;
        //remove any numbers past the tenth place
        int decimalIndex = moveValue.indexOf('.');
        moveValue.chop(moveValue.length()-decimalIndex-2);
        //force rounding to ".3" ".6" or ".0"
        QString lastChar = moveValue.right(1);
        int decimal = lastChar.toInt();
        if(decimal==1){
            decimal = 0;
        }else if(decimal==2||decimal==4){
            decimal =3;
        }else if(decimal == 5||decimal == 7){
            decimal = 6;
        }else if(decimal ==8||decimal == 9){
            roundUp=true;
            decimal = 0;
        }

        if(roundUp){
            moveValue.chop(2);
            moveValue = QString::number(moveValue.toDouble()+1)+"."+ QString::number(decimal);

        }else{
            moveValue.chop(1);
            moveValue = moveValue + QString::number(decimal);
        }
    }
    return moveValue;

}
//if the heading (position the rotor is moving to) is equal to the current position, return true
//otherwise return false
bool RotorControl::headingEqualsPosition(){
    //sometimes rotor ignores command for some reason, so this makes
    //the code keep sending the same command until it gets there
    bool azimuthAtPosition = (azimuthHeading==azimuthPosition);
    bool elevationAtPosition = (elevationHeading==elevationPosition);
    if(dummyVariable==5&&(advancedTestStarted||simpleTestStarted)){
        if(!azimuthAtPosition){
            QString offsetString = QString::number(azimuthHeading);
            moveRotor(offsetString,false,true);
        }else if(!elevationAtPosition){
            QString offsetString = QString::number(elevationHeading);
            moveRotor(offsetString,true,true);
        }
    }else if(dummyVariable<5){
        dummyVariable++;
    }
    return azimuthAtPosition&&elevationAtPosition;
}

//when a simple test is called, this steps the rotor through each position using the fields that were
//setup in the startSimpleTest function
bool RotorControl::manageSimpleGridTest(){
    if(elevationPosition>=maxElevation&&(azimuthPosition>=maxAzimuth||azimuthPosition<=minAzimuth)&&lastAzimuthRun){
        lastAzimuthRun=false;
        azimuthGoingUp=true;
        return true;

    }else if(elevationPosition>=maxElevation&&(azimuthPosition>=maxAzimuth||azimuthPosition<=minAzimuth)&&!lastAzimuthRun){
        lastAzimuthRun=true;
    }
    if(azimuthGoingUp){
        QString azResolution = QString::number(this->azResolution);
        QString elResolution = QString::number(this->elResolution);
        if(azimuthPosition>=maxAzimuth){

            moveRotor(elResolution, true, false);
            azimuthGoingUp=false;
        }else{
            moveRotor(azResolution,false,false);
        }
    }else{

        if(azimuthPosition<=minAzimuth){
            QString resolution = QString::number(this->elResolution);
            moveRotor(resolution, true, false);
            azimuthGoingUp=true;
        }else{
            QString resolution = QString::number(-this->azResolution);
            moveRotor(resolution,false,false);
        }
    }

    return false;
}
//Takes in a list of positions from a .txt file and sets RotorControl's position list equal to it.
//moves the rotor to it's first position
void RotorControl::startAdvancedTest(QList<QString> positionList){
    positionListIndex = 0;
    this->positionList=positionList;
    manageAdvancedTest();
}
//runs through each position from positionList, returns true when the last position is reached.
bool RotorControl::manageAdvancedTest(){
    if(positionListIndex==positionList.size()){
        positionList.clear();
        positionListIndex=0;
        return true;
    }
    QList<QString> currentPositions = positionList.at(positionListIndex).split(',');
    moveRotor(currentPositions.at(0),false,true);
    moveRotor(currentPositions.at(1),true,true);
    positionListIndex++;

    return false;
}

void RotorControl::setHeadingsToCurrentPosition(){
    azimuthHeading = azimuthPosition;
    elevationHeading=elevationPosition;
}

//returns true if the rotor cant keep up, otherwise returns false
bool RotorControl::manageDroneTest(double az, double el, double refreshRate){
    bool canKeepUp= true;
    double azSpeed = abs((lastDroneAzPosition -az)*refreshRate);
    double elSpeed = abs((lastDroneElPosition -el)*refreshRate);
    //rotor can move about 5.5 degrees/second at max speed
    //so if position plus speed (without having to take refresh rate into account)
    //is greater than 5.5 return true, saying that the rotor can't keep up.
    if(azSpeed>5.5||elSpeed>5.5){
        canKeepUp=false;
    }
    moveRotor(QString::number(az+180), false, true);
    moveRotor(QString::number(el+180), true, true);
    lastDroneAzPosition = az;
    lastDroneElPosition = el;
    if(canKeepUp){
        return false;
    }else{
        return true;
    }

}
//writes the command to the correct port and starts a timer that, if it times out, will resend the last instruction
void RotorControl::write(QByteArray command, bool isElevation){
    if(isElevation){
        el->write(command);
        elLastCommand = command;
        elResendDataTimer->start(200);
    }else{
        az->write(command);
        azLastCommand = command;
        azResendDataTimer->start(200);
    }
}
//all of these timeout functions resend data if their respective timers timeout
void RotorControl::timerTimeout(bool isElevation){
    if(isElevation){
        elLastCommand.chop(1);
        write(elLastCommand,true);
    }else {
        azLastCommand.chop(1);
        write(azLastCommand,false);
    }

}

void RotorControl::maxSpeedTimerTimeout(){
    if(isElevationSpeedRequested){
        el->write("RG0;");
    }else{
        az->write("RG0;");
    }
}

void RotorControl::minSpeedTimerTimeout(){
    if(isElevationSpeedRequested){
        el->write("RF0;");
    }else{
        az->write("RF0;");
    }
}

void RotorControl::rampTimerTimeout(){
    if(isElevationSpeedRequested){
        el->write("RN0;");
    }else{
        az->write("RN0;");
    }
}
