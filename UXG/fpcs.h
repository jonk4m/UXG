#ifndef FPCS_H
#define FPCS_H

#include <QMainWindow>
#include <QObject>
#include <QWidget>
#include <QTextEdit>
#include <QTextStream> //allows us to read and write to the file
#include <QFile> //allows us to open, create, or close a file
#include <QtDebug> //allows us to output debug messages
#include <QStandardPaths> //allows us to quickly find directories in the computer
#include <QDir>
#include <QProcess>
#include <QThread>


class Fpcs
{
public:

    struct fpcs_entry{
        int comment = 0; //will be changed to increment for each entry in table
        QString state = "ON"; //default.
        QString codingType = "BOTH"; //FREQUENCY, PHASE, or BOTH
        int length = 0; //the length here is NOT the number of entries in the pattern but rather the count of the number of bits in the pattern
        int bitsPerSubpulse = 4; //default
        QString phaseStates = "0000000000000000"; //starting out the pattern is all 16 zeros. (4 bits is max option)
        QString freqStates = "0000000000000000"; //starting out the pattern is all 16 zeros. (4 bits is max option)
        QString hexPattern = ""; //might need to make a function for this conversion
        int phases[16] = {0, 180, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //16 is max number of entries
        unsigned long long int freqs[16] = {10, 20, 30, 40, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //16 is max number of entries. we must use unsigned long long int since our frequencies can get into the gigaHertz
        QString freqUnits[16] = {"M","k","M","M","M","M","M","M","M","M","M","M","M","M","M","M"};
        QString bitPattern = ""; //the length will be the .size() of this field.
    };

    struct fpcs_settings{
        bool usingCustomTableName = false;
        bool usingCustomFilePath = false;
        bool usingExistingTable = false;
        //this fileInPlay checkflag is true when a file has either been created or opened, AND the header has been created/checked AND the streamer is in the right position using that file
        bool fileInPlay = false;
        int defaultTableNumber = 0;
        QString defaultFilePath = QDir::currentPath() + "/fileFolder/uploads"; //note that the "fileFolder" and "uploads" folders needs to be created next to the executable on deployment
        QString defaultTableName = "Table0"; //TODO this will be incremented each time using: "Table" + QString::number(defaultTableNumber);
        QString customFilePath;
        QString customTableName;
        QString existingTableFilePath; //contains the path and name, unlike the others in this struct
    };

    QFile workingFile;
    QTextStream streamer; //used for writing and reading from the QFile
    fpcs_settings settings;
    fpcs_entry workingEntry;


    Fpcs();
    bool initialize_workingFile();
    void close_file();
    void set_streamer();
    void write_header_to_workingFile();
    bool delete_file_on_uxg();
    bool delete_file_on_local();
    QString mapped_file_path();
    bool delete_all_previously_created_files();
    bool check_file_header();
    bool import_fpcs_onto_uxg(QProcess *ftpProcess);
    bool add_entry();
};

#endif // FPCS_H

