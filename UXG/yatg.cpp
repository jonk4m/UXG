#include "yatg.h"

YATG::YATG(){
    //constructor
}

YATG::YATG(FtpManager *ftpManager){
    this->ftp_manager = ftpManager;
}

bool YATG::upload_file_to_uxg(){
    //set the WorkingFile to be the file at the WorkingFilePath

    workingFile.close(); //if a different file was already open, this will close it so we can select a new file

    workingFile.setFileName(workingFilePath); //set this file path to be for the working file
    bool exists = workingFile.exists(workingFilePath);
    if(exists){
        if(!workingFile.open(QFile::ReadWrite | QFile::Text | QFile::ExistingOnly)) //Options: ExistingOnly, NewOnly, Append
        {
            qDebug() << " Could not open File : " + workingFilePath;
            return false;
        }
    }else{
        qDebug() << "File Not Found in Local System: " << workingFilePath;
        return false;
    }

    streamer.setDevice(&workingFile); //set the streamer for this file

    //Parse through the header
    streamer.seek(0);
    QStringList readHeader = streamer.readLine().remove("\n").split(QRegExp(","), Qt::SkipEmptyParts);
    qDebug() << "header of YATG : " << readHeader;
    QHash<QString,int> indexes;
    for(QString temp : readHeader){
        if(temp.contains("Freq",Qt::CaseInsensitive))
            indexes.insert("Freq",readHeader.indexOf(temp));
        if(temp.contains("Pri",Qt::CaseInsensitive))
            indexes.insert("Pri",readHeader.indexOf(temp));
        if(temp.contains("Pw",Qt::CaseInsensitive))
            indexes.insert("Pw",readHeader.indexOf(temp));
        if(temp.contains("Count",Qt::CaseInsensitive))
            indexes.insert("Count",readHeader.indexOf(temp));
        if(temp.contains("Att",Qt::CaseInsensitive))
            indexes.insert("Att",readHeader.indexOf(temp));
        if(temp.contains("Phase",Qt::CaseInsensitive))
            indexes.insert("Phase",readHeader.indexOf(temp));
        if(temp.contains("Cw",Qt::CaseInsensitive))
            indexes.insert("Cw",readHeader.indexOf(temp));
        if(temp.contains("Mop",Qt::CaseInsensitive))
            indexes.insert("Mop",readHeader.indexOf(temp));
        if(temp.contains("Shape",Qt::CaseInsensitive))
            indexes.insert("Shape",readHeader.indexOf(temp));
        if(temp.contains("Rate",Qt::CaseInsensitive))
            indexes.insert("Rate",readHeader.indexOf(temp));
    }
    if(indexes.size() < 10){ //the number 10 here came from the number of if statements above
        qDebug() << "Not all columns were defined in Yatg file.";
        qDebug() << indexes;
        return false;
    }else{
        qDebug() << "indexes loaded in correctly: " << indexes;
    }

    QString header = "Operation,Pulse Start Time (s),Pulse Width (s),Frequency (Hz),Phase Mode,Phase (deg),Relative Power (dB),Pulse Mode,Markers,Band Adjust,Chirp Shape,Freq/Phase Coding Index,Chirp Rate (Hz/s),Frequency Band Map\n";
    //create a QString here to use for the command below to replace the 'josh' part.
    ftp_manager->send_SCPI("MEM:DATA 'josh.csv',#3209" + header);

    //Parse through the rest of the entries in the YATG file, ignoring any line that contains "#", and appending to the file for each row
    if(!append_rows_to_uxg_file(indexes)){
        qDebug() << "appending rows to file failed";
        return false;
    }

    //get the fileName without the directory portion
    QFileInfo fileInfo(workingFile.fileName());
    QString fileNameWithoutDirectory(fileInfo.fileName());
    QString fileName = fileNameWithoutDirectory;

    //tell the UXG to import the file as a pdw
    ftp_manager->send_SCPI("MEMory:IMPort:STReam '" + fileName + "', '" + fileName + "'");

}

bool YATG::append_rows_to_uxg_file(QHash<QString,int> indexes){
    //enum pdwIndex{ //not needed but left for debugging
    //    operation, pulseStart, pulseWidth, frequency, phaseMode, phase, relPower, pulseMode, marker, bandAdjust, chirpShape, codingIndex, chirpRate, freqBand
    //};//YATG is : freq,pri,pw,count,att,phase,cw,mop,shape,rate

    //get the fileName without the directory portion
    QFileInfo fileInfo(workingFile.fileName());
    QString fileNameWithoutDirectory(fileInfo.fileName());
    QString fileName = fileNameWithoutDirectory;

    QString operationVal = "1";
    unsigned long long currentTOA = 0;
    QString output;
    QString outputData;
    while(true){
        //read the next line
        QString row = streamer.readLine().remove("\n");
        //check if we are at the end of the file
        if(row.size() == 0){
            //output the last pdw (which is operation of 2 and is basically blank values) then break
            break;
        }
        //check if we should ignore this row in the file
        if(row.contains("#")){
            continue;
        }
        QStringList rowList = row.split(QRegExp(","), Qt::SkipEmptyParts);
        //qDebug() << "rowList : " << rowList;
        //check that there is a value for each column
        if(rowList.size() != indexes.size()){
            qDebug() << "Number of values in row do not match the number of columns. row: " << rowList;
            return false;
        }

        //loop through the number of pdw's desired for that row
        for(int i = 0; i < indexes.value("Count"); i++){
            output = "MEM:DATA:APP '" + fileName + "',#";
            outputData = "";
            outputData.append(operationVal + ",");
            outputData.append(QString::number(currentTOA) + ","); //pulse start time
            //TODO calculate what the next TOA will be (currentTOA + PRI)
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
            outputData.append("0,"); //marker statically set
            outputData.append("CW,"); //TODO Band Adjust?!?!?!?!?!
            outputData.append(rowList.at(indexes.value("Shape")) + ",");
            outputData.append(rowList.at(indexes.value("Mop")) + ",");
            outputData.append(rowList.at(indexes.value("Rate")) + ",");
            outputData.append("A"); //TODO have the freqBandMap adjust dynamically by checking what the center freq is

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
    outputData.append("0,"); //marker statically set
    outputData.append("CW,"); //BandAdjust
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
    return true;
}

bool YATG::upload_multiple_files_to_uxg(){
    return true;
}
