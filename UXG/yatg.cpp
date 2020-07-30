#include "yatg.h"

YATG::YATG(){
    //constructor
}

YATG::YATG(FtpManager *ftpManager, QMainWindow *window){
    this->ftp_manager = ftpManager;
    QMainWindow::connect(this, SIGNAL(userMessage(QString)), window, SLOT(output_to_console(QString)));
}

bool YATG::upload_file_to_uxg(){
    //set the WorkingFile to be the file at the WorkingFilePath

    workingFile.close(); //if a different file was already open, this will close it so we can select a new file

    workingFile.setFileName(workingFilePath); //set this file path to be for the working file
    bool exists = workingFile.exists(workingFilePath);
    if(exists){
        if(!workingFile.open(QFile::ReadWrite | QFile::Text | QFile::ExistingOnly)) //Options: ExistingOnly, NewOnly, Append
        {
            emit userMessage("Could not open File : " + workingFilePath);
            qDebug() << "Could not open File : " + workingFilePath;
            return false;
        }
    }else{
        emit userMessage("File Not Found in Local System to upload to UXG: " + workingFilePath);
        qDebug() << "File Not Found in Local System to upload to UXG: " << workingFilePath;
        return false;
    }

    streamer.setDevice(&workingFile); //set the streamer for this file

    //Parse through the header
    streamer.seek(0);
    QStringList readHeader = streamer.readLine().remove("\n").split(QRegExp(","), Qt::SkipEmptyParts);
    //qDebug() << "header of YATG : " << readHeader;
    QHash<QString,int> indexes;
    QHash<QString,QString> units;
    double timeScalingFactor = 0;
    for(QString temp : readHeader){
        if(temp.contains("Freq",Qt::CaseInsensitive)){
            indexes.insert("Freq",readHeader.indexOf(temp));
            //capture freq units
            QString unit = readHeader.at(readHeader.indexOf(temp));
            bool containsM = unit.contains('M');
            bool containsm = unit.contains('m');
            bool containsK = unit.contains('K',Qt::CaseInsensitive);
            bool containsG = unit.contains('G',Qt::CaseInsensitive);
            if((containsG && containsM) || (containsG && containsK) || (containsM && containsK)){
                emit userMessage("Multiple units imputted for the frequency column. Plase choose only one unit, i.e. 'M', 'G', or 'k'. unit was: " + unit);
                qDebug() << "Multiple units imputted for the frequency column. Plase choose only one unit, i.e. 'M', 'G', or 'k'. unit was: " + unit;
                return false;
            }
            if(containsm){
                emit userMessage("milliHz is not a valid option for the frequency units. Please modify your YATG file and try again.");
                qDebug() << "milliHz is not a valid option for the frequency units. Please modify your YATG file and try again.";
                return false;
            }
            if(containsG)
                units.insert("Freq"," (GHz),");
            if(containsM)
                units.insert("Freq"," (MHz),");
            if(containsK)
                units.insert("Freq"," (kHz),");
            if(!containsG && !containsM && !containsK)
                units.insert("Freq"," (Hz),");
        }else if(temp.contains("Pri",Qt::CaseInsensitive)){
            indexes.insert("Pri",readHeader.indexOf(temp));
            //capture pri units
            QString unit = readHeader.at(readHeader.indexOf(temp));
            bool containsn = unit.contains('n',Qt::CaseInsensitive);
            bool containsu = unit.contains('u',Qt::CaseInsensitive);
            bool containsm = unit.contains('m',Qt::CaseInsensitive); //even though this really should be case sensitive (milli versus mega) the UXG exports in all caps so this accounts for that
            if((containsn && containsu) || (containsn && containsm) || (containsu && containsm)){
                emit userMessage("Multiple units imputted for the PRI column. Plase choose only one unit, i.e. 'n', 'u', or 'm', unit was: " + unit);
                qDebug() << "Multiple units imputted for the PRI column. Plase choose only one unit, i.e. 'n', 'u', or 'm', unit was: " + unit;
                emit userMessage("booleans were: " + QString(containsn) + QString(containsu) + QString(containsm));
                qDebug() << "booleans were: " << containsn << containsu << containsm;
                return false;
            }
            if(containsn){
                units.insert("Pri"," (ns),");
                timeScalingFactor = 1000000000; //from sec to nano
            }else if(containsu){
                units.insert("Pri"," (us),");
                timeScalingFactor = 1000000; //from sec to micro
            }else if(containsm){
                units.insert("Pri"," (ms),");
                timeScalingFactor = 1000; //from sec to milli
            }else if(!containsn && !containsu && !containsm){
                units.insert("Pri"," (s),");
                timeScalingFactor = 1; //from sec to sec
            }
        }else if(temp.contains("Pw",Qt::CaseInsensitive)){
            indexes.insert("Pw",readHeader.indexOf(temp));
        }else if(temp.contains("Count",Qt::CaseInsensitive)){
            indexes.insert("Count",readHeader.indexOf(temp));
        }else if(temp.contains("Att",Qt::CaseInsensitive)){
            indexes.insert("Att",readHeader.indexOf(temp));
            //capture attenuation units
            QString unit = readHeader.at(readHeader.indexOf(temp));
            bool containsm = unit.contains("m",Qt::CaseInsensitive);
            if(containsm){
                units.insert("Att"," (dB),");
                //TODO add conversion from dBm to dB here
            }else{
                units.insert("Att"," (dB),");
            }
        }else if(temp.contains("Phase",Qt::CaseInsensitive)){
            indexes.insert("Phase",readHeader.indexOf(temp));
        }else if(temp.contains("Cw",Qt::CaseInsensitive)){
            indexes.insert("Cw",readHeader.indexOf(temp));
        }else if(temp.contains("Mop",Qt::CaseInsensitive)){
            indexes.insert("Mop",readHeader.indexOf(temp));
        }else if(temp.contains("Shape",Qt::CaseInsensitive)){
            indexes.insert("Shape",readHeader.indexOf(temp));
        }else if(temp.contains("Rate",Qt::CaseInsensitive)){
            indexes.insert("Rate",readHeader.indexOf(temp));
            //capture rate units
            QString unit = readHeader.at(readHeader.indexOf(temp));
            unit.remove("chirp",Qt::CaseInsensitive).remove(" ").remove("rate",Qt::CaseInsensitive);
            unit.prepend(" ");
            unit.append(",");
            units.insert("Rate",unit);
        }else{
            emit userMessage("undefined header column found in the YATG file : " + temp);
            qDebug() << "undefined header column found in the YATG file : " + temp;
            emit userMessage("acceptable header columns are : ");
            qDebug() << "acceptable header columns are : ";
            emit userMessage("Operation,Pulse Start Time (<units>),Pulse Width (<units>),Frequency (<units>),Phase Mode,Phase (<units>),Relative Power (<units>),Pulse Mode,Markers,Band Adjust,Chirp Shape,Freq/Phase Coding Index,Chirp Rate (<units>),Frequency Band Map");
            qDebug() << "Operation,Pulse Start Time (<units>),Pulse Width (<units>),Frequency (<units>),Phase Mode,Phase (<units>),Relative Power (<units>),Pulse Mode,Markers,Band Adjust,Chirp Shape,Freq/Phase Coding Index,Chirp Rate (<units>),Frequency Band Map";
        }
    }
    if(indexes.size() < 10){ //the number 10 here came from the number of if statements above
        emit userMessage("Not all columns were defined in Yatg file.");
        qDebug() << "Not all columns were defined in Yatg file.";
        //TODO figure out how to turn hashmap to QString
        emit userMessage("Not all columns were defined in Yatg file.");
        qDebug() << indexes;
        return false;
    }else{
        //qDebug() << "indexes loaded in correctly: " << indexes;
    }

    //get the fileName without the directory portion
    QFileInfo fileInfo(workingFile.fileName());
    QString fileNameWithoutDirectory(fileInfo.fileName());
    QString fileName = fileNameWithoutDirectory;

    QString header = "Operation,Pulse Start Time";
    header.append(units.value("Pri"));
    header.append("Pulse Width");
    header.append(units.value("Pri"));
    header.append("Frequency");
    header.append(units.value("Freq"));
    header.append("Phase Mode,Phase (deg),Relative Power");
    header.append(units.value("Att"));
    header.append("Pulse Mode,Markers,Band Adjust,Chirp Shape,Freq/Phase Coding Index,Chirp Rate");
    header.append(units.value("Rate"));
    header.append("Frequency Band Map");

    //calculate size of outputData and add one for the newline character
    QString size = QString::number(header.size() + 1);
    //add the newline character
    header.append("\n");
    QString lengthOfSizeNumber = QString::number(size.size());
    QString command = "MEM:DATA '" + fileName + "',#";
    command.append(lengthOfSizeNumber);
    command.append(size);
    //append the outputData to the output
    command.append(header);
    //send scpi
    //qDebug() << "header is : " << header;
    ftp_manager->send_SCPI(command);

    //Parse through the rest of the entries in the YATG file, ignoring any line that contains "#", and appending to the file for each row
    if(!append_rows_to_uxg_file(indexes,fileName,timeScalingFactor)){
        emit userMessage("appending rows to file failed");
        qDebug() << "appending rows to file failed";
        return false;
    }

    //tell the UXG to import the file as a pdw
    ftp_manager->send_SCPI("MEMory:IMPort:STReam '" + fileName + "', '" + fileName + "'");

    return true;
}

bool YATG::append_rows_to_uxg_file(QHash<QString,int> indexes, QString fileName,double timeScalingFactor){
    //enum pdwIndex{ //not needed but left for debugging
    //    operation, pulseStart, pulseWidth, frequency, phaseMode, phase, relPower, pulseMode, marker, bandAdjust, chirpShape, codingIndex, chirpRate, freqBand
    //};//YATG is : freq,pri,pw,count,att,phase,cw,mop,shape,rate

    QString operationVal = "1";
    double currentTOA = 0;
    QString output;
    QString outputData;
    while(true){
        //read the next line
        QString row = streamer.readLine().remove("\n");
        //check if we should ignore this row in the file
        if(row.contains("#")){
            continue;
        }
        QStringList rowList = row.split(QRegExp(","), Qt::SkipEmptyParts);
        //qDebug() << "Read-in pdw row : " << rowList;

        //check if we are at the end of the file
        if(rowList.size() == 0){
            emit userMessage("finished parsing through pdw's");
            qDebug() << "finished parsing through pdw's";
            break;
        }

        //check that there is a value for each column
        if(rowList.size() != indexes.size()){
            //TODO rowList to qstring
            emit userMessage("Number of values in row do not match the number of columns. rowList size: " + QString::number(rowList.size())
                             + ", indexes size: " + QString::number(indexes.size()) +", row: " + QString(""));
            qDebug() << "Number of values in row do not match the number of columns. rowList size: " << rowList.size() << ", indexes size: " << indexes.size() << ", row: " << rowList;
            return false;
        }

        //loop through the number of pdw's desired for that row
        for(int i = 0; i < rowList.at(indexes.value("Count")).toInt(); i++){

            output = "MEM:DATA:APP '" + fileName + "',#";
            outputData = "";
            outputData.append(operationVal + ",");
            outputData.append(QString::number(currentTOA) + ","); //pulse start time
            bool ok = false;
            //calculate the next toa
            double valueOfPri = rowList.at(indexes.value("Pri")).toDouble(&ok);
            double setupTime = (double)0.00000027 * (timeScalingFactor); //adds in 270ns for the setup time
            currentTOA += valueOfPri + setupTime;
            emit userMessage("currentToa: " + QString::number(currentTOA));
            qDebug() << "currentToa: " << currentTOA;
            outputData.append(rowList.at(indexes.value("Pw")) + ",");
            outputData.append(rowList.at(indexes.value("Freq")) + ",");
            outputData.append("COH,");
            outputData.append(rowList.at(indexes.value("Phase")) + ",");
            outputData.append(rowList.at(indexes.value("Att")) + ",");
            if(rowList.at(indexes.value("Cw")) == "1"){
                outputData.append("CW,");
            }else{
                outputData.append("ON,");
            }
            outputData.append("0x0,"); //marker statically set
            outputData.append("0,"); //Band Adjust (currently statically set to CW switch points)
            outputData.append(rowList.at(indexes.value("Shape")) + ",");
            outputData.append(rowList.at(indexes.value("Mop")) + ",");
            outputData.append(rowList.at(indexes.value("Rate")) + ",");
            outputData.append("A"); //freqBandMap

            //calculate size of outputData and add one for the newline character
            QString size = QString::number(outputData.size() + 1);
            //add the newline character
            outputData.append("\n");
            QString lengthOfSizeNumber = QString::number(size.size());
            output.append(lengthOfSizeNumber);
            output.append(size);
            //append the outputData to the output
            output.append(outputData);
            //send scpi
            //qDebug() << "output: " << output;
            ftp_manager->send_SCPI(output);
            ftp_manager->tcpSocket->waitForBytesWritten();

            //this if statement is placed at the end of the loop so the first iteration will have an operationVal of 1
            if(operationVal == "1"){
                operationVal = "0";
            }
        }
    }

    //append the last blank pdw to the file with operationVal of 2
    output = "MEM:DATA:APP '" + fileName + "',#";
    outputData = "";
    outputData.append("2,"); //operation value of 2 for the end
    outputData.append(QString::number(currentTOA) + ","); //pulse start time
    outputData.append("0,"); //PW
    outputData.append("0,"); //freq
    outputData.append("COH,");//phaseMode
    outputData.append("0,");//phase
    outputData.append("0,");//power
    outputData.append("OFF,");//pulseMode
    outputData.append("0x0,"); //marker statically set
    outputData.append("0,"); //BandAdjust
    outputData.append("Ramp,");//shape
    outputData.append("0,"); //MOP
    outputData.append("0,");//rate
    outputData.append("A"); //freqBandMap

    //calculate size of outputData and add one for the newline character
    QString size = QString::number(outputData.size() + 1);
    //add the newline character
    outputData.append("\n");
    QString lengthOfSizeNumber = QString::number(size.size());
    output.append(lengthOfSizeNumber);
    output.append(size);
    //append the outputData to the output
    output.append(outputData);
    //send scpi
    ftp_manager->send_SCPI(output);
    ftp_manager->tcpSocket->waitForBytesWritten();
    return true;
}

bool YATG::upload_multiple_files_to_uxg(){
    bool successful_upload = true;
    //loop through list of files setting each one's filepath to be the working filepath before calling upload_file_to_uxg
    //assume the directory exists and contains some files and you want all jpg and JPG files
    QDir directory(workingFilePath);
    QStringList files = directory.entryList(QStringList() << "*.csv" << "*.CSV" << "*.txt" << "*.TXT",QDir::Files);
    QString workingFolderPath = QString(workingFilePath);
    foreach(QString filename, files) {
        emit userMessage("sending file: " + filename);
        qDebug() << "sending file: " << filename;
        workingFilePath = QString(workingFolderPath);
        workingFilePath.append("/");
        workingFilePath.append(filename);
        emit userMessage("workingFilePath: " + workingFilePath);
        qDebug() << "workingFilePath: " << workingFilePath;
        successful_upload = upload_file_to_uxg();
        if(!successful_upload)
            break;
    }

    //if, during the loop, any of the calls to upload_file_to_uxg returns false, we break out of the loop and the if statement below the loop returns that result
    if(successful_upload){
        emit userMessage("Finished uploading all files");
        qDebug() << "Finished uploading all files";
        return true;
    }else{
        emit userMessage("Error occured in uploading file: " +workingFilePath +", aborting batch file upload");
        qDebug() << "Error occured in uploading file: " << workingFilePath << ", aborting batch file upload";
        return false;
    }
}
