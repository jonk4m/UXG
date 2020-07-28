#ifndef YATG_H
#define YATG_H

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
#include <QHash>
#include <ftpmanager.h>

class YATG
{
public:

    YATG();
    YATG(FtpManager*);
    bool upload_file_to_uxg();
    bool upload_multiple_files_to_uxg();

    QFile workingFile;
    QString workingFilePath;
    QTextStream streamer; //used for writing and reading from the QFile
    FtpManager *ftp_manager;

    bool uploadingMultipleFiles = false;

private:

};

#endif // YATG_H
