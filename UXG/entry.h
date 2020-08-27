#ifndef ENTRY_H
#define ENTRY_H

/*
 *
 * email IT a list of everything you currently have just to remind them and say Greg Grimes is cool with it. Mention you aren't going to keep the laptops. send it over h4f internal too to ITnotifications
get rid of emit messages on a lot of stuff
get rid of todos in code
start on the manual
include in the example file a list of comments that describe limits and that a # is for a comment line

talk to more people about senior project
ask Spencer if turning into a target generator could be a senior project

show mike how if you are looking at a file, then try to upload a file with the same name, it crashes the uxg

chirp rate absolutely has to be in Hz/us, KHz/us, Mhz/us, or THz/us
+/- 95.9765625 THz/us
resolution is 21.827872842550278 Hz/us

1 Hz/us = 1e6 Hz/s

relative power:
Range: -161.6 dB to 31.07 dB
Resolution: 0.0085 dB


idea for Mike:
markers
triggers
dynamic streaming
faster rf switching by checking what band it's in

*/

#include <QMainWindow>
#include <QObject>
#include <QDebug>

class Entry: public QObject
{
Q_OBJECT
public:

    Entry(QMainWindow *window);
    //Entry(const Entry &obj);
    void parse_entry_for_plain_text_pattern();
    QString hex_to_binary_converter(QString hex);
    QString binary_to_hex_converter(QString binary);
    void clear_pattern();

    QString comment = "0"; //will be changed to increment for each entry in table
    QString state = "ON"; //default.
    QString codingType = "BOTH"; //FREQUENCY, PHASE, or BOTH
    int length = 0; //the length here is NOT the number of entries in the pattern but rather the count of the number of bits in the pattern
    int bitsPerSubpulse = 4; //default
    QString hexPattern = ""; //might need to make a function for this conversion
    QList<QString> phases = {"0", "180", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0"}; //16 is max number of entries
    QList<QString> freqs = {"10", "20", "30", "40", "50", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0"}; //16 is max number of entries. we must use unsigned long long int since our frequencies can get into the gigaHertz
    QList<QString> freqUnits = {"M","k","M","M","M","M","M","M","M","M","M","M","M","M","M","M"};
    QString bitPattern = ""; //the length will be the .size() of this field.
    QString plainTextRepresentation = "";
    int numOfPhasesOrFreqs = 16; //mainly used for consistency with the GUI
signals:
     void userMessage(QString message);
};

#endif // ENTRY_H
