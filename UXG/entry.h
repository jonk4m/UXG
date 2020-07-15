#ifndef ENTRY_H
#define ENTRY_H

#include <QMainWindow>
#include <QObject>

class Entry
{
public:
    Entry();

    QString comment = "0"; //will be changed to increment for each entry in table
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
    QString plainTextRepresentation = "";

};

#endif // ENTRY_H
