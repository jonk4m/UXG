#ifndef ENTRY_H
#define ENTRY_H

#include <QMainWindow>
#include <QObject>
#include <QDebug>

class Entry
{
public:
    Entry();
    void parse_entry_for_plain_text_pattern();
    QString hex_to_binary_converter(QString hex);
    QString binary_to_hex_converter(QString binary);

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

};

#endif // ENTRY_H
