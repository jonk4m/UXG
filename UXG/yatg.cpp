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
        if(temp.contains("shape",Qt::CaseInsensitive))
            indexes.insert("shape",readHeader.indexOf(temp));
        if(temp.contains("rate",Qt::CaseInsensitive))
            indexes.insert("rate",readHeader.indexOf(temp));
    }
    if(indexes.size() < 10){ //the number 10 here came from the number of if statements above
        qDebug() << "Not all columns were defined in Yatg file.";
        qDebug() << indexes;
        return false;
    }else{
        qDebug() << "indexes loaded in correctly: " << indexes;
    }

    enum pdwIndex{
        operation, pulseStart, pulseWidth, frequency, phaseMode, phase, relPower, pulseMode, marker, bandAdjust, chirpShape, codingIndex, chirpRate, freqBand
    };

    QString header = "Operation,Pulse Start Time (s),Pulse Width (s),Frequency (Hz),Phase Mode,Phase (deg),Relative Power (dB),Pulse Mode,Markers,Band Adjust,Chirp Shape,Freq/Phase Coding Index,Chirp Rate (Hz/s),Frequency Band Map\n";
    //create a QString here to use for the command below to replace the 'joshisgay' part.
    ftp_manager->send_SCPI("MEM:DATA 'joshIsGay.csv',#3209" + header);

    //Parse through the rest of the entries in the YATG file, ignoring any line that contains "#"
    QString operationVal = "1";
    while(true){
        //read the next line
        QString row = streamer.readLine().remove("\n");
        QString output;
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
        if(operationVal == "1"){

        }


    }

}

bool YATG::upload_multiple_files_to_uxg(){
    return true;
}
