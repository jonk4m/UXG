#include "entry.h"

Entry::Entry(QMainWindow *window){
    QMainWindow::connect(this, SIGNAL(userMessage(QString)), window, SLOT(output_to_console(QString)));
}

/*
 * This takes the entire binaryPattern and parses through it to rewrite the plain text pattern of this entry
 * Assumes the binaryPattern has no padded zeros on the end
 */
void Entry::parse_entry_for_plain_text_pattern(){
    //First update the user-end text display to show the plain text pattern they are creating
    this->plainTextRepresentation.clear();

    QString plainText;

    for(int i = 0; i < this->bitPattern.size(); i = i + this->bitsPerSubpulse){
        plainText = "[";
        QString currentPulse = this->bitPattern.mid(i,this->bitsPerSubpulse);
        if(this->codingType == "PHASE"){
            bool ok;
            plainText.append(this->phases.at(currentPulse.toInt(&ok,2)) + "°");
            if(!ok){
                emit userMessage("error occured converting binary QString to int");
                qDebug() << "error occured converting binary QString to int, QString offender: " << currentPulse.toInt(&ok,2);
            }
        }else if(this->codingType == "FREQUENCY"){
            bool ok;
            plainText.append(this->freqs.at(currentPulse.toInt(&ok,2)));
            plainText.append(this->freqUnits.at(currentPulse.toInt(&ok,2)) + "hz");
            if(!ok){
                emit userMessage("error occured converting binary QString to int");
                qDebug() << "error occured converting binary QString to int" << currentPulse.toInt(&ok,2);
            }
        }else if(this->codingType == "BOTH"){
            bool ok;
            bool ok2;
            plainText.append(this->phases.at(currentPulse.toInt(&ok,2)) + "°");
            plainText.append(",");
            plainText.append(this->freqs.at(currentPulse.toInt(&ok2,2)));
            plainText.append(this->freqUnits.at(currentPulse.toInt(&ok,2)) + "hz");
            if(!ok || !ok2){
                emit userMessage("error occured converting binary QString to int");
                qDebug() << "error occured converting binary QString to int";
            }
        }else{
            emit userMessage("Unrecognizable Pattern Coding Type Entry : " + this->state);
            qDebug() << "Unrecognizable Pattern Coding Type Entry : " << this->state;
            return;
        }
        plainText.append("], ");
        this->plainTextRepresentation.append(plainText);
    }
}

/*
 * pad the bitpattern with trailing zeros if it's size is not divisible by 4, append zeros until it is. This must be done because the UXG begins reading bit pulses from the
 * most significant bit working back.
 *
 * this function preserves the binary strings leading zeros and appends zeros to made the number of bits divisible by four so it can be converted to hex
 */
QString Entry::binary_to_hex_converter(QString binary){
    QString hexString;

    while(binary.size() % 4 != 0){
        binary.append("0");
    }

    //check if it's all zeros
    if(!binary.contains("1")){
        int numberOfZerosNeeded = binary.size() / 4;
        for(int i = 0; i < numberOfZerosNeeded; i++){
            hexString.append("0");
        }
        return hexString;
    }

    //loop through the binary string taking every 4 chars to be converted into a hex representation and appended
    for(int i = 0; i < binary.size(); i = i + 4){
        QString segment = binary.mid(i,4); //grab 4 bits
        if(segment == "0000"){
            hexString.append("0");
        }else if(segment == "0001"){
            hexString.append("1");
        }else if(segment == "0010"){
            hexString.append("2");
        }else if(segment == "0011"){
            hexString.append("3");
        }else if(segment == "0100"){
            hexString.append("4");
        }else if(segment == "0101"){
            hexString.append("5");
        }else if(segment == "0110"){
            hexString.append("6");
        }else if(segment == "0111"){
            hexString.append("7");
        }else if(segment == "1000"){
            hexString.append("8");
        }else if(segment == "1001"){
            hexString.append("9");
        }else if(segment == "1010"){
            hexString.append("a");
        }else if(segment == "1011"){
            hexString.append("b");
        }else if(segment == "1100"){
            hexString.append("c");
        }else if(segment == "1101"){
            hexString.append("d");
        }else if(segment == "1110"){
            hexString.append("e");
        }else if(segment == "1111"){
            hexString.append("f");
        }else{
            emit userMessage("error binary to hex conversion");
            qDebug() << "error binary to hex conversion";
        }
    }
    return hexString;
}

/*
 * Notably, this method uses the Entry's bitlength to determine if trailing zeros should be left out
 * create the bitPattern -  the bit length specifies the number of significant data bits sampled, starting with the most significant bit of the pattern
 * ex. if the pattern is 3 bits per subpulse, and you enter two phases 110 and 111, your final pattern will be 11011100 since it is in hex. Notice that the padded zeros added
 * are trailing not leading (00110111 is leading). This is because the UXG parses through the bit pattern in Transmission order. However, with the bit length is set to 6, those
 * trailing two zeros will be ignored.
 *
 * This function assumes the hex string has no "0x" or "#h" leading specifiers for hex in the string and that the hex string has been properly padded on the end with zeros
 *
 * This returns a binary string without padded zeros
 */
QString Entry::hex_to_binary_converter(QString hex){
    QString binaryString;

    //check if it's all zeros
    if(hex == "0"){
         for(int i = 0; i < this->length; i++){
             binaryString.append("0");
         }
         return binaryString;
     }

    //loop through the hex string taking every char to be converted into a binary representation and appended
    for(int i = 0; i < hex.size(); i++){
        QString segment = hex.mid(i,1);
        if(segment == "0"){
            binaryString.append("0000");
        }else if(segment == "1"){
            binaryString.append("0001");
        }else if(segment == "2"){
            binaryString.append("0010");
        }else if(segment == "3"){
            binaryString.append("0011");
        }else if(segment == "4"){
            binaryString.append("0100");
        }else if(segment == "5"){
            binaryString.append("0101");
        }else if(segment == "6"){
            binaryString.append("0110");
        }else if(segment == "7"){
            binaryString.append("0111");
        }else if(segment == "8"){
            binaryString.append("1000");
        }else if(segment == "9"){
            binaryString.append("1001");
        }else if(segment == "a"){
            binaryString.append("1010");
        }else if(segment == "b"){
            binaryString.append("1011");
        }else if(segment == "c"){
            binaryString.append("1100");
        }else if(segment == "d"){
            binaryString.append("1101");
        }else if(segment == "e"){
            binaryString.append("1110");
        }else if(segment == "f"){
            binaryString.append("1111");
        }else{
            emit userMessage("error binary to hex conversion");
            qDebug() << "error binary to hex conversion";
        }
    }

    //get rid of extraneous trailing zeros
    int numberOfZerosToRemove = binaryString.size() - this->length;
    binaryString.chop(numberOfZerosToRemove);

    return binaryString;
}

void Entry::clear_pattern(){
    bitPattern.clear();
    parse_entry_for_plain_text_pattern();
}
