#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDebug"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    MainWindow::fpcs_setup();
    m_manager = new QNetworkAccessManager();
    if(!connect(m_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(fn(QNetworkReply*))))
        qDebug() << "issue with slot 0";
}

MainWindow::~MainWindow()
{
    window_fpcs.close_file(); //close the file (which also flushes the buffer) before exiting
    delete ui;
}

void MainWindow::fpcs_setup(){
    //configured on setup
    ui->table_directory_line_edit->setText(window_fpcs.settings.defaultFilePath); //make the directory text line start out with default file path
    ui->default_name_line_edit->setText(window_fpcs.settings.defaultTableName);
}

void MainWindow::on_specify_table_name_radio_button_clicked()
{
    window_fpcs.settings.usingCustomTableName = true;

    //set the progress bar to 0%
    ui->create_progress_bar->reset();
    ui->loaded_table_progress_bar->reset();
}

void MainWindow::on_specified_table_name_line_edit_textChanged(const QString &arg1)
{
    if(window_fpcs.settings.usingCustomTableName == true){
        //if they checked the radio button for using a custom table name, let's go ahead and change our Table name to whatever they typed in
        window_fpcs.settings.customTableName = arg1;
    }
}

/*
 * When the "use default name" button is pressed, our settings for which name to use is updated, and the default name is shown as well as the default
 * filePath
 */
void MainWindow::on_use_default_name_radio_button_clicked()
{
    window_fpcs.settings.usingCustomTableName = false; //when this is false, we use the defaultTableName and defaultFilePath

    ui->default_name_line_edit->setText(window_fpcs.settings.defaultTableName);

    //set the progress bar to 0%
    ui->create_progress_bar->reset();
    ui->loaded_table_progress_bar->reset();
}

void MainWindow::on_change_table_directory_push_button_clicked()
{
    //prompt the user to select a folder
    QString folderName = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    window_fpcs.settings.customFilePath = folderName;

    window_fpcs.settings.usingCustomFilePath = true;

    ui->table_directory_line_edit->setText(folderName);

    //set the progress bar to 0%
    ui->create_progress_bar->reset();
    ui->loaded_table_progress_bar->reset();

}

void MainWindow::on_use_default_file_directory_check_box_stateChanged(int arg1)
{
    //0 is unckecked
    //2 is checked
    if(arg1 == 0){ //unchecked
        window_fpcs.settings.usingCustomFilePath = true;

    }else{ //checked
        window_fpcs.settings.usingCustomFilePath = false; //we are using the default file path
        ui->table_directory_line_edit->setText(window_fpcs.settings.defaultFilePath); //show the default file path on the editLine
    }

    //set the progress bar to 0%
    ui->create_progress_bar->reset();
    ui->loaded_table_progress_bar->reset();
}

//Dialog Buttons for Creating for Loading Table ------------------------------
void MainWindow::on_create_new_table_button_box_accepted()
{
    window_fpcs.settings.usingExistingTable = false;
    bool fileInitialized = window_fpcs.initialize_workingFile();
    if(fileInitialized){
        //make the progress bar move
        ui->loaded_table_progress_bar->reset();
        for(int j=0; j<=100; j++){
            ui->create_progress_bar->setValue(j);
            QThread::msleep(10);
        }
        ui->current_table_line_edit->setText(window_fpcs.workingFile.fileName());
    }
}

void MainWindow::on_select_existing_table_button_box_accepted()
{
    window_fpcs.settings.usingExistingTable = true;
    bool fileInitialized = window_fpcs.initialize_workingFile();
    if(fileInitialized){
        //make the progress bar move
        ui->create_progress_bar->reset();
        for(int j=0; j<=100; j++){
            ui->loaded_table_progress_bar->setValue(j);
            QThread::msleep(10);
        }
        ui->current_table_line_edit->setText(window_fpcs.workingFile.fileName());
    }
}

void MainWindow::on_create_new_table_button_box_helpRequested()
{
    //TODO open a help pop-up
}

void MainWindow::on_select_existing_table_button_box_helpRequested()
{
    //TODO open a help pop-up
}
//------------------------------------------------------------------------------


//allow the user to select a preexisting file (the header is checked when the "Open" button calls on Fpcs::initialize_workingFile() to check it)
void MainWindow::on_select_file_push_button_clicked()
{
    //prompt the user to select a folder
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"),
                QString(), tr("CSV Files (*.csv);;Text Files (*.txt)"));

    window_fpcs.settings.existingTableFilePath = filePath; //set the filePath

    ui->select_file_line_edit->setText(window_fpcs.settings.existingTableFilePath); //update the EditLine Box so the user can see the filePath they just chose

    //set the progress bar to 0%
    ui->create_progress_bar->reset();
    ui->loaded_table_progress_bar->reset();
}

void MainWindow::on_update_current_table_with_pattern_push_button_clicked()
{
    window_fpcs.add_entry();
    ui->table_or_pattern_toolbox->setCurrentIndex(0); //this then opens the part of the toolbox that is used for visualizing the table
}

/*
 * Note that we intentionally do not reset the entry fields here when creating another entry. The intention is that if the user is making several patterns quickly with only minor changes
 * between each pattern, they will not have to worry about changing every field each time.
 */
void MainWindow::on_add_another_pattern_to_this_table_push_button_clicked()
{
    if(window_fpcs.settings.fileInPlay){
        window_fpcs.streamer.readAll(); // this will cause the position to be set to the end of the file.
        ui->table_or_pattern_toolbox->setCurrentIndex(1); //this then opens the part of the toolbox that is used for editing the pattern
        update_pattern_table();
    }else{
        qDebug() << "No Table Currently Selected to Add a Pattern to.";
        return;
    }
}

/*
 * TODO, this would only be used once, when the very first time the "add a pattern to this table" button is used. My work around for this currently is to have the default values in the struct
 * and the default widget values in the pattern table match to start with.
 *
 * Edit: Actually it will be used more than once since everytime we select an existing row on the table to edit, we need the table to reflect those values as well as the working_Entry values
 *
 */
void MainWindow::update_pattern_table(){
    for(int i = 0; i < 16; i++){
       // ui->phase_freq_pattern_entry_table.seti
    }
}


//TODO when an existing pattern is selected by checking the checkbox in it's row of the visualizer, a function is called that updates the fields of window_fpcs's workingEntry. the streamer's
//position is then updated to be the start of that row on the file. The widgets in the "Edit Pattern" groupbox simply reflect the field values or change them if the widget is edited. This way
//a row is selected, the streamer position is moved, workingEntry has its fields updated with the values of that row read in by the streamer, the streamer's position is AGAIN to the beginning
//of that row, the widgets on the edit pattern page are updated to reflect workingEntry's fields, the widgets are edited by the user which changes workingEntry's fields, and finally when the
//pushbutton to update the table is clicked the streamer writes the values of the workingEntry just as it would if it were a new entry being added, the only difference being that the position
//of streamer is at the beginning of that row rather than the end of the table.

/*
 * This radio button shows the phase pattern column and hides the freq pattern column
 */
void MainWindow::on_phase_pattern_type_radio_button_clicked()
{
    ui->phase_freq_pattern_entry_table->showColumn(0);
    ui->phase_freq_pattern_entry_table->showColumn(1);
    ui->phase_freq_pattern_entry_table->showColumn(4);
    //hide the freq columns
    ui->phase_freq_pattern_entry_table->hideColumn(2);
    ui->phase_freq_pattern_entry_table->hideColumn(3);

    window_fpcs.workingEntry.codingType = "PHASE";
}

/*
 * This radio button shows the freq pattern column and hides the phase pattern column
 */
void MainWindow::on_freq_pattern_type_radio_button_clicked()
{
    ui->phase_freq_pattern_entry_table->showColumn(2);
    ui->phase_freq_pattern_entry_table->showColumn(3);
    ui->phase_freq_pattern_entry_table->showColumn(4);
    //hide the phase columns
    ui->phase_freq_pattern_entry_table->hideColumn(0);
    ui->phase_freq_pattern_entry_table->hideColumn(1);

    window_fpcs.workingEntry.codingType = "FREQUENCY";
}

/*
 * This radio button shows both the phase pattern column and the freq pattern column
 */
void MainWindow::on_both_pattern_type_radio_button_clicked()
{
    ui->phase_freq_pattern_entry_table->showColumn(0);
    ui->phase_freq_pattern_entry_table->showColumn(1);
    ui->phase_freq_pattern_entry_table->showColumn(2);
    ui->phase_freq_pattern_entry_table->showColumn(3);
    ui->phase_freq_pattern_entry_table->showColumn(4);

    window_fpcs.workingEntry.codingType = "BOTH";
}

void MainWindow::on_how_many_different_phase_or_freq_spin_box_valueChanged(int arg1)
{
    if(arg1 == 2){
        //bits per subpulse => 1
        window_fpcs.workingEntry.bitsPerSubpulse = 1;
    }else if(arg1 <= 4){
        //bits per subpulse => 2
        window_fpcs.workingEntry.bitsPerSubpulse = 2;
    }else if(arg1 <= 8){
        //bits per subpulse => 3
        window_fpcs.workingEntry.bitsPerSubpulse = 3;
    }else{
        //bits per subpulse => 4
        window_fpcs.workingEntry.bitsPerSubpulse = 4;
    }

    for(int i = 0; i < arg1; i++){
        ui->phase_freq_pattern_entry_table->showRow(i);
    }

    for(int i = arg1; i < 16; i++){
        ui->phase_freq_pattern_entry_table->hideRow(i);
    }

    window_fpcs.workingEntry.length = arg1;
}

void MainWindow::add_button_in_pattern_table_setup(int row)
{
        //First update the user-end text display to show the plain text pattern they are creating
        if(window_fpcs.workingEntry.codingType == "PHASE"){
            //add the phase value to the pattern visualizer
            ui->pattern_nonBinary_values_shown_text_editor->insertPlainText("[");
            ui->pattern_nonBinary_values_shown_text_editor->insertPlainText(ui->phase_freq_pattern_entry_table->item(row, 0)->text());
            ui->pattern_nonBinary_values_shown_text_editor->insertPlainText("], ");

        }else if(window_fpcs.workingEntry.codingType == "FREQUENCY"){
            //add the freq value to the pattern visualizer
            ui->pattern_nonBinary_values_shown_text_editor->insertPlainText("[");
            ui->pattern_nonBinary_values_shown_text_editor->insertPlainText(ui->phase_freq_pattern_entry_table->item(row, 2)->text());
            ui->pattern_nonBinary_values_shown_text_editor->insertPlainText("], ");

        }else if(window_fpcs.workingEntry.codingType == "BOTH"){
            //add both values to the pattern visualizer
            ui->pattern_nonBinary_values_shown_text_editor->insertPlainText("[");
            ui->pattern_nonBinary_values_shown_text_editor->insertPlainText(ui->phase_freq_pattern_entry_table->item(row, 0)->text());
            ui->pattern_nonBinary_values_shown_text_editor->insertPlainText(",");
            ui->pattern_nonBinary_values_shown_text_editor->insertPlainText(ui->phase_freq_pattern_entry_table->item(row, 2)->text());
            ui->pattern_nonBinary_values_shown_text_editor->insertPlainText("], ");
        }else{
            qDebug() << "Unrecognizable Pattern Coding Type Entry : " + window_fpcs.workingEntry.state;
            return;
        }

        //Then update the binary pattern QString and display this as well in the binary pattern text window
        switch(window_fpcs.workingEntry.bitsPerSubpulse){
        case 1:
            if(row <= 1){
                //This line creates a string from the row int converted into binary (hence the 2), and add zero padding if the number of bits does not reach 1 bits (which it always will for 1 bit though)
                QString rowNumberStringInBinary = QString("%1").arg(row, 1, 2, QChar('0')); //convert the row int to a binary QString
                window_fpcs.workingEntry.bitPattern.append(rowNumberStringInBinary); //append this new chunk of bits to the bit pattern
                ui->pattern_binary_pattern_shown_text_editor->setText(window_fpcs.workingEntry.bitPattern); //update the ui with the bit pattern

            }else{
                qDebug() << "bits per subpulse mismatch with row number being added";
            }
            break;
        case 2:
            if(row <= 3){
                //This line creates a string from the row int converted into binary (hence the 2), and add zero padding if the number of bits does not reach 2 bits.
                QString rowNumberStringInBinary = QString("%1").arg(row, 2, 2, QChar('0')); //convert the row int to a binary QString
                window_fpcs.workingEntry.bitPattern.append(rowNumberStringInBinary); //append this new chunk of bits to the bit pattern
                ui->pattern_binary_pattern_shown_text_editor->setText(window_fpcs.workingEntry.bitPattern); //update the ui with the bit pattern
            }else{
                qDebug() << "bits per subpulse mismatch with row number being added";
            }
            break;
        case 3:
            if(row <= 7){
                //This line creates a string from the row int converted into binary (hence the 2), and add zero padding if the number of bits does not reach 3 bits.
                QString rowNumberStringInBinary = QString("%1").arg(row, 3, 2, QChar('0')); //convert the row int to a binary QString
                window_fpcs.workingEntry.bitPattern.append(rowNumberStringInBinary); //append this new chunk of bits to the bit pattern
                ui->pattern_binary_pattern_shown_text_editor->setText(window_fpcs.workingEntry.bitPattern); //update the ui with the bit pattern
            }else{
                qDebug() << "bits per subpulse mismatch with row number being added";
            }
            break;
        case 4:
            if(row <= 15){
                //This line creates a string from the row int converted into binary (hence the 2), and add zero padding if the number of bits does not reach 4 bits.
                QString rowNumberStringInBinary = QString("%1").arg(row, 4, 2, QChar('0')); //convert the row int to a binary QString
                window_fpcs.workingEntry.bitPattern.append(rowNumberStringInBinary); //append this new chunk of bits to the bit pattern
                ui->pattern_binary_pattern_shown_text_editor->setText(window_fpcs.workingEntry.bitPattern); //update the ui with the bit pattern
            }else{
                qDebug() << "bits per subpulse mismatch with row number being added";
            }
            break;
        default:
            qDebug() << "Bits per subpulse value is invalid";
            break;
        }
}

void MainWindow::on_phase_freq_pattern_entry_table_cellChanged(int row, int column)
{
    switch(column){
    case 0 :
        //this gets the TableWidgetItem that was changed, gets it's QString text, converts it to an Int, then sets that equal to the phase
        window_fpcs.workingEntry.phases[row] = ui->phase_freq_pattern_entry_table->item(row, column)->text().toInt();
        break;
    case 2:
        window_fpcs.workingEntry.freqs[row] = ui->phase_freq_pattern_entry_table->item(row, column)->text().toInt();
        break;
    case 3:
        window_fpcs.workingEntry.freqUnits[row] = ui->phase_freq_pattern_entry_table->item(row, column)->text();
        break;
    case 4:
        //refuse to let the user change it from "ADD"
        ui->phase_freq_pattern_entry_table->item(row,column)->setText("ADD");
        break;
    default:
        qDebug() << "Column in table chosen outside scope of the table";
        return;
    }
}

void MainWindow::on_phase_freq_pattern_entry_table_cellClicked(int row, int column)
{
    if(column == 4){
        add_button_in_pattern_table_setup(row);
    }
}

void MainWindow::on_remove_last_entry_pushbutton_clicked()
{
    //remove last part of the textPattern on the text Editor
    QString currentText = ui->pattern_nonBinary_values_shown_text_editor->toPlainText();
    int startingIndex = currentText.lastIndexOf('[');
    currentText = currentText.remove(startingIndex, 20);
    ui->pattern_nonBinary_values_shown_text_editor->clear();
    ui->pattern_nonBinary_values_shown_text_editor->insertPlainText(currentText);

    //remove that entry from the binaryPattern
    switch(window_fpcs.workingEntry.bitsPerSubpulse){
    case 1:
        window_fpcs.workingEntry.bitPattern.remove(window_fpcs.workingEntry.bitPattern.size() - 1, 1); //remove the last character in the QString
        break;
    case 2:
        window_fpcs.workingEntry.bitPattern.remove(window_fpcs.workingEntry.bitPattern.size() - 2, 2); //remove the last 2 characters in the QString
        break;
    case 3:
        window_fpcs.workingEntry.bitPattern.remove(window_fpcs.workingEntry.bitPattern.size() - 3, 3); //remove the last 3 characters in the QString
        break;
    case 4:
        window_fpcs.workingEntry.bitPattern.remove(window_fpcs.workingEntry.bitPattern.size() - 4, 4); //remove the last four characters in the QString
        break;
    }

    //remove last part of the binaryPattern on the binary text editor
    ui->pattern_binary_pattern_shown_text_editor->setText(window_fpcs.workingEntry.bitPattern);

}

void MainWindow::on_upload_table_to_uxg_pushbutton_clicked()
{
   //if(window_fpcs.settings.fileInPlay){ UNCOMMENT ME
        QUrl *url = new QUrl("ftp://");
        url->setHost("K-N5193A-90114");//169.254.24.85 K-N5193A-90114
        url->setPort(21);
        url->setPath("/BIN/PQEXPORT.CSV");
        url->setUserName("user");
        url->setPassword("keysight");
        qDebug() << "URL is : " + url->toString();
        //if(!connect(m_manager, &QNetworkAccessManager::finished, this, &MainWindow::uploadComplete,Qt::DirectConnection))
        //     qDebug() << "1.1";
        if(!connect(m_manager, &QNetworkAccessManager::authenticationRequired, this, &MainWindow::auth, Qt::DirectConnection))
             qDebug() << "1.2";
        if(!connect(m_manager, &QNetworkAccessManager::sslErrors, this, &MainWindow::auth, Qt::DirectConnection))
             qDebug() << "1.2";
        if(!connect(m_manager, &QNetworkAccessManager::proxyAuthenticationRequired, this, &MainWindow::auth, Qt::DirectConnection))
             qDebug() << "1.2";
        if(!connect(m_manager, &QNetworkAccessManager::encrypted, this, &MainWindow::auth, Qt::DirectConnection))
             qDebug() << "1.2";
        if(!connect(m_manager, &QNetworkAccessManager::preSharedKeyAuthenticationRequired, this, &MainWindow::auth, Qt::DirectConnection))
             qDebug() << "1.2";

        QNetworkReply *reply = m_manager->get(QNetworkRequest(*url));

        if(!connect(reply,&QNetworkReply::readyRead, this, &MainWindow::rr))
            qDebug() << "1";
        if(!connect(reply,&QNetworkReply::errorOccurred, this, &MainWindow::po))
                 qDebug() << "3";
        if(!connect(reply,&QNetworkReply::preSharedKeyAuthenticationRequired, this, &MainWindow::er))
                 qDebug() << "4";
        if(!connect(reply,&QNetworkReply::redirected, this, &MainWindow::er))
                 qDebug() << "5";

    /*}else{
        qDebug() << "No Table Currently Selected for uploading to the UXG.";
        return;
    }*/

}

void MainWindow::fn(QNetworkReply* reply){
    qDebug() << "finished reply";
    QString read = reply->readAll();
    qDebug() << read;
    qDebug() << "entering second process";

    if(looper == 1)
        return;

    QUrl *url = new QUrl("ftp://");
    url->setHost("K-N5193A-90114");//169.254.24.85 K-N5193A-90114
    url->setPort(21);
    url->setPath("/USER/BIN/PQEXPORT.CSV");
    url->setUserName("user");
    url->setPassword("keysight");
    qDebug() << "URL2 is : " + url->toString();
    QNetworkRequest *request = new QNetworkRequest();

    QNetworkReply *reply2 = m_manager->get(*request);

    if(!connect(reply2,&QNetworkReply::readyRead, this, &MainWindow::rr))
        qDebug() << "1";
    if(!connect(reply2,&QNetworkReply::errorOccurred, this, &MainWindow::po))
             qDebug() << "3";
    if(!connect(reply2,&QNetworkReply::preSharedKeyAuthenticationRequired, this, &MainWindow::er))
             qDebug() << "4";
    if(!connect(reply2,&QNetworkReply::redirected, this, &MainWindow::er))
             qDebug() << "5";
    looper++;

    //send a custom command "get /USER/BIN/BIGTABLE.CSV"

}

void MainWindow::po(){
    qDebug() << "error occured reply --------------------";
}

void MainWindow::er(){
    qDebug() << "preshare auth required reply";
}

void MainWindow::rr(){
    qDebug() << "ready for reading reply ------------------";
}

void MainWindow::auth(){
    qDebug() << "needs authentification manager";
}

void MainWindow::uploadComplete(QNetworkReply *reply)
{
    // If the upload was successful without errors
    if (!reply->error())
    {

        //window_fpcs.workingFile.close();
        //window_fpcs.workingFile.deleteLater();  // delete object of file

        /*QFile *testFile = new QFile("C:/Users/abms1/Desktop/downloadedFile.txt");
        if(testFile->open(QFile::ReadWrite)){
            testFile->write(reply->readAll());
            testFile->close();
        }*/
        qDebug() << "from file: ";
        qDebug() << reply->isFinished();
        //qDebug() << reply->readAll();
        reply->deleteLater();   // delete object of reply
        window_fpcs.settings.fileInPlay = false;
    }else{
        qDebug() << "ERROR : ";
        qDebug() << reply->errorString();
    }
}

/*void MainWindow::uploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    // Display the progress of the upload
    ui->progressBar->setValue(100 * bytesSent/bytesTotal);
}*/

/*void MainWindow::output_to_command_line_text_editor(){
    //output all messages from cmd to the text editor for the user to view
    QProcess* ftpProcess = (QProcess*) sender();

    QByteArray o = ftpProcess->readAll();
    QByteArray output = ftpProcess->readAllStandardOutput();
    QByteArray errorOutput = ftpProcess->readAllStandardError();

    QStringList oL = QString(o).split("\n");
    QStringList outputList = QString(output).split("\n");
    QStringList outputErrorList = QString(errorOutput).split("\n");

    qDebug() << "readAll: \n";
    foreach(QString line, oL){
        qDebug() << line;
    }
    qDebug() << "Output: \n";
    foreach(QString line, outputErrorList){
        qDebug() << line;
    }
    qDebug() << "Error: \n";
    foreach(QString line, outputList){
        qDebug() << line;
    }
    //ui->command_line_edit_text_editor->append()
}*/

void MainWindow::on_batch_pattern_entry_push_button_clicked()
{
    //prompt the user to select a folder
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"),
                QString(), tr("Text Files (*.txt);;CSV Files (*.csv)"));

    //TODO setup a temp qTextStreamer to read in the file and basically create a table out of this text file input assuming all patterns follow the current settings

}

