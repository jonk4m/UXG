#include "fpcs.h"

/*
 * The Fpcs class has the ability to create it's own csv, edit an existing csv (or the one it created), or delete a csv on the local system and it's corresponding fpcs file on the UXG.
 * The only difference between editing a csv the class has created or editing a csv that's already exists, is that the fpcs class will check that the header of the csv file that already existed matches the conventions of how the fpcs class would create the csv
 * The Fpcs class does not create the fpcs file on the UXG using SCPI commands to accomplish "line by line" patterns, but rather creates or edits an entire csv file that is then imported into the UXG which converts it into the fpcs file format. This is done for added flexibility in editing the fpcs
 *
 * Possible Edit: Fpcs will keep a resource file that will be used to store the names of all previously made csv's so even if the program is exited and reopened, the delete all files method will work by viewing this text file of names
 *
 */
Fpcs::Fpcs()
{
    //none needed, but method kept for future debugging
};

/*
 * This method either creates a file in that path or opens the existing file to be edited.
 * If the file is created, one will be created and initialized with the Fpcs header.
 * If the file is loaded, the header will be checked and the QTextStreamer's position will be placed at the end of the file.
 * In both scenarios, the QTextStreamer will use the file either created or opened as it's device
 * In both scenarios, the file is left open until either the program is exited, or the user changes files.
 */
bool Fpcs::initialize_workingFile(){

    //if a different file was already open, this will close it so we can select a new file
    workingFile.close();

    if(this->settings.usingExistingTable == true){ //loading in an existing Table
        return initialize_existingFile_local();
    }else{
        return initialize_newFile();
    }
}

bool Fpcs::initialize_existingFile_local(){

    workingFile.setFileName(this->settings.existingTableFilePath); //set this file path to be for the working file
    bool exists = workingFile.exists(this->settings.existingTableFilePath);
    if(exists){
        if(!workingFile.open(QFile::ReadWrite | QFile::Text | QFile::ExistingOnly)) //Options: ExistingOnly, NewOnly, Append
        {
            qDebug() << " Could not open File : " + this->settings.existingTableFilePath;
            return false;
        }
    }else{
        qDebug() << "File Not Found";
        return false;
    }
    //create the QTextStreamer to operate on this file until further notice
    this->set_streamer();
    //check header
    bool correctHeader = this->check_file_header();
    if(!correctHeader){
        qDebug() << "Incorrect Header found on Loaded Table File";
    }
    //TODO put streamer's position at end of the file (do I need to after using it to check the file header?)

    //checkflag so widgets know a valid file is in play
    this->settings.fileInPlay = true;
    return true;
}

bool Fpcs::initialize_newFile(){
    /* creating a new file
    *  Note that we use two separate strings that make up the filepath. One is the folder filepath, and the other is the name of the file
    *  this is done to simplify editing both separately in the gui.
    */
    bool exists = workingFile.exists(this->mapped_file_path());
    if(exists){
        qDebug() << "File : " + this->mapped_file_path() + " already exists, aborting file creation...";
        return false;
    }
    workingFile.setFileName(this->mapped_file_path()); //set this file path for the working file
    if(!workingFile.open(QFile::ReadWrite | QFile::Text)) //Options: ExistingOnly, NewOnly, Append
    {
        qDebug() << " Could not create File : " + this->mapped_file_path();
        return false;
    }
    //create the QTextStreamer to operate on this file until further notice
    this->set_streamer();
    //write the header at the beginning of the file
    this->write_header_to_workingFile();
    //checkflag so widgets know a valid file is in play
    this->settings.fileInPlay = true;
    return true;
}

/*
 * Closes current workingFile
 */
void Fpcs::close_file(){
    workingFile.close();
    this->settings.fileInPlay = false;
};

/*
 *This method is used to construct the full filePath name when juggling the options of custom vs default filePaths and Names when creating a file
 */
QString Fpcs::mapped_file_path(){
    QString totalFilePath;
    if(this->settings.usingCustomFilePath == true){ //custom file path
        totalFilePath.append(this->settings.customFilePath);
        totalFilePath.append("/");
        if(this->settings.usingCustomTableName == true){ //custom file path and custom name
            if(this->settings.customTableName == ""){
                qDebug() << "Custom Table Name is Currently Empty";
                return "";
            }
            totalFilePath.append(this->settings.customTableName);
            totalFilePath.append(".csv");
        }else{ //custom file path and default name
            totalFilePath.append(this->settings.defaultTableName);
            totalFilePath.append(".csv");
        }
    }else{ //default file path
        totalFilePath.append(this->settings.defaultFilePath);
        totalFilePath.append("/");
        if(this->settings.usingCustomTableName == true){ //default file path and custom name
            if(this->settings.customTableName == ""){
                qDebug() << "Custom Table Name is Currently Empty";
                return "";
            }
            totalFilePath.append(this->settings.customTableName);
            totalFilePath.append(".csv");
        }else{ //default file path and default name
            totalFilePath.append(this->settings.defaultTableName);
            totalFilePath.append(".csv");
        }
    }
    return totalFilePath;
}

/*
 * Sets the Class's QTextStream streamer to the current workingFile
 */
void Fpcs::set_streamer(){
    streamer.setDevice(&workingFile);
};

/*
 * Writes the correct header to the csv file, and writes over the current header if one already exists. workingFile MUST be set before calling.
 * header is "Comment,State,Coding Type,Length,Bits Per Subpulse,Phase State 0 (deg),Phase State 1 (deg),Phase State 2 (deg),Phase State 3 (deg),Phase State 4 (deg),Phase State 5 (deg),Phase State 6 (deg),Phase State 7 (deg),Phase State 8 (deg),Phase State 9 (deg),Phase State 10 (deg),Phase State 11 (deg),Phase State 12 (deg),Phase State 13 (deg),Phase State 14 (deg),Phase State 15 (deg),Frequency State 0 (Hz),Frequency State 1 (Hz),Frequency State 2 (Hz),Frequency State 3 (Hz),Frequency State 4 (Hz),Frequency State 5 (Hz),Frequency State 6 (Hz),Frequency State 7 (Hz),Frequency State 8 (Hz),Frequency State 9 (Hz),Frequency State 10 (Hz),Frequency State 11 (Hz),Frequency State 12 (Hz),Frequency State 13 (Hz),Frequency State 14 (Hz),Frequency State 15 (Hz),Hex Pattern"
 */
void Fpcs::write_header_to_workingFile(){
    //insert the string in the file
    streamer.seek(0); //make sure the TextStream starts at the beginning of the file
    QString header = "Comment,State,Coding Type,Length,Bits Per Subpulse,Phase State 0 (deg),Phase State 1 (deg),Phase State 2 (deg),Phase State 3 (deg),Phase State 4 (deg),Phase State 5 (deg),Phase State 6 (deg),Phase State 7 (deg),Phase State 8 (deg),Phase State 9 (deg),Phase State 10 (deg),Phase State 11 (deg),Phase State 12 (deg),Phase State 13 (deg),Phase State 14 (deg),Phase State 15 (deg),Frequency State 0 (Hz),Frequency State 1 (Hz),Frequency State 2 (Hz),Frequency State 3 (Hz),Frequency State 4 (Hz),Frequency State 5 (Hz),Frequency State 6 (Hz),Frequency State 7 (Hz),Frequency State 8 (Hz),Frequency State 9 (Hz),Frequency State 10 (Hz),Frequency State 11 (Hz),Frequency State 12 (Hz),Frequency State 13 (Hz),Frequency State 14 (Hz),Frequency State 15 (Hz),Hex Pattern\n";
    streamer << header;
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
        qDebug() << "Correct : " + header;
        qDebug() << "Given   : " + headerRead;
        return false;
    }
    return true;
};

/*
 * This method uses the existing working Entry struct to parse through and output the most current data manipulated in the gui pattern edit page into the workingFile through the streamer.
 */
bool Fpcs::add_entry(){
    if(this->settings.fileInPlay == true){
       // streamer << "Testing\n";
        //parse through the fpcs_entry and add those values to the table csv file
        workingEntry.comment++;
        streamer << QString::number(workingEntry.comment) + ","; //comment gets incremented then converted to a QString
        streamer << workingEntry.state + ",";
        streamer << workingEntry.codingType + ",";
        workingEntry.length = workingEntry.bitPattern.size();
        streamer << QString::number(workingEntry.length) + ",";
        streamer << QString::number(workingEntry.bitsPerSubpulse) + ",";

        //enhanced for loop going through the array of phase values
        for(int i = 0; i < 16; i++){
            streamer << QString::number(workingEntry.phases[i]) + ",";
        }

        //enhanced for loop going through the array of freq values
        for(int i = 0; i < 16; i++){
            unsigned long long int tempMultiplier = 1;
            if(workingEntry.freqUnits == QString("G") || workingEntry.freqUnits == QString("g")){
                tempMultiplier = 1000000000;
            }else if(workingEntry.freqUnits == QString("M") || workingEntry.freqUnits == QString("m")){
                tempMultiplier = 1000000;
            }else if(workingEntry.freqUnits == QString("K") || workingEntry.freqUnits == QString("k")){
                tempMultiplier = 1000;
            }else{
                qDebug() << "Unknown Scaling Factor used for a frequency in a pattern";
                return false;
            }

            streamer << QString::number(workingEntry.freqs[i] * tempMultiplier) + ",";
        }

        //add the hex pattern TODO the hex conversion is shortening the length. So if I have a length of 20 0's, the hex is just #h0 instead of #h00000
        bool fOK;
        int iValue = workingEntry.bitPattern.toInt(&fOK, 2);  //2 is the base
        workingEntry.hexPattern = QString::number(iValue, 16);  //The new base is 16
        streamer << "#h" + workingEntry.hexPattern;

        streamer << "\n"; //end of entry, new line for next entry

        streamer.flush();

    }else{
        qDebug() << "No File is currently selected for the entry to be added to.";
        return false;
    }
    return false;
}

