#include "fpcs.h"

/*
 * The Fpcs class has the ability to create it's own csv, edit an existing csv (or the one it created), or delete a csv on the local system and it's corresponding fpcs file on the UXG.
 * The Fpcs class does not create the fpcs file on the UXG using SCPI commands to accomplish "line by line" patterns, but rather creates or edits an entire csv file that is then imported into the UXG which converts it into the fpcs file format. This is done for added flexibility in editing the fpcs
 *
 */
Fpcs::Fpcs(QMainWindow *window)
{
    QMainWindow::connect(this, SIGNAL(userMessage(QString)), window, SLOT(output_to_console(QString)));
    workingEntry = new Entry(window);
    this->window=window;
    //none needed, but method kept for future debugging
};

/*
 * This method either creates a file in that path or opens the existing file to be edited.
 * If the file is created, one will be created and initialized with the Fpcs header.
 * If the file is loaded, the header will be checked and the QTextStreamer's position will be placed at the end of the file.
 * In both scenarios, the QTextStreamer will use the file either created or opened as it's device
 * In both scenarios, the file is left open until either the program is exited, or the user changes files.
 */
bool Fpcs::initialize_workingFile(bool exists){

    //if a different file was already open, this will close it so we can select a new file
    workingFile.close();

    settings.tableNameAndPath.clear();
    settings.tableNameAndPath.append(settings.tablePath).append("/").append(settings.tableName).append(".csv");
    userMessage("Initializing File. file name and path: " + settings.tableNameAndPath);

    if(exists == true){ //loading in an existing Table
        return initialize_existingFile_local();
    }else{
        return initialize_newFile();
    }
}

bool Fpcs::initialize_existingFile_local(){

    workingFile.setFileName(settings.tableNameAndPath); //set this file path + name to be the working file
    bool exists = workingFile.exists(settings.tableNameAndPath);
    if(exists){
        if(!workingFile.open(QFile::ReadWrite | QFile::Text | QFile::ExistingOnly)) //Options: ExistingOnly, NewOnly, Append
        {
            emit userMessage(" Could not open File : " + settings.tableNameAndPath);
            qDebug() << " Could not open File : " + settings.tableNameAndPath;
            return false;
        }
    }else{
        emit userMessage("File Not Found in Local System when initializing file in local system: " + settings.tableNameAndPath);
        qDebug() << "File Not Found in Local System when initializing file in local system: " << settings.tableNameAndPath;
        return false;
    }

    workingEntryList.clear();

    //create the QTextStreamer to operate on this file until further notice
    this->set_streamer();

    //check header
    bool correctHeader = this->check_file_header();
    if(!correctHeader){
       // emit userMessage("Incorrect Header found on Loaded Table File");
       // qDebug() << "Incorrect Header found on Loaded Table File";
    }

    //whether the header is incorrect or not, we need to parse through the file and to assign the data to Entry's, write the correct header using "write_header_to_workingFile()" which also clears the file first
    //then once we are finished editing the entries or adding some, we will rewrite the entire csv file anyway
    import_entries_from_existing_file();

    //checkflag so widgets know a valid file is in play
    this->settings.fileInPlay = true;
    return true;
}

bool Fpcs::initialize_newFile(){
    /* creating a new file
    *  Note that we use two separate strings that make up the filepath. One is the folder filepath, and the other is the name of the file
    *  this is done to simplify editing both separately in the gui.
    */
    bool exists = workingFile.exists(settings.tableNameAndPath); //this->mapped_file_path()
    if(exists){
        emit userMessage("File : " + settings.tableNameAndPath + " already exists, aborting file creation...");
        qDebug() << "File : " + settings.tableNameAndPath + " already exists, aborting file creation...";
        return false;
    }
    workingFile.setFileName(settings.tableNameAndPath); //set this file path for the working file
    if(!workingFile.open(QFile::ReadWrite | QFile::Text)) //Options: ExistingOnly, NewOnly, Append
    {
        emit userMessage(" Could not create File : " + settings.tableNameAndPath);
        qDebug() << " Could not create File : " + settings.tableNameAndPath;
        return false;
    }

    workingEntryList.clear();
    //create the QTextStreamer to operate on this file until further notice
    this->set_streamer();
    //checkflag so widgets know a valid file is in play
    this->settings.fileInPlay = true;
    return true;
}

/*
 * Closes current workingFile
 */
void Fpcs::close_file(){
    if(settings.fileInPlay == true){
        workingFile.close();
        this->settings.fileInPlay = false;
    }
};

/*
 * Sets the Class's QTextStream streamer to the current workingFile
 */
void Fpcs::set_streamer(){
    streamer.setDevice(&workingFile);
};

/*
 * Writes the correct header to the csv file, and writes over the current header if one already exists. workingFile MUST be set before calling.
 * header is "Comment,State,Coding Type,Length,Bits Per Subpulse,Phase State 0 (deg),Phase State 1 (deg),Phase State 2 (deg),Phase State 3 (deg),Phase State 4 (deg),Phase State 5 (deg),Phase State 6 (deg),Phase State 7 (deg),Phase State 8 (deg),Phase State 9 (deg),Phase State 10 (deg),Phase State 11 (deg),Phase State 12 (deg),Phase State 13 (deg),Phase State 14 (deg),Phase State 15 (deg),Frequency State 0 (Hz),Frequency State 1 (Hz),Frequency State 2 (Hz),Frequency State 3 (Hz),Frequency State 4 (Hz),Frequency State 5 (Hz),Frequency State 6 (Hz),Frequency State 7 (Hz),Frequency State 8 (Hz),Frequency State 9 (Hz),Frequency State 10 (Hz),Frequency State 11 (Hz),Frequency State 12 (Hz),Frequency State 13 (Hz),Frequency State 14 (Hz),Frequency State 15 (Hz),Hex Pattern"
 * clear the file, insert the header string at the start, then add an OFF row (required for the UXg according to documentation)
 */
void Fpcs::write_header_to_workingFile(){
    //clear the file
    workingFile.resize(0);
    //insert the header string at the start
    streamer.seek(0); //make sure the TextStream starts at the beginning of the file
    QString header = "Comment,State,Coding Type,Length,Bits Per Subpulse,Phase State 0 (deg),Phase State 1 (deg),Phase State 2 (deg),Phase State 3 (deg),Phase State 4 (deg),Phase State 5 (deg),Phase State 6 (deg),Phase State 7 (deg),Phase State 8 (deg),Phase State 9 (deg),Phase State 10 (deg),Phase State 11 (deg),Phase State 12 (deg),Phase State 13 (deg),Phase State 14 (deg),Phase State 15 (deg),Frequency State 0 (Hz),Frequency State 1 (Hz),Frequency State 2 (Hz),Frequency State 3 (Hz),Frequency State 4 (Hz),Frequency State 5 (Hz),Frequency State 6 (Hz),Frequency State 7 (Hz),Frequency State 8 (Hz),Frequency State 9 (Hz),Frequency State 10 (Hz),Frequency State 11 (Hz),Frequency State 12 (Hz),Frequency State 13 (Hz),Frequency State 14 (Hz),Frequency State 15 (Hz),Hex Pattern\n";
    streamer << header;
    //add an OFF row (required for the UXg according to documentation)
    QString blankRow = ",OFF,PHASE,0,1,0,180,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,\n";
    streamer << blankRow;
    streamer.flush(); //ensure all the data is written to the file before moving on
};

/*
 * This function ensures the header on the imported file is correct for parsing and editing
 */
bool Fpcs::check_file_header(){
    streamer.seek(0); //make sure the TextStream starts at the beginning of the file
    QString header = "Comment,State,Coding Type,Length,Bits Per Subpulse,Phase State 0 (deg),Phase State 1 (deg),Phase State 2 (deg),Phase State 3 (deg),Phase State 4 (deg),Phase State 5 (deg),Phase State 6 (deg),Phase State 7 (deg),Phase State 8 (deg),Phase State 9 (deg),Phase State 10 (deg),Phase State 11 (deg),Phase State 12 (deg),Phase State 13 (deg),Phase State 14 (deg),Phase State 15 (deg),Frequency State 0 (Hz),Frequency State 1 (Hz),Frequency State 2 (Hz),Frequency State 3 (Hz),Frequency State 4 (Hz),Frequency State 5 (Hz),Frequency State 6 (Hz),Frequency State 7 (Hz),Frequency State 8 (Hz),Frequency State 9 (Hz),Frequency State 10 (Hz),Frequency State 11 (Hz),Frequency State 12 (Hz),Frequency State 13 (Hz),Frequency State 14 (Hz),Frequency State 15 (Hz),Hex Pattern";
    QString headerRead = streamer.readLine().remove("\n");
    if(headerRead != header){
        //qDebug() << "Correct : " + header;
        //qDebug() << "Given   : " + headerRead;
        return false;
    }
    return true;
};

/*
 * This method uses the existing working Entry struct to parse through and output the most current data manipulated in the gui pattern edit page into the workingFile through the streamer.
 */
bool Fpcs::add_entry_to_file(){
    if(this->settings.fileInPlay == true){
        //parse through the fpcs_entry and add those values to the table csv file
        streamer << workingEntry->comment + ","; //comment gets incremented then converted to a QString
        streamer << workingEntry->state + ",";
        streamer << workingEntry->codingType + ",";
        workingEntry->length = workingEntry->bitPattern.size();
        streamer << QString::number(workingEntry->length) + ",";
        streamer << QString::number(workingEntry->bitsPerSubpulse) + ",";

        //enhanced for loop going through the array of phase values
        for(int i = 0; i < 16; i++){
            streamer << workingEntry->phases[i] + ",";
        }

        //for loop going through the array of freq values
        for(int i = 0; i < 16; i++){
            unsigned long long int tempMultiplier = 1;
            if(workingEntry->freqUnits.at(i) == QString("G") || workingEntry->freqUnits.at(i) == QString("g")){
                tempMultiplier = 1000000000;
            }else if(workingEntry->freqUnits.at(i) == QString("M") || workingEntry->freqUnits.at(i) == QString("m")){
                tempMultiplier = 1000000;
            }else if(workingEntry->freqUnits.at(i) == QString("K") || workingEntry->freqUnits.at(i) == QString("k")){
                tempMultiplier = 1000;
            }else{
                emit userMessage("Unknown Scaling Factor used for a frequency in a pattern : " + workingEntry->freqUnits.at(i));
                qDebug() << "Unknown Scaling Factor used for a frequency in a pattern : " << workingEntry->freqUnits.at(i);
                return false;
            }

            streamer << QString::number(workingEntry->freqs.at(i).toULongLong() * tempMultiplier) + ",";
        }

        //add the hex pattern
        workingEntry->hexPattern = workingEntry->binary_to_hex_converter(workingEntry->bitPattern);

        streamer << "#h" << workingEntry->hexPattern;

        streamer << "\n"; //end of entry, new line for next entry

        streamer.flush();

    }else{
        emit userMessage("No File is currently selected for the entry to be added to.");
        qDebug() << "No File is currently selected for the entry to be added to.";
        return false;
    }
    return false;
}

void Fpcs::data_dump_onto_file(){
    if(settings.fileInPlay == true){
        write_header_to_workingFile();
        for(Entry *entry : workingEntryList){
            workingEntry = entry;
            add_entry_to_file();
        }
    }
}

/*
 * Note that after extensive testing, it never occurred that the units for frequency of an exported file ever deviated from "(Hz)". Thus, this function assumes this static condition
 */
void Fpcs::import_entries_from_existing_file(){
    //parse through the header to determine mapping of values available to values
    streamer.seek(0);
    QStringList readHeader = streamer.readLine().remove("\n").split(QRegExp(","), Qt::SkipEmptyParts);
    //qDebug() << "header list : " << readHeader;
    QString throwAwayRow = streamer.readLine().remove("\n"); //the first row is always a filler by convention in teh documentation

    while(true){
        //read the next line
        QString row = streamer.readLine().remove("\n");
        //emit userMessage("row: " + row);

        //check if we are at the end of the file
        if(row.size() == 0){
            break;
        }

        QStringList rowList = row.split(QRegExp(","), Qt::SkipEmptyParts);
        //qDebug() << "rowList : " << rowList;

        Entry *tempEntry = new Entry(window);

        tempEntry->comment = rowList.at(0);
        tempEntry->state = rowList.at(1);
        tempEntry->codingType = rowList.at(2);
        tempEntry->length = rowList.at(3).toInt();
        tempEntry->bitsPerSubpulse = rowList.at(4).toInt();
        tempEntry->numOfPhasesOrFreqs = qPow(2,tempEntry->bitsPerSubpulse);
        if(readHeader.indexOf("Hex Pattern",Qt::CaseInsensitive) == -1){
            emit userMessage("Error, hex pattern not found in file. Index is : " +QString::number(readHeader.indexOf("Hex Pattern",Qt::CaseInsensitive)));
            qDebug() << "Error, hex pattern not found in file. Index is : " << readHeader.indexOf("Hex Pattern",Qt::CaseInsensitive);
            return;
        }
        tempEntry->hexPattern = rowList.at(readHeader.indexOf("Hex Pattern",Qt::CaseInsensitive));

        //assign appropriate phases based on indexes
        int tempPhaseIndex;
        for(int i=0; i<16; i++){
            tempPhaseIndex = readHeader.indexOf("Phase State " + QString::number(i) + " (deg)",Qt::CaseInsensitive);
            if (tempPhaseIndex != -1)
                tempEntry->phases.replace(i, rowList.at(tempPhaseIndex));
        }

        //assign appropriate freqs based on indexes
        int tempFreqIndex;
        double freq;
        for(int i=0; i<16; i++){
            tempFreqIndex = readHeader.indexOf("Frequency State " + QString::number(i) + " (Hz)",Qt::CaseInsensitive);
            if (tempFreqIndex != -1){
                freq = rowList.at(tempFreqIndex).toDouble(); //changed
                if(freq > 999999999){
                    //use Giga
                    tempEntry->freqUnits.replace(i, "G");
                    freq = freq / 1000000000;
                }else if(freq > 999999){
                    //use Mega
                    tempEntry->freqUnits.replace(i, "M");
                    freq = freq / 1000000;
                }else{
                    //use kilo
                    tempEntry->freqUnits.replace(i, "K");
                    freq = freq / 1000;
                }
                //emit userMessage("freq " + QString::number(i) + " is: " + QString::number(freq));
                tempEntry->freqs.replace(i, QString::number(freq));
            }
        }
        //convert from hex QString to binary QString
        tempEntry->hexPattern.remove("#h");
        tempEntry->bitPattern = tempEntry->hex_to_binary_converter(tempEntry->hexPattern);

        //create the plaintext pattern
        tempEntry->parse_entry_for_plain_text_pattern();

        workingEntryList << tempEntry;
    }

}

bool Fpcs::add_entry(Entry *newEntry){
    workingEntryList.append(newEntry); //add a new entry to the fpcs list of entries
    return true;
}

















