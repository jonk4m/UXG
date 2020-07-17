#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDebug"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    MainWindow::fpcs_setup();
    window_ftpManager = new FtpManager(this);

    ui->uxg_fpcs_files_combo_box->hide();
    ui->delete_table_from_uxg_push_button->hide();
    ui->table_or_pattern_toolbox->setCurrentIndex(0);
    ui->table_or_pattern_toolbox->setItemEnabled(1,false);
    output_to_console("Program started.");
    qDebug() << "Program started.";
}

MainWindow::~MainWindow()
{
    window_fpcs.data_dump_onto_file();
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

//Dialog Buttons for Creating Table
void MainWindow::on_create_new_table_button_box_accepted()
{
    window_fpcs.data_dump_onto_file();
    window_fpcs.close_file(); //close the file (which also flushes the buffer) before exiting

    window_fpcs.settings.usingExistingTable = false;
    bool fileInitialized = window_fpcs.initialize_workingFile();
    if(fileInitialized){
        //make the progress bar move
        ui->loaded_table_progress_bar->reset();
        for(int j=0; j<=100; j++){
            ui->create_progress_bar->setValue(j);
            QThread::msleep(3);
        }
        ui->current_table_line_edit->setText(window_fpcs.workingFile.fileName());
    }else{
        output_to_console("File unable to initialize");
        qDebug() << "File unable to initialize";
    }

    //update the table visualization
    update_table_visualization();
}

void MainWindow::on_create_new_table_button_box_helpRequested()
{
    //TODO open a help pop-up
}

void MainWindow::on_select_existing_table_button_box_helpRequested()
{
    //TODO open a help pop-up
}

//allow the user to select a preexisting file (TODO the header is checked when the "Open" button calls on Fpcs::initialize_workingFile() to check it)
void MainWindow::on_select_file_push_button_clicked()
{
    if(window_fpcs.settings.usingExistingTableLocal == true){
    //prompt the user to select a folder from the local system
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"),
                "/fileFolder/downloads", tr("CSV Files (*.csv);;Text Files (*.txt)"));

    window_fpcs.settings.existingTableFilePath = filePath; //set the filePath

    ui->select_file_line_edit->setText(window_fpcs.settings.existingTableFilePath); //update the EditLine Box so the user can see the filePath they just chose

    //set the progress bar to 0%
    ui->create_progress_bar->reset();
    ui->loaded_table_progress_bar->reset();
    }else{
        output_to_console("Cannot select local file with option to choose file from UXG currently selected");
        qDebug() << "Cannot select local file with option to choose file from UXG currently selected";
    }
}

void MainWindow::on_update_current_table_with_pattern_push_button_clicked()
{
    if(window_fpcs.workingEntry.bitPattern != ""){
        if(highlightedFpcsRow == -1){
            //add a new pattern
            window_fpcs.add_entry(window_fpcs.workingEntry);
        }else{
            //replace the existing pattern
            window_fpcs.workingEntryList.replace(highlightedFpcsRow, window_fpcs.workingEntry);
        }

        ui->table_or_pattern_toolbox->setCurrentIndex(0); //this then opens the part of the GUI toolbox that is used for visualizing the table
        update_table_visualization();
    }else{
        qDebug() << "The pattern you have created is empty, so it was not added to the table!";
    }
}

/*
 * Note that we intentionally do not reset the entry fields here when creating another entry. The intention is that if the user is making several patterns quickly with only minor changes
 * between each pattern, they will not have to worry about changing every field each time.
 */
void MainWindow::on_add_another_pattern_to_this_table_push_button_clicked()
{
    if(window_fpcs.settings.fileInPlay){
        Entry newEntry = *new Entry();
        highlightedFpcsRow = -1;
        //copy over basic settings of the previous entry to be the same as this entry so the GUI doesn't need to be updated
        newEntry.codingType = window_fpcs.workingEntry.codingType;
        newEntry.bitsPerSubpulse = window_fpcs.workingEntry.bitsPerSubpulse;
        newEntry.phases = window_fpcs.workingEntry.phases;
        newEntry.freqs = window_fpcs.workingEntry.freqs;
        newEntry.freqUnits = window_fpcs.workingEntry.freqUnits;
        newEntry.numOfPhasesOrFreqs = window_fpcs.workingEntry.numOfPhasesOrFreqs;

        //set that new entry to be the workingEntry
        window_fpcs.workingEntry = newEntry;
        update_pattern_edit_visualization();
        ui->table_or_pattern_toolbox->setCurrentIndex(1); //this then opens the part of the GUI toolbox that is used for editing the pattern
    }else{
        output_to_console("No Table Currently Selected to Add a Pattern to.");
        qDebug() << "No Table Currently Selected to Add a Pattern to.";
        return;
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
    ui->phase_freq_pattern_entry_table->showColumn(3);
    //hide the freq columns
    ui->phase_freq_pattern_entry_table->hideColumn(1);
    ui->phase_freq_pattern_entry_table->hideColumn(2);

    window_fpcs.workingEntry.codingType = "PHASE";
}

/*
 * This radio button shows the freq pattern column and hides the phase pattern column
 */
void MainWindow::on_freq_pattern_type_radio_button_clicked()
{
    ui->phase_freq_pattern_entry_table->showColumn(1);
    ui->phase_freq_pattern_entry_table->showColumn(2);
    ui->phase_freq_pattern_entry_table->showColumn(3);
    //hide the phase columns
    ui->phase_freq_pattern_entry_table->hideColumn(0);

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

    window_fpcs.workingEntry.numOfPhasesOrFreqs = arg1;
}

void MainWindow::add_button_in_pattern_table_setup(int row)
{
        //Then update the binary pattern QString and display this as well in the binary pattern text window
        switch(window_fpcs.workingEntry.bitsPerSubpulse){
        case 1:
            if(row <= 1){
                //This line creates a string from the row int converted into binary (hence the 2), and add zero padding if the number of bits does not reach 1 bits (which it always will for 1 bit though)
                QString rowNumberStringInBinary = QString("%1").arg(row, 1, 2, QChar('0')); //convert the row int to a binary QString
                window_fpcs.workingEntry.bitPattern.append(rowNumberStringInBinary); //append this new chunk of bits to the bit pattern
                ui->pattern_binary_pattern_shown_text_editor->setText(window_fpcs.workingEntry.bitPattern); //update the ui with the bit pattern
            }else{
                output_to_console("bits per subpulse mismatch with row number being added");
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
                output_to_console("bits per subpulse mismatch with row number being added");
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
                output_to_console("bits per subpulse mismatch with row number being added");
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
                output_to_console("bits per subpulse mismatch with row number being added");
                qDebug() << "bits per subpulse mismatch with row number being added";
            }
            break;
        default:
            output_to_console("Bits per subpulse value is invalid");
            qDebug() << "Bits per subpulse value is invalid";
            break;
        }

        window_fpcs.workingEntry.parse_entry_for_plain_text_pattern();
        ui->pattern_nonBinary_values_shown_text_editor->clear();
        ui->pattern_nonBinary_values_shown_text_editor->insertPlainText(window_fpcs.workingEntry.plainTextRepresentation);
}

void MainWindow::on_phase_freq_pattern_entry_table_cellChanged(int row, int column)
{
    switch(column){
    case 0 : { //case 0 is the phase column
        //this gets the TableWidgetItem that was changed, gets it's QString text, converts it to an Int, then sets that equal to the phase
        window_fpcs.workingEntry.phases.replace(row, ui->phase_freq_pattern_entry_table->item(row, column)->text().remove("°"));
        QString currentText = ui->phase_freq_pattern_entry_table->item(row,column)->text();
        if(currentText.contains("°")){
                break; //prevent infinite loop
        }
        //add the degree symbol on the end of their number
        currentText = currentText + "°";
        ui->phase_freq_pattern_entry_table->item(row,column)->setText(currentText);
        break;
    }
    case 1: { //case 1 is the freq column
        window_fpcs.workingEntry.freqs.replace(row, ui->phase_freq_pattern_entry_table->item(row, column)->text());
        break;
    }
    case 2: { //case 2 is the freq units
        //set the freq units but remove the ending "hz"
        window_fpcs.workingEntry.freqUnits.replace(row, ui->phase_freq_pattern_entry_table->item(row, column)->text().remove("hz"));
        QString currentText = ui->phase_freq_pattern_entry_table->item(row,column)->text();
        if(currentText.contains("hz")){
                break; //prevent infinite loop
        }
        currentText = currentText + "hz";
        ui->phase_freq_pattern_entry_table->item(row,column)->setText(currentText);
        break;
    }
    case 3: { //case 3 is the add button
        //refuse to let the user change it from "ADD"
        ui->phase_freq_pattern_entry_table->item(row,column)->setText("ADD");
        break;
    }
    default: {
        output_to_console("Column in table chosen outside scope of the table");
        qDebug() << "Column in table chosen outside scope of the table";
        return;
    }
    }
}

void MainWindow::on_phase_freq_pattern_entry_table_cellClicked(int row, int column)
{
    if(column == 3){
        add_button_in_pattern_table_setup(row);
    }
}

void MainWindow::on_remove_last_entry_pushbutton_clicked()
{

    //remove that entry from the binaryPattern of the entry struct
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

    //update the plainText Pattern and update the gui
    window_fpcs.workingEntry.parse_entry_for_plain_text_pattern();
    ui->pattern_nonBinary_values_shown_text_editor->clear();
    ui->pattern_nonBinary_values_shown_text_editor->insertPlainText(window_fpcs.workingEntry.plainTextRepresentation);
}

//TODO
/*void MainWindow::uploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    // Display the progress of the upload
    ui->progressBar->setValue(100 * bytesSent/bytesTotal);
}*/

/*
 * An important part of the functionality of this function is that window_fpcs.workingFile.fileName() is the filename and directory whereas we set
 * the uploadingFileOrFolderName field to just be the name of the file without the directory. The reason behind this is so that the included directory
 * is needed for the ftp process (since it utilizes windows's ftp client routine and has no idea what the "current directory" is) whereas when the
 * ftp process is finished we send scpi commands to the UXG that only need the fileName (since it's on the UXG now that the ftp process is finished).
 */
void MainWindow::on_qprocess_upload_push_button_clicked()
{
    if(window_ftpManager->tcpSocket->state() == QAbstractSocket::ConnectedState){
        if(window_fpcs.settings.fileInPlay == true){
            window_fpcs.data_dump_onto_file();
            window_ftpManager->current_state = FtpManager::state::uploading;
            QString filename = window_fpcs.workingFile.fileName();

            //change the ftpManager field to have the fileName without the directory
            QFileInfo fileInfo(window_fpcs.workingFile.fileName());
            QString fileNameWithoutDirectory(fileInfo.fileName());
            window_ftpManager->uploadingFileOrFolderName = fileNameWithoutDirectory;

            //feed fileName with current directory into the process that will upload the csv to the uxg bin directory
            window_ftpManager->start_process(filename);
        }else{
            output_to_console("Cannot upload to UXG without selecting a file");
            qDebug() << "Cannot upload to UXG without selecting a file";
        }
    }else{
        output_to_console("Cannot upload to UXG without connecting TCPSocket first");
        qDebug() << "Cannot upload to UXG without connecting TCPSocket first";
    }
}

void MainWindow::on_download_all_files_from_uxg_push_button_clicked()
{
    window_ftpManager->current_state = FtpManager::state::downloading;
    QString folderName = QDir::currentPath() + "/fileFolder/downloadFtpCommands.txt"; //note that the downloads folder must be added upon deployment
    window_ftpManager->start_process(folderName);
}

void MainWindow::on_pushButton_clicked()
{
    QString host = "169.254.24.85";
    window_ftpManager->connect(host, 5025);  //K-N5193A-90114 or 169.254.24.85
}

void MainWindow::on_pushButton_2_clicked()
{
    QString message = "*IDN?";
    window_ftpManager->send_SPCI(message);
}

void MainWindow::on_download_all_files_from_uxg_push_button_2_clicked()
{
    MainWindow::on_download_all_files_from_uxg_push_button_clicked();
}

void MainWindow::on_select_local_file_radio_button_clicked()
{
    window_fpcs.settings.usingExistingTableLocal = true;
    ui->select_file_push_button->show();
    ui->select_file_line_edit->show();
    ui->uxg_fpcs_files_combo_box->hide();
    ui->delete_table_from_uxg_push_button->hide();
    ui->uxg_fpcs_files_combo_box->clear();
}

void MainWindow::on_select_file_from_uxg_radio_button_clicked()
{
     window_fpcs.settings.usingExistingTableLocal = false;
     //Make the drop down menu cover the "select File" button and the text editor next to it
     ui->select_file_push_button->hide();
     ui->select_file_line_edit->hide();
     ui->uxg_fpcs_files_combo_box->show();
     ui->delete_table_from_uxg_push_button->show();

     window_ftpManager->waitingForFPCSFileList = true;
     window_ftpManager->send_SPCI(":MEMory:CATalog:FPCSetup?");
}

void MainWindow::on_cancel_editing_pattern_push_button_clicked()
{
    ui->table_or_pattern_toolbox->setCurrentIndex(0);
}

void MainWindow::on_clear_pattern_push_button_clicked()
{
    window_fpcs.workingEntry.bitPattern.clear();
    ui->pattern_binary_pattern_shown_text_editor->clear();
    window_fpcs.workingEntry.parse_entry_for_plain_text_pattern();
    ui->pattern_nonBinary_values_shown_text_editor->clear();
    ui->pattern_nonBinary_values_shown_text_editor->insertPlainText(window_fpcs.workingEntry.plainTextRepresentation);
}

void MainWindow::on_power_off_uxg_push_button_clicked()
{
    switch( QMessageBox::question(
                this,
                tr("Power Down"),
                tr("Are you sure you want to power down?"),
                QMessageBox::Yes |
                QMessageBox::Cancel,
                QMessageBox::Cancel ) )
    {
      case QMessageBox::Yes:
        window_ftpManager->send_SPCI(":SYSTem:PDOWn");
        break;
      case QMessageBox::Cancel:
        //do nothing, the messageBox will close itself upon a selection
        break;
      default:
        //do nothing, the messageBox will close itself upon a selection
        break;
    }
}

void MainWindow::on_initiate_uxg_self_test_push_button_clicked()
{
    window_ftpManager->send_SPCI("*TST?");
}

void MainWindow::on_delete_all_files_on_uxg_push_button_clicked()
{
    switch( QMessageBox::question(
                this,
                tr("Delete All Files on UXG"),
                tr("This will delete all PDW, Binary, and FPCS Files on the connected UXG. \n\t\t\tAre you sure?"),
                QMessageBox::Yes |
                //QMessageBox::No |
                QMessageBox::Cancel,
                QMessageBox::Cancel ) )
    {
      case QMessageBox::Yes:
        window_ftpManager->send_SPCI(":MEMory:DELete:ALL");
        break;
      //case QMessageBox::No:
        //do nothing, the messageBox will close itself upon a selection
      //break;
      case QMessageBox::Cancel:
        //do nothing, the messageBox will close itself upon a selection
        break;
      default:
        //do nothing, the messageBox will close itself upon a selection
        break;
    }
}

void MainWindow::on_binary_data_view_push_button_clicked(bool checked)
{
    window_fpcs.settings.usingBinaryDataView = checked;
    //TODO call a method window_fpcs.updateTableVisualizer(); which will check that bool and update the table view to be either binary bits or the layman's [phase,freq],... view
}

void MainWindow::on_socket_readyRead(){
    output_to_console("Socket readyRead");
    qDebug() << "Socket readyRead";
    QString allRead = window_ftpManager->tcpSocket->readAll();
    QStringList allReadParsed = allRead.split(QRegExp("[\r\n]"), Qt::SkipEmptyParts);
    for(QString item : allReadParsed){
        qDebug() << item;
        output_to_console(item);
    }

    //this process is specifically for when the user needs to know what all available FPCS files on the UXG are.
    if(window_ftpManager->waitingForFPCSFileList){
        //clear the current list of available files on the dropdown menu
        ui->uxg_fpcs_files_combo_box->clear();

        output_to_console("FPCS files : ");
        qDebug() << "FPCS files : ";
        QStringList allReadParsedFPCS = allRead.split(QLatin1Char(','),Qt::SkipEmptyParts);
        QStringList outputList;
        for(QString item : allReadParsedFPCS){
            if(item.contains("@")){
                outputList << item.remove("FPCS").remove("@").remove('"').remove('.');
            }
        }
        window_ftpManager->waitingForFPCSFileList = false;
        ui->uxg_fpcs_files_combo_box->addItems(outputList);
    }

    //this process is specifically for when the user is downloading a uxg fpcs file to then edit.
    /*if(window_ftpManager->downloadState == window_ftpManager->exportingTable){
        window_ftpManager->downloadState = window_ftpManager->finished;
        output_to_console("file is exported, ready for ftp");
        qDebug() << "file is exported, ready for ftp";
        //download the table from the uxg into the downloads folder
        window_ftpManager->current_state = FtpManager::state::downloading;
        //then feed it into the process
        window_ftpManager->start_process(window_fpcs.settings.existingTableFilePath); //note that start_process() only needs the name of the file
        //note we can immediately use the file after starting the process since it's a blocking call
        bool fileInitialized = window_fpcs.initialize_workingFile();
        if(fileInitialized){
            //make the progress bar move
            ui->create_progress_bar->reset();
            for(int j=0; j<=100; j++){
                ui->loaded_table_progress_bar->setValue(j);
                QThread::msleep(3);
            }
            ui->current_table_line_edit->setText(window_fpcs.workingFile.fileName());
            update_table_visualization();
        }else{
            output_to_console("File unable to initialize");
            qDebug() << "File unable to initialize";
        }
    }*/
}

void MainWindow::on_uxg_fpcs_files_combo_box_currentTextChanged(const QString &arg1)
{
    window_fpcs.settings.existingTableFilePath = arg1;
    window_fpcs.settings.existingTableFilePath = window_fpcs.settings.existingTableFilePath.remove('"').remove(" ");
}

/*
 * Dialog Buttons for loading Table
 * Assumes a file name has been chosen before pressed, but also checks if that's the case.
 *
 */
void MainWindow::on_select_existing_table_button_box_accepted()
{
    window_fpcs.data_dump_onto_file();
    window_fpcs.close_file(); //close the file (which also flushes the buffer) before exiting

    window_fpcs.settings.usingExistingTable = true;
    window_fpcs.workingEntryList.clear();

    if(window_fpcs.settings.usingExistingTableLocal == false){
        /*
         * extra steps required here to import the file from the uxg, then we initialize the file as if it already existed locally
         * tell the uxg to export the fpcs file as a csv
         */

        //select the chosen table as the current table on the UXG
        QString scpiCommand = ":SOURce:PULM:STReam:FPCSetup:SELect "; //Note this command will be rejected if the UXG is not in PDW Streaming Mode
        scpiCommand.append('"');
        scpiCommand.append(window_fpcs.settings.existingTableFilePath.append(".fpcs"));
        scpiCommand.append('"');
        output_to_console("sending: " + scpiCommand);
        qDebug() << "sending: " << scpiCommand;
        window_ftpManager->send_SPCI(scpiCommand);

        output_to_console("file set to current table");
        qDebug() << "file set to current table";
        window_ftpManager->downloadState = window_ftpManager->exportingTable;
         //export the fpcs file as a csv into the UXG's BIN directory
        scpiCommand = ":MEMory:EXPort:ASCii:FPCSetup ";
        scpiCommand.append('"');
        scpiCommand.append(window_fpcs.settings.existingTableFilePath.remove(".fpcs").append(".csv"));
        scpiCommand.append('"');
        output_to_console(scpiCommand);
        qDebug() << "now sending: " << scpiCommand;
        window_ftpManager->send_SPCI(scpiCommand);

        //-------------------------
        output_to_console("file is exported, ready for ftp");
        qDebug() << "file is exported, ready for ftp";
        //download the table from the uxg into the downloads folder
        window_ftpManager->current_state = FtpManager::state::downloading;
        //then feed it into the process
        window_ftpManager->start_process(window_fpcs.settings.existingTableFilePath); //note that start_process() only needs the name of the file
        //note we can immediately use the file after starting the process since it's a blocking call
        window_fpcs.settings.existingTableFilePath = QDir::currentPath() + "/fileFolder/downloads/" + window_fpcs.settings.existingTableFilePath;
        bool fileInitialized = window_fpcs.initialize_workingFile();
        if(fileInitialized){
            //make the progress bar move
            ui->create_progress_bar->reset();
            for(int j=0; j<=100; j++){
                ui->loaded_table_progress_bar->setValue(j);
                QThread::msleep(3);
            }
            ui->current_table_line_edit->setText(window_fpcs.workingFile.fileName());
            update_table_visualization();
        }else{
            output_to_console("File unable to initialize");
            qDebug() << "File unable to initialize";
        }
        //--------------------------------

    }else{
        bool fileInitialized = window_fpcs.initialize_workingFile();
        if(fileInitialized){
            //make the progress bar move
            ui->create_progress_bar->reset();
            for(int j=0; j<=100; j++){
                ui->loaded_table_progress_bar->setValue(j);
                QThread::msleep(3);
            }
            ui->current_table_line_edit->setText(window_fpcs.workingFile.fileName());
            update_table_visualization();
        }else{
            output_to_console("File unable to initialize");
            qDebug() << "File unable to initialize";
        }
    }
}

/*
 * When this function is called, it's assumed the exported file is already in the UXG's BIN folder
 * qDebug() << "initializing : " << settings.existingTableFileNameUxg;
    workingFile.setFileName(this->settings.existingTableFileNameUxg); //set this file path to be for the working file
 */
/*bool MainWindow::pre_initialize_uxg_file(){
    //download the table from the uxg into the downloads folder
    window_ftpManager->current_state = FtpManager::state::downloading;
    //then feed it into the process
    window_ftpManager->start_process(window_fpcs.settings.existingTableFilePath); //note that start_process() only needs the name of the file
    //we can immediately begin using the file after this call because when the window_ftpManager current_state is downloading,
    //the start_process function will use a blocking function before returning

    //By this point the exported csv file representation of the fpcs file is downloaded over ftp into this program's downloads folder
    QFile tempFile;
    QString tempFilePath = QDir::currentPath() + "/fileFolder/downloads/" + window_fpcs.settings.existingTableFilePath;
    output_to_console("tempFilePath: " + tempFilePath);
    qDebug() << "tempFilePath: " << tempFilePath;
    tempFile.setFileName(tempFilePath);
    bool exists = tempFile.exists(tempFilePath);
    if(exists){
        if(!tempFile.open(QFile::ReadWrite | QFile::Text | QFile::ExistingOnly)) //Options: ExistingOnly, NewOnly, Append
        {
            output_to_console("Could not open File : " + tempFilePath);
            qDebug() << "Could not open File : " << tempFilePath;
            return false;
        }
    }else{
        output_to_console("File Not Found : " + tempFilePath);
        qDebug() << "File Not Found : " << tempFilePath;
        return false;
    }
    //create the QTextStreamer to operate on this file until further notice
    QTextStream tempStreamer;
    tempStreamer.setDevice(&tempFile);
    //check header
    tempStreamer.seek(0); //make sure the TextStream starts at the beginning of the file
    QString read = tempStreamer.readAll();
    output_to_console("Downloaded File Data : " + read);
    qDebug() << "Downloaded File Data : " << read;

    //TODO parse through the header of the file here and update it with the new header as well as moving data over into the correct spots thereafter
    //QString header = "Comment,State,Coding Type,Length,Bits Per Subpulse,Phase State 0 (deg),Phase State 1 (deg),Phase State 2 (deg),Phase State 3 (deg),Phase State 4 (deg),Phase State 5 (deg),Phase State 6 (deg),Phase State 7 (deg),Phase State 8 (deg),Phase State 9 (deg),Phase State 10 (deg),Phase State 11 (deg),Phase State 12 (deg),Phase State 13 (deg),Phase State 14 (deg),Phase State 15 (deg),Frequency State 0 (Hz),Frequency State 1 (Hz),Frequency State 2 (Hz),Frequency State 3 (Hz),Frequency State 4 (Hz),Frequency State 5 (Hz),Frequency State 6 (Hz),Frequency State 7 (Hz),Frequency State 8 (Hz),Frequency State 9 (Hz),Frequency State 10 (Hz),Frequency State 11 (Hz),Frequency State 12 (Hz),Frequency State 13 (Hz),Frequency State 14 (Hz),Frequency State 15 (Hz),Hex Pattern\n";
    //tempStreamer << header;
    //tempStreamer.flush(); //ensure all the data is written to the file before moving on

    window_fpcs.settings.existingTableFilePath = tempFilePath;
    bool fileInitialized = window_fpcs.initialize_workingFile();
    if(fileInitialized){
        //make the progress bar move
        ui->create_progress_bar->reset();
        for(int j=0; j<=100; j++){
            ui->loaded_table_progress_bar->setValue(j);
            QThread::msleep(10);
        }
        ui->current_table_line_edit->setText(window_fpcs.workingFile.fileName());
    }else{
        output_to_console("File unable to initialize");
        qDebug() << "File unable to initialize";
    }

    return true;
    //TODO Update table visualization with the new data
}*/

void MainWindow::on_delete_table_from_uxg_push_button_clicked(){
    //make sure the correct radio button is selected
    if(window_fpcs.settings.usingExistingTableLocal == false){
        //delete the fpcs file under that name
        QString command = "MEMory:DELete:NAME ";
        command.append('"');
        command.append(window_fpcs.settings.existingTableFilePath.append(".fpcs"));
        command.append('"');
        window_ftpManager->send_SPCI(command);
        //delete the csv file under that name
        command.clear();
        command = "MEMory:DELete:NAME ";
        command.append('"');
        command.append(window_fpcs.settings.existingTableFilePath.remove(".fpcs").append(".csv"));
        command.append('"');
        window_ftpManager->send_SPCI(command);
        //reset the list of available files on the UXG
        on_select_file_from_uxg_radio_button_clicked();

        //make the progress bar move
        ui->create_progress_bar->reset();
        for(int j=0; j<=100; j++){
            ui->loaded_table_progress_bar->setValue(j);
            QThread::msleep(3);
        }
    }else{
        output_to_console("Cannot delete a table on the uxg if radio button for selecting an existing table is not enabled.");
        qDebug() << "Cannot delete a table on the uxg if radio button for selecting an existing table is not enabled.";
    }
}

void MainWindow::output_to_console(QString text){
    ui->console_text_editor->appendPlainText(text + "\n");
}

/*
 * If the "select existing table" tab is selected, automatically have the currently selected radio button option refreshed to update file names if need be.
 */
void MainWindow::on_create_or_select_table_tab_widget_currentChanged(int index)
{
    if(index == 1){
        if(window_fpcs.settings.usingExistingTableLocal){
            on_select_local_file_radio_button_clicked();
        }else{
            on_select_file_from_uxg_radio_button_clicked();
        }
    }
}

void MainWindow::on_binary_data_view_push_button_stateChanged(int arg1)
{
    window_fpcs.settings.preferredFormat = arg1;
    update_table_visualization();
}

void MainWindow::on_delete_selected_row_push_button_clicked()
{
    //Delete the Entry in the entryList that has the index of the index of the highlighted row
    window_fpcs.workingEntryList.removeAt(highlightedFpcsRow);
    update_table_visualization();
}

/*
 * This function assumes all changes that need to be reflected on the table visualization were made to the workingFile.
 * Thus, the workingFile CSV will be the reference for all changes made in this function call.
 */
void MainWindow::update_table_visualization(){
    int rowIndex = 0;

    //clear all the current text first
    int tableSize = ui->table_visualization_table_widget->size().height();
    for(int i = 0; i < tableSize; i++){
        ui->table_visualization_table_widget->item(i,0)->setText("");
    }

    if(window_fpcs.settings.preferredFormat == 0){
        //the box is unchecked so display in plaintext format
        rowIndex = 0;
        //qDebug() << "format 0 updating Table Visualization";
        for(Entry entry:window_fpcs.workingEntryList){
            //qDebug() << "plainText : " << entry.plainTextRepresentation;
            ui->table_visualization_table_widget->item(rowIndex,0)->setText(entry.plainTextRepresentation);
            rowIndex++;
        }
    }else{
        //the box is checked so display in binary format
        rowIndex = 0;
        //qDebug() << "format 1 updating Table Visualization";
        for(Entry entry:window_fpcs.workingEntryList){
            qDebug() << "plainText : " << entry.plainTextRepresentation;
            ui->table_visualization_table_widget->item(rowIndex,0)->setText(entry.bitPattern);
            rowIndex++;
        }
    }
}

/*
 * Select a pattern to edit and begin syncronizing the gui to match
 *
 * Note that when we "edit" an entry, we really create a whole new entry with the values of the entry at that row copied into it. This is because if the user selects "cancel"
 * we can simply delete that entry object with the copied values and nothing in the fpcs's entry list changes. If the user commits to their changes we simply replace the
 * original entry object in the list with our new one.
 */
void MainWindow::on_edit_selected_row_push_button_clicked(){

    //if the selected row does not represent an entry in our list
    if(window_fpcs.workingEntryList.size() - 1 < highlightedFpcsRow){
        qDebug() << "Please select a valid pattern from the table.";
    }
    Entry newEntry = *new Entry();
    //copy over basic settings of the previous entry to be the same as this entry so the GUI doesn't need to be updated
    newEntry.codingType = window_fpcs.workingEntryList.at(highlightedFpcsRow).codingType;
    newEntry.bitsPerSubpulse = window_fpcs.workingEntryList.at(highlightedFpcsRow).bitsPerSubpulse;
    newEntry.phases = window_fpcs.workingEntryList.at(highlightedFpcsRow).phases;
    newEntry.freqs = window_fpcs.workingEntryList.at(highlightedFpcsRow).freqs;
    newEntry.freqUnits = window_fpcs.workingEntryList.at(highlightedFpcsRow).freqUnits;
    newEntry.bitPattern = window_fpcs.workingEntryList.at(highlightedFpcsRow).bitPattern;
    newEntry.numOfPhasesOrFreqs = window_fpcs.workingEntryList.at(highlightedFpcsRow).numOfPhasesOrFreqs;
    newEntry.plainTextRepresentation = window_fpcs.workingEntryList.at(highlightedFpcsRow).plainTextRepresentation;

    //set that new entry to be the workingEntry
    window_fpcs.workingEntry = newEntry;
    update_pattern_edit_visualization();
    ui->table_or_pattern_toolbox->setCurrentIndex(1); //this then opens the part of the GUI toolbox that is used for editing the pattern
}

void MainWindow::on_table_visualization_table_widget_cellClicked(int row, int)
{
    highlightedFpcsRow = row;
}

void MainWindow::update_pattern_edit_visualization(){
    ui->pattern_nonBinary_values_shown_text_editor->clear();
    ui->pattern_binary_pattern_shown_text_editor->clear();

    //use the workingEntry as the source of data for calling on signals of the pattern edit page
    ui->how_many_different_phase_or_freq_spin_box->setValue(window_fpcs.workingEntry.numOfPhasesOrFreqs);

    if(window_fpcs.workingEntry.codingType == "BOTH"){
        ui->both_pattern_type_radio_button->setChecked(true);
        ui->both_pattern_type_radio_button->click();
        ui->freq_pattern_type_radio_button->setChecked(false);
        ui->phase_pattern_type_radio_button->setChecked(false);
    }else if(window_fpcs.workingEntry.codingType == "FREQUENCY"){
        ui->both_pattern_type_radio_button->setChecked(false);
        ui->freq_pattern_type_radio_button->setChecked(true);
        ui->freq_pattern_type_radio_button->click();
        ui->phase_pattern_type_radio_button->setChecked(false);
    }else if(window_fpcs.workingEntry.codingType == "PHASE"){
        ui->both_pattern_type_radio_button->setChecked(false);
        ui->freq_pattern_type_radio_button->setChecked(false);
        ui->phase_pattern_type_radio_button->setChecked(true);
        ui->phase_pattern_type_radio_button->click();
    }else{
        qDebug() << "Error, unknown codingType for working Entry";
    }

    ui->pattern_nonBinary_values_shown_text_editor->setText(window_fpcs.workingEntry.plainTextRepresentation);
    ui->pattern_binary_pattern_shown_text_editor->setText(window_fpcs.workingEntry.bitPattern);

    //enter in the phase values
    int count = 0;
    for(QString phase:window_fpcs.workingEntry.phases){
        ui->phase_freq_pattern_entry_table->item(count, 0)->setText(phase + "°");
        count++;
    }

    //enter in the freq values
    count = 0;
    for(QString freq:window_fpcs.workingEntry.freqs){
        ui->phase_freq_pattern_entry_table->item(count, 1)->setText(freq);
        count++;
    }

    //enter in the freq values
    count = 0;
    for(QString freqUnits:window_fpcs.workingEntry.freqUnits){
        ui->phase_freq_pattern_entry_table->item(count, 2)->setText(freqUnits + "hz");
        count++;
    }
}

