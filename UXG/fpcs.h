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
#include <entry.h>


class Fpcs
{
public:

    struct fpcs_settings{
        bool usingCustomTableName = false;
        bool usingCustomFilePath = false;
        bool usingExistingTable = false;
        bool usingExistingTableLocal = true;
        bool usingBinaryDataView = true;
        //this fileInPlay checkflag is true when a file has either been created or opened, AND the header has been created/checked AND the streamer is in the right position using that file
        bool fileInPlay = false;
        int defaultTableNumber = 0;
        QString defaultFilePath = QDir::currentPath() + "/fileFolder/uploads"; //note that the "fileFolder" and "uploads" folders needs to be created next to the executable on deployment
        QString defaultTableName = "Table0"; //TODO this will be incremented each time using: "Table" + QString::number(defaultTableNumber);
        QString customFilePath;
        QString customTableName;
        QString existingTableFilePath; //contains the path and name, unlike the others in this struct
        int preferredFormat = 0;
    };

    QFile workingFile;
    QTextStream streamer; //used for writing and reading from the QFile
    fpcs_settings settings;
    Entry workingEntry;
    QList<Entry> workingEntryList;


    Fpcs();
    bool initialize_workingFile();
    bool initialize_existingFile_local();
    bool initialize_newFile();

    void close_file();
    void set_streamer();
    void write_header_to_workingFile();
    QString mapped_file_path();
    bool check_file_header();
    bool add_entry(Entry &newEntry);
    bool add_entry_to_file();
    void data_dump_onto_file();
    void import_entries_from_existing_file();

};

#endif // FPCS_H

