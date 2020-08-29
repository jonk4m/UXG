#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDebug"
//Hello Jon,  Rose is really depressing with all of their anti-COVID measures
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    MainWindow::fpcs_setup();
    window_ftpManager = new FtpManager(this);
    window_yatg = new YATG(window_ftpManager, this);

    ui->uxg_fpcs_files_combo_box->hide();
    ui->delete_table_from_uxg_push_button->hide();
    ui->table_or_pattern_toolbox->setCurrentIndex(0);
    ui->table_or_pattern_toolbox->setItemEnabled(1,false);
    ui->host_name_edit_line->setText(window_ftpManager->hostName);
    output_to_console("Program started.");
    qDebug() << "Program started.";
    setup();
}

MainWindow::~MainWindow()
{
    if(fpcsWasOpenedDuringUse){
        switch( QMessageBox::question(
                    this,
                    tr("Save Before Exiting"),
                    tr("Save Current Table to File Before Exiting?\n"),
                    QMessageBox::Yes |
                    QMessageBox::No) )
        {
        case QMessageBox::Yes:
            window_fpcs->data_dump_onto_file();
            window_fpcs->close_file(); //close the file (which also flushes the buffer) before exiting
            delete ui;
            break;
        case QMessageBox::No:
            window_fpcs->close_file(); //close the file (which also flushes the buffer) before exiting
            delete ui;
            break;
        default:
            //do nothing, the messageBox will close itself upon a selection
            break;
        }
    }
    udpSocket->closeUdpSocket();
    serial->closeSerialPorts();
}

void MainWindow::setup(){
    udpSocket = new UdpSocket(this);
    serial = new RotorControl(this);//initialize the serial pointer
    on_usingRotorCheckBox_toggled(false);
    //connects the serial ports between the rotor and the
    mainTimer = new QTimer(this);//starts a timer to update the LCD displays in real time
    connect(mainTimer, SIGNAL(timeout()), this, SLOT(updatePositions()));//connect the timer to the updatePositions slot (down below)
    mainTimer->start(200);//the timer will go off every 200ms
    stopMotionTimer = new QTimer(this);
    connect(stopMotionTimer, SIGNAL(timeout()), this, SLOT(stopMotion()));

    ui->testCreatorTableWidget->viewport()->installEventFilter(this);
    ui->testCreatorTableWidget->installEventFilter(this);
    QList<QString> resolution = ui->tableResolutionLineEdit->text().split(',');
    resetTable(resolution.at(0).toInt(), resolution.at(1).toInt());
    //scrolling to an item doesn't put it in the center of the table, so this scrolls past the origin
    //by 15
    QTableWidgetItem *originOffset = ui->testCreatorTableWidget->item(95,102);
    ui->testCreatorTableWidget->scrollToItem(originOffset);
    //set the origin to black to make it easy to see
    ui->testCreatorTableWidget->item(90,90)->setBackground(Qt::black);
}

void MainWindow::fpcs_setup(){
    //configured on setup
    window_fpcs = new Fpcs(this);
    ui->table_directory_line_edit->setText(window_fpcs->settings.defaultFilePath); //make the directory text line start out with default file path
    window_fpcs->settings.tablePath = window_fpcs->settings.defaultFilePath; //set the current filepath to be the default path on startup
    ui->default_name_line_edit->setText(window_fpcs->settings.defaultTableName);
}

void MainWindow::on_specify_table_name_radio_button_clicked()
{
    window_fpcs->settings.usingCustomTableName = true;
    output_to_console("usingCustomTableName = true");
}

/*
 * When the "use default name" button is pressed, our settings for which name to use is updated, and the default name is shown as well as the default
 * filePath
 */
void MainWindow::on_use_default_name_radio_button_clicked()
{
    window_fpcs->settings.usingCustomTableName = false; //when this is false, we use the defaultTableName and defaultFilePath

    ui->default_name_line_edit->setText(window_fpcs->settings.defaultTableName);
}

//Note that the filePath is set when the user presses the "Ok" button not in this slot
void MainWindow::on_change_table_directory_push_button_clicked()
{
    //prompt the user to select a folder
    QString folderName = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                           "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    //check if the user pressed "cancel"
    if(folderName.size() == 0){
        output_to_console("cancelled selecting folder");
        return;
    }

    ui->table_directory_line_edit->setText(folderName);

}

//Dialog Buttons for Creating Table
void MainWindow::on_create_new_table_button_box_accepted()
{
    output_to_console("ok button pressed, TableName: " + window_fpcs->settings.tableName + "   tablePath: " + window_fpcs->settings.tablePath);

    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    //handle GUI options before calling any other class functions
    if(window_fpcs->settings.usingCustomTableName == true){
        //if they checked the radio button for using a custom table name, let's go ahead and change our Table name to whatever they typed in
        //check that they typed something in
        if(ui->specified_table_name_line_edit->text().size() == 0){
            output_to_console("No custom name for the table has been entered. Please enter a name and try again.");
            QGuiApplication::restoreOverrideCursor();
            return;
        }
        window_fpcs->settings.tableName = ui->specified_table_name_line_edit->text();
        //Note that since the tableName is being declared here, it does not matter if the tableName is specified in the "existing table" tab because this will overright it

    }else{
        window_fpcs->settings.tableName = window_fpcs->settings.defaultTableName;
    }

    //set the tablePath
    window_fpcs->settings.tablePath = ui->table_directory_line_edit->text();

    fpcsWasOpenedDuringUse = true;

    window_fpcs->data_dump_onto_file();
    window_fpcs->close_file(); //close the file (which also flushes the buffer) before exiting

    bool fileInitialized = window_fpcs->initialize_workingFile(false); //false means we are creating a new table
    if(fileInitialized){
        ui->current_table_line_edit->setText(window_fpcs->workingFile.fileName());
    }else{
        output_to_console("File unable to initialize");
        qDebug() << "File unable to initialize";
    }

    update_table_visualization();

    QGuiApplication::restoreOverrideCursor();
}

void MainWindow::on_create_new_table_button_box_helpRequested()
{
    //open a help pop-up
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setStandardButtons(QMessageBox::Ok);
    QString helpText = "How to Create a new Frequency Phase Coding Setup Table for use-cases like Barker codes, Modulation On Pulse, etc. : \n\n   This is done by typing in a name for the table and selecting the 'Specify Table Name' radio button then clicking the 'OK' button, or by using the default table name. The directory for where the file is stored can be selected using the 'Change Table Directory' button PRIOR to pressing the 'OK' button that creates the file. Once the 'OK' button is pressed, the user may begin to edit the table by selecting one of the options in the 'Table Visualization' groupbox.";
    msgBox->setInformativeText(helpText);
    msgBox->open( this, SLOT(msgBoxClosed(QAbstractButton*)));
}

void MainWindow::on_select_existing_table_button_box_helpRequested()
{
    //open a help pop-up
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setStandardButtons(QMessageBox::Ok);
    QString helpText = "How to Load a Frequency Phase Coding Setup Table for use-cases like Barker codes, Modulation On Pulse, etc. : \n\n   This is done by either selecting the 'Select Local File' radio button or the 'Select File from UXG' radio button to start. If the user is selecting from the local system, they will click the 'Select File' button to browse their directories until finding the file, then click 'Open' to open that file for editing. Only after pressing the 'Open' button can the file then be uploaded to the UXG by pressing the 'Upload Finished Table to UXG' button. If the user is selecting a file from the UXG, the dropdown menu will allow them to choose available files on the UXG. Once a name is selected (The top name is selected by default) the rest of the process follows the same as selecting from their local directory.";
    msgBox->setInformativeText(helpText);
    msgBox->open( this, SLOT(msgBoxClosed(QAbstractButton*)));
}

//allow the user to select a preexisting file
void MainWindow::on_select_file_push_button_clicked()
{
    if(window_fpcs->settings.usingExistingTableLocal == true){
        //prompt the user to select a file from the local system
        QString filePathAndName = QFileDialog::getOpenFileName(this, tr("Open Table File"),
                                                               ( QDir::currentPath() + "/fileFolder"), tr("CSV Files (*.csv);;Text Files (*.txt)"));

        //check if the user pressed "Cancel"
        if(filePathAndName.size() == 0){
            output_to_console("cancelled selecting file");
            return;
        }

        ui->select_file_line_edit->setText(filePathAndName); //update the EditLine Box so the user can see the filePath they just chose

    }else{
        output_to_console("Cannot select local file with option to choose file from UXG currently selected");
        qDebug() << "Cannot select local file with option to choose file from UXG currently selected";
    }
}

void MainWindow::on_update_current_table_with_pattern_push_button_clicked()
{
    if(window_fpcs->workingEntry->bitPattern != ""){
        if(highlightedFpcsRow == -1){
            //add a new pattern
            window_fpcs->add_entry(window_fpcs->workingEntry);
        }else{
            //replace the existing pattern
            window_fpcs->workingEntryList.replace(highlightedFpcsRow, window_fpcs->workingEntry);
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
    if(window_fpcs->settings.fileInPlay){
        Entry *newEntry = new Entry(this);
        highlightedFpcsRow = -1;
        //copy over basic settings of the previous entry to be the same as this entry so the GUI doesn't need to be updated
        newEntry->codingType = window_fpcs->workingEntry->codingType;
        newEntry->bitsPerSubpulse = window_fpcs->workingEntry->bitsPerSubpulse;
        newEntry->phases = window_fpcs->workingEntry->phases;
        newEntry->freqs = window_fpcs->workingEntry->freqs;
        newEntry->freqUnits = window_fpcs->workingEntry->freqUnits;
        newEntry->numOfPhasesOrFreqs = window_fpcs->workingEntry->numOfPhasesOrFreqs;

        //set that new entry to be the workingEntry
        window_fpcs->workingEntry = newEntry;
        update_pattern_edit_visualization();
        ui->table_or_pattern_toolbox->setCurrentIndex(1); //this then opens the part of the GUI toolbox that is used for editing the pattern
    }else{
        output_to_console("No Table Currently Selected to Add a Pattern to.");
        qDebug() << "No Table Currently Selected to Add a Pattern to.";
        return;
    }
}

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

    window_fpcs->workingEntry->codingType = "PHASE";
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

    window_fpcs->workingEntry->codingType = "FREQUENCY";
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

    window_fpcs->workingEntry->codingType = "BOTH";
}

void MainWindow::on_how_many_different_phase_or_freq_spin_box_valueChanged(int arg1)
{
    if(arg1 == 2){
        //bits per subpulse => 1
        window_fpcs->workingEntry->bitsPerSubpulse = 1;
    }else if(arg1 <= 4){
        //bits per subpulse => 2
        window_fpcs->workingEntry->bitsPerSubpulse = 2;
    }else if(arg1 <= 8){
        //bits per subpulse => 3
        window_fpcs->workingEntry->bitsPerSubpulse = 3;
    }else{
        //bits per subpulse => 4
        window_fpcs->workingEntry->bitsPerSubpulse = 4;
    }

    for(int i = 0; i < arg1; i++){
        ui->phase_freq_pattern_entry_table->showRow(i);
    }

    for(int i = arg1; i < 16; i++){
        ui->phase_freq_pattern_entry_table->hideRow(i);
    }

    //window_fpcs->workingEntry->bitPattern.clear();
    //window_fpcs->workingEntry->plainTextRepresentation.clear();
    window_fpcs->workingEntry->numOfPhasesOrFreqs = arg1;
}

void MainWindow::add_button_in_pattern_table_setup(int row)
{
    //Then update the binary pattern QString and display this as well in the binary pattern text window
    switch(window_fpcs->workingEntry->bitsPerSubpulse){
    case 1:
        if(row <= 1){
            //This line creates a string from the row int converted into binary (hence the 2), and add zero padding if the number of bits does not reach 1 bits (which it always will for 1 bit though)
            QString rowNumberStringInBinary = QString("%1").arg(row, 1, 2, QChar('0')); //convert the row int to a binary QString
            window_fpcs->workingEntry->bitPattern.append(rowNumberStringInBinary); //append this new chunk of bits to the bit pattern
            ui->pattern_binary_pattern_shown_text_editor->setText(window_fpcs->workingEntry->bitPattern); //update the ui with the bit pattern
        }else{
            output_to_console("bits per subpulse mismatch with row number being added");
            qDebug() << "bits per subpulse mismatch with row number being added";
        }
        break;
    case 2:
        if(row <= 3){
            //This line creates a string from the row int converted into binary (hence the 2), and add zero padding if the number of bits does not reach 2 bits.
            QString rowNumberStringInBinary = QString("%1").arg(row, 2, 2, QChar('0')); //convert the row int to a binary QString
            window_fpcs->workingEntry->bitPattern.append(rowNumberStringInBinary); //append this new chunk of bits to the bit pattern
            ui->pattern_binary_pattern_shown_text_editor->setText(window_fpcs->workingEntry->bitPattern); //update the ui with the bit pattern
        }else{
            output_to_console("bits per subpulse mismatch with row number being added");
            qDebug() << "bits per subpulse mismatch with row number being added";
        }
        break;
    case 3:
        if(row <= 7){
            //This line creates a string from the row int converted into binary (hence the 2), and add zero padding if the number of bits does not reach 3 bits.
            QString rowNumberStringInBinary = QString("%1").arg(row, 3, 2, QChar('0')); //convert the row int to a binary QString
            window_fpcs->workingEntry->bitPattern.append(rowNumberStringInBinary); //append this new chunk of bits to the bit pattern
            ui->pattern_binary_pattern_shown_text_editor->setText(window_fpcs->workingEntry->bitPattern); //update the ui with the bit pattern
        }else{
            output_to_console("bits per subpulse mismatch with row number being added");
            qDebug() << "bits per subpulse mismatch with row number being added";
        }
        break;
    case 4:
        if(row <= 15){
            //This line creates a string from the row int converted into binary (hence the 2), and add zero padding if the number of bits does not reach 4 bits.
            QString rowNumberStringInBinary = QString("%1").arg(row, 4, 2, QChar('0')); //convert the row int to a binary QString
            window_fpcs->workingEntry->bitPattern.append(rowNumberStringInBinary); //append this new chunk of bits to the bit pattern
            ui->pattern_binary_pattern_shown_text_editor->setText(window_fpcs->workingEntry->bitPattern); //update the ui with the bit pattern
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

    window_fpcs->workingEntry->parse_entry_for_plain_text_pattern();
    ui->pattern_nonBinary_values_shown_text_editor->clear();
    ui->pattern_nonBinary_values_shown_text_editor->insertPlainText(window_fpcs->workingEntry->plainTextRepresentation);
}

void MainWindow::on_phase_freq_pattern_entry_table_cellChanged(int row, int column)
{
    switch(column){
    case 0 : { //case 0 is the phase column
        //this gets the TableWidgetItem that was changed, gets it's QString text, converts it to an Int, then sets that equal to the phase
        window_fpcs->workingEntry->phases.replace(row, ui->phase_freq_pattern_entry_table->item(row, column)->text().remove("°"));
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
        window_fpcs->workingEntry->freqs.replace(row, ui->phase_freq_pattern_entry_table->item(row, column)->text());
        break;
    }
    case 2: { //case 2 is the freq units
        //set the freq units but remove the ending "hz"
        QString enteredValue = ui->phase_freq_pattern_entry_table->item(row, column)->text().remove("hz",Qt::CaseInsensitive);
        if(enteredValue == "m")
            enteredValue = "M"; // don't allow the user to put lower-case "m" only upper case "M"
        window_fpcs->workingEntry->freqUnits.replace(row, enteredValue);
        if(enteredValue.contains("hz")){
            break; //prevent infinite loop
        }
        enteredValue = enteredValue + "hz";
        ui->phase_freq_pattern_entry_table->item(row,column)->setText(enteredValue);
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
    switch(window_fpcs->workingEntry->bitsPerSubpulse){
    case 1:
        window_fpcs->workingEntry->bitPattern.remove(window_fpcs->workingEntry->bitPattern.size() - 1, 1); //remove the last character in the QString
        break;
    case 2:
        window_fpcs->workingEntry->bitPattern.remove(window_fpcs->workingEntry->bitPattern.size() - 2, 2); //remove the last 2 characters in the QString
        break;
    case 3:
        window_fpcs->workingEntry->bitPattern.remove(window_fpcs->workingEntry->bitPattern.size() - 3, 3); //remove the last 3 characters in the QString
        break;
    case 4:
        window_fpcs->workingEntry->bitPattern.remove(window_fpcs->workingEntry->bitPattern.size() - 4, 4); //remove the last four characters in the QString
        break;
    }

    //remove last part of the binaryPattern on the binary text editor
    ui->pattern_binary_pattern_shown_text_editor->setText(window_fpcs->workingEntry->bitPattern);

    //update the plainText Pattern and update the gui
    window_fpcs->workingEntry->parse_entry_for_plain_text_pattern();
    ui->pattern_nonBinary_values_shown_text_editor->clear();
    ui->pattern_nonBinary_values_shown_text_editor->insertPlainText(window_fpcs->workingEntry->plainTextRepresentation);
}

void MainWindow::on_qprocess_upload_push_button_clicked()
{
    if(window_ftpManager->tcpSocket->state() == QAbstractSocket::ConnectedState){
        if(window_fpcs->settings.fileInPlay == true){
            window_fpcs->data_dump_onto_file();

            window_ftpManager->current_state = FtpManager::state::uploading;
            QString filename = window_fpcs->workingFile.fileName();

            //change the ftpManager field to have the fileName without the directory
            QFileInfo fileInfo(window_fpcs->workingFile.fileName());
            QString fileNameWithoutDirectory(fileInfo.fileName());
            window_ftpManager->uploadingFileOrFolderName = fileNameWithoutDirectory;

            //feed fileName with current directory into the process that will upload the csv to the uxg bin directory
            QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            window_ftpManager->start_process(filename);
        }else{
            output_to_console("ERROR: Cannot upload to UXG without selecting a file and clicking the 'Open' Button first to initialize it for uploading");
            qDebug() << "ERROR: Cannot upload to UXG without selecting a file and clicking the 'Open' Button first to initialize it for uploading";
        }
    }else{
        output_to_console("Cannot upload to UXG without connecting TCPSocket first");
        qDebug() << "Cannot upload to UXG without connecting TCPSocket first";
    }
}

void MainWindow::on_download_all_files_from_uxg_push_button_clicked()
{
    window_ftpManager->current_state = FtpManager::state::downloading;
    window_ftpManager->downloadingAllFiles = true;
    QString folderName = "*"; //note that the downloads folder must be added upon deployment
    window_ftpManager->start_process(folderName);
}

void MainWindow::on_pushButton_clicked()
{
    window_ftpManager->abortTcpSocket();
    window_ftpManager->connect(5025);
}

void MainWindow::on_pushButton_2_clicked()
{
    QString message = "*IDN?";
    window_ftpManager->send_SCPI(message);
}

void MainWindow::on_download_all_files_from_uxg_push_button_2_clicked()
{
    MainWindow::on_download_all_files_from_uxg_push_button_clicked();
}

void MainWindow::on_select_local_file_radio_button_clicked()
{
    window_fpcs->settings.usingExistingTableLocal = true;
    ui->select_file_push_button->show();
    ui->select_file_line_edit->show();
    ui->uxg_fpcs_files_combo_box->hide();
    ui->delete_table_from_uxg_push_button->hide();
    ui->uxg_fpcs_files_combo_box->clear();
}

void MainWindow::on_select_file_from_uxg_radio_button_clicked()
{
    window_fpcs->settings.usingExistingTableLocal = false;
    //Make the drop down menu cover the "select File" button and the text editor next to it
    ui->select_file_push_button->hide();
    ui->select_file_line_edit->hide();
    ui->uxg_fpcs_files_combo_box->show();
    ui->delete_table_from_uxg_push_button->show();

    window_ftpManager->waitingForFPCSFileList = true;
    window_ftpManager->send_SCPI(":MEMory:CATalog:FPCSetup?");
}

void MainWindow::on_cancel_editing_pattern_push_button_clicked()
{
    ui->table_or_pattern_toolbox->setCurrentIndex(0);
}

void MainWindow::on_clear_pattern_push_button_clicked()
{
    window_fpcs->workingEntry->clear_pattern();
    ui->pattern_binary_pattern_shown_text_editor->clear();
    ui->pattern_nonBinary_values_shown_text_editor->clear();
    ui->pattern_nonBinary_values_shown_text_editor->insertPlainText(window_fpcs->workingEntry->plainTextRepresentation);
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
        window_ftpManager->send_SCPI(":SYSTem:PDOWn");
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
    window_ftpManager->send_SCPI("*TST?");
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
        window_ftpManager->send_SCPI(":MEMory:DELete:ALL");
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
    window_fpcs->settings.usingBinaryDataView = checked;
}

void MainWindow::on_socket_readyRead(){
    QString allRead = window_ftpManager->tcpSocket->readAll();
    QStringList allReadParsed = allRead.split(QRegExp("[\r\n]"), Qt::SkipEmptyParts);
    for(QString item : allReadParsed){
        if(item != "PLAY"){
            output_to_console("Socket readyRead : " + item);
        }
        qDebug() << "Socket readyRead : " << item;
    }

    //This is required so that the ftp process doesn't start before the UXG is finished exporting a FPCS file as a CSV file for us to download
    if(window_ftpManager->waitingForFPCSConversion && allRead== "1\n"){
        output_to_console("file is exported, ready for ftp");
        qDebug() << "file is exported, ready for ftp";

        //check if the file with the same name already exists in the downloads folder
        QDir directory("./fileFolder/downloads"); //get a list of all file names in the downloads folder
        QStringList files = directory.entryList(QStringList() << "*.csv" << "*.CSV" << "*.txt" << "*.TXT",QDir::Files);
        //output_to_console("fileName without ending is : " + window_fpcs->settings.tableName);
        QString fileNameWithoutType = window_fpcs->settings.tableName;
        //output_to_console("List of files in downloads folder is : " + files.join(", "));

        foreach(QString filename, files) {
            //output_to_console("fileName from list is : " + filename + "   fileName of the actual file : " + fileNameWithoutType.remove(".csv",Qt::CaseInsensitive).remove(".txt",Qt::CaseInsensitive).append(".csv").toUpper());
            if(filename == fileNameWithoutType.remove(".csv",Qt::CaseInsensitive).remove(".txt",Qt::CaseInsensitive).append(".csv").toUpper()){
                //file name matches, delete that file with .csv ending
                QString deleteFileName = directory.path() + "/" + filename;
                QFile temp(deleteFileName);
                qDebug() << "Deleting file : " << deleteFileName << "  to be replaced by identically named file on UXG";
                output_to_console("Deleting file : " + deleteFileName + "  to be replaced by identically named file on UXG");
                temp.remove();
            }
            if(filename == fileNameWithoutType.remove(".csv",Qt::CaseInsensitive).remove(".txt",Qt::CaseInsensitive).append(".txt").toUpper()){
                //file name matches, delete that file with .txt ending
                QString deleteFileName = directory.path() + "/" + filename;
                QFile temp(deleteFileName);
                qDebug() << "Deleting file : " << deleteFileName << "  to be replaced by identically named file on UXG";
                output_to_console("Deleting file : " + deleteFileName + "  to be replaced by identically named file on UXG");
                temp.remove();
            }
        }
        //if the file already exists in your download folder we need to delete it first

        //download the table from the uxg into the downloads folder
        window_ftpManager->current_state = FtpManager::state::downloading;
        //then feed it into the process
        window_ftpManager->start_process(window_fpcs->settings.tableName); //note that start_process() only needs the name of the file

        //now delete the file that the uxg created when it exported the fpcs to a csv file (note the only way the uxg knows which is which is by us adding the ".csv" string to the fileName)
        QString command = "MEMory:DELete:NAME ";
        command.append('"');
        command.append(window_fpcs->settings.tableName + ".csv");
        command.append('"');
        window_ftpManager->send_SCPI(command);

        //note we can immediately use the file after starting the process since it's a blocking call
        window_fpcs->settings.tablePath = QDir::currentPath() + "/fileFolder/downloads"; //note that we leave off the last "/" to follow the conventions of this variable outlined in the fpcs class at its declaration
        bool fileInitialized = window_fpcs->initialize_workingFile(true); //true means we are NOT creating a new table
        if(fileInitialized){
            ui->current_table_line_edit->setText(window_fpcs->workingFile.fileName());
            update_table_visualization();
        }else{
            output_to_console("File unable to initialize");
            qDebug() << "File unable to initialize";
        }
        window_ftpManager->waitingForFPCSConversion = false;
        QGuiApplication::restoreOverrideCursor();
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
                outputList << item.remove("FPCS",Qt::CaseInsensitive).remove("@",Qt::CaseInsensitive).remove('"').remove('.');
            }
        }
        window_ftpManager->waitingForFPCSFileList = false;
        ui->uxg_fpcs_files_combo_box->addItems(outputList);
        //check if their are any in the list
        if(outputList.size() == 0){
            output_to_console("It appears there are currently no tables available on the selected UXG.");
            on_select_local_file_radio_button_clicked();
        }
    }

    if(window_ftpManager->waitingForPdwUpload && allRead== "1\n"){
        window_ftpManager->waitingForPdwUpload = false;
        qDebug() << "returned 1 from uxg ";
        output_to_console("returned 1 from uxg. UXG finished converting PDW file.");
        QGuiApplication::restoreOverrideCursor();
        output_to_console("time elapsed for UXG to respond that it is finished uploading PDW file: " + QString::number(timer.elapsed()) + " milliseconds");
    }

    if(!window_ftpManager->UXGSetupFinished && allRead== "1\n"){
        window_ftpManager->UXGSetup();
    }
    else if(serial->simpleTestStarted && allRead == "ARMED\n"){
        window_ftpManager->send_SCPI(":OUTPut OFF");
        window_ftpManager->send_SCPI(":OUTPut:MODulation OFF");
        window_ftpManager->send_SCPI(":STReam:STATe OFF");
        triggerSent=false;
        if(serial->manageSimpleGridTest()){
            ui->console_text_editor->appendPlainText("Test Finished");
            serial->simpleTestStarted=false;
            on_stopTestPushButton_clicked();

        }

    }else if(serial->advancedTestStarted && allRead== "ARMED\n"){

        window_ftpManager->send_SCPI(":OUTPut OFF");
        window_ftpManager->send_SCPI(":OUTPut:MODulation OFF");
        window_ftpManager->send_SCPI(":STReam:STATe OFF");
        triggerSent=false;
        if(serial->manageAdvancedTest()){
            ui->console_text_editor->appendPlainText("Test Finished");
            serial->advancedTestStarted=false;
            on_stopTestPushButton_clicked();

        }
    }

    if(multiplePDWsPlaying&& allRead== "ARMED\n"){
        window_ftpManager->send_SCPI(":OUTPut OFF");
        window_ftpManager->send_SCPI(":OUTPut:MODulation OFF");
        window_ftpManager->send_SCPI(":STReam:STATe OFF");
        if(fileNumber==pdwFileNames.length()){
            multiplePDWsPlaying=false;
            window_ftpManager->send_SCPI(":STReam:STATe OFF");
            window_ftpManager->send_SCPI(":OUTPut OFF");
            window_ftpManager->send_SCPI(":OUTPut:MODulation OFF");
            on_stopTestPushButton_clicked();
            output_to_console("Finished with PDWs");
        }else{
            QString fileName = "'" + pdwFileNames.at(fileNumber) + "'";
            fileNumber++;
            window_ftpManager->playPDW(fileName, ui->continuousPDWCheckBox->isChecked());
            output_to_console("Playing PDW");
        }



    }
}

void MainWindow::on_uxg_fpcs_files_combo_box_currentTextChanged(const QString &arg1)
{
    QString tableName = arg1;
    tableName = tableName.remove('"').remove(" ");
    /* In this next line, we need to make a copy of the file name incase the user presses the "open" button. Pressing "Open"
     * causes the window_fpcs->settings.existingTableFilePath to change to the file in the local directory. However, we just
     * want the name to delete it, not the whole directory, so we store a copy of the name for that purpose. Why not just strip
     * the workingFile of it's filename you say? Well then in the "delete" part of the code we would need to distinguish whether
     * or not we currently have a workingFile in place which would be a hassle. This approach also allows the user to delete a
     * table without opening it first. They can just select the name and click delete.
     */
    window_fpcs->settings.currentTableSelectedThatIsOnTheUxg = tableName;
    qDebug() << "Current FPCS Table on the UXG selected is: " << tableName;
    output_to_console("Current FPCS Table on the UXG selected is: " + tableName);
}

/*
 * Dialog Buttons for loading Table
 * Assumes a file name has been chosen before pressed, but also checks if that's the case.
 */
void MainWindow::on_select_existing_table_button_box_accepted()
{
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    fpcsWasOpenedDuringUse = true; //used for when the user exits out of the mainwindow it will prompt to save the current table before exiting

    window_fpcs->data_dump_onto_file();
    window_fpcs->workingEntryList.clear();

    if(window_fpcs->settings.usingExistingTableLocal == false){ //for knowing if we are getting our existing table from the uxg or the local system, in this case from uxg

        window_fpcs->settings.tableName = ui->uxg_fpcs_files_combo_box->currentText(); //get the currently selected name

        output_to_console("open button pressed for a file on the UXG, TableName: " + window_fpcs->settings.tableName );
        /*
         * extra steps required here to import the file from the uxg, then we initialize the file as if it already existed locally
         * tell the uxg to export the fpcs file as a csv
         */
        //select the chosen table as the current table on the UXG
        QString scpiCommand = ":SOURce:PULM:STReam:FPCSetup:SELect "; //Note this command will be rejected if the UXG is not in PDW Streaming Mode
        scpiCommand.append('"');
        scpiCommand.append(window_fpcs->settings.tableName + ".fpcs");
        scpiCommand.append('"');
        output_to_console("sending: " + scpiCommand);
        qDebug() << "sending: " << scpiCommand;
        window_ftpManager->send_SCPI(scpiCommand);

        output_to_console("file set to current table");
        qDebug() << "file set to current table";
        window_ftpManager->downloadState = window_ftpManager->exportingTable;
        //export the fpcs file as a csv into the UXG's BIN directory
        scpiCommand = ":MEMory:EXPort:ASCii:FPCSetup ";
        scpiCommand.append('"');
        scpiCommand.append(window_fpcs->settings.tableName + ".csv"); //note that the ".csv" here tells the UXG to export the file with this name MINUS the .csv and output it to a file that INCLUDES the ".csv"
        scpiCommand.append('"');
        output_to_console(QString("Now sending: " + scpiCommand));
        qDebug() << "now sending: " << scpiCommand;
        window_ftpManager->waitingForFPCSConversion = true;
        window_ftpManager->send_SCPI(scpiCommand);
        window_ftpManager->send_SCPI("*OPC?");

    }else{
        QString filePathAndName = ui->select_file_line_edit->text();
        //get the file name without the directory by sepperating the string into a list of strings sepperated by "/" then using the last element in the list
        QStringList list = filePathAndName.split(QLatin1Char('/'), Qt::SkipEmptyParts);
        QString temp = list.last();
        temp.remove(".csv",Qt::CaseInsensitive).remove(".txt",Qt::CaseInsensitive);

        window_fpcs->settings.tableName = temp; //removed file extension here to follow variable's conventions outlined in fpcs class variable declaration

        window_fpcs->settings.tablePath = filePathAndName.remove("/" + window_fpcs->settings.tableName + ".csv",Qt::CaseInsensitive); //set the filePath, remove last "/" to follow convention for this variable outlined in fpcs class declaration

        output_to_console("open button pressed, TableName: " + window_fpcs->settings.tableName + "   tablePath: " + window_fpcs->settings.tablePath);

        bool fileInitialized = window_fpcs->initialize_workingFile(true);//true means we are NOT creating a new table
        if(fileInitialized){
            ui->current_table_line_edit->setText(window_fpcs->workingFile.fileName());
            update_table_visualization();
        }else{
            output_to_console("File unable to initialize from Local");
            qDebug() << "File unable to initialize from Local";
        }
        QGuiApplication::restoreOverrideCursor();
    }
}

void MainWindow::on_delete_table_from_uxg_push_button_clicked(){
    //make sure the correct radio button is selected
    if(window_fpcs->settings.usingExistingTableLocal == false){

        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        //delete the fpcs file under that name
        QString command = "MEMory:DELete:NAME ";
        command.append('"');

        qDebug() << "Filename for delete is: " << window_fpcs->settings.currentTableSelectedThatIsOnTheUxg.remove(".csv",Qt::CaseInsensitive).remove(".fpcs",Qt::CaseInsensitive).append(".fpcs");
        output_to_console("Filename for delete is: " + window_fpcs->settings.currentTableSelectedThatIsOnTheUxg.remove(".csv",Qt::CaseInsensitive).remove(".fpcs",Qt::CaseInsensitive).append(".fpcs"));

        command.append(window_fpcs->settings.currentTableSelectedThatIsOnTheUxg).remove(".csv",Qt::CaseInsensitive).remove(".fpcs",Qt::CaseInsensitive).append(".fpcs"); //window_fpcs->settings.existingTableFilePath.append(".fpcs")
        command.append('"');
        window_ftpManager->send_SCPI(command);
        //reset the list of available files on the UXG
        on_select_file_from_uxg_radio_button_clicked();

        QGuiApplication::restoreOverrideCursor();


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
        if(window_fpcs->settings.usingExistingTableLocal){
            on_select_local_file_radio_button_clicked();
        }else{
            on_select_file_from_uxg_radio_button_clicked();
        }
    }
}

void MainWindow::on_binary_data_view_push_button_stateChanged(int arg1)
{
    window_fpcs->settings.preferredFormat = arg1;
    update_table_visualization();
}

void MainWindow::on_delete_selected_row_push_button_clicked()
{
    //Delete the Entry in the entryList that has the index of the index of the highlighted row
    window_fpcs->workingEntryList.removeAt(highlightedFpcsRow);
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

    if(window_fpcs->settings.preferredFormat == 0){
        //the box is unchecked so display in plaintext format
        rowIndex = 0;
        for(Entry *entry: window_fpcs->workingEntryList){
            //qDebug() << "plainText : " << entry.plainTextRepresentation;
            ui->table_visualization_table_widget->item(rowIndex,0)->setText(entry->plainTextRepresentation);
            rowIndex++;
        }
    }else{
        //the box is checked so display in binary format
        rowIndex = 0;
        for(Entry *entry: window_fpcs->workingEntryList){
            output_to_console("plainText : " + entry->plainTextRepresentation);
            qDebug() << "plainText : " << entry->plainTextRepresentation;
            ui->table_visualization_table_widget->item(rowIndex,0)->setText(entry->bitPattern);
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
    if(window_fpcs->workingEntryList.size() - 1 < highlightedFpcsRow){
        output_to_console("Please select a valid pattern from the table.");
        qDebug() << "Please select a valid pattern from the table.";
    }
    Entry *newEntry = new Entry(this);
    //copy over basic settings of the previous entry to be the same as this entry so the GUI doesn't need to be updated
    newEntry->codingType = window_fpcs->workingEntryList.at(highlightedFpcsRow)->codingType;
    newEntry->bitsPerSubpulse = window_fpcs->workingEntryList.at(highlightedFpcsRow)->bitsPerSubpulse;
    newEntry->phases = window_fpcs->workingEntryList.at(highlightedFpcsRow)->phases;
    newEntry->freqs = window_fpcs->workingEntryList.at(highlightedFpcsRow)->freqs;
    newEntry->freqUnits = window_fpcs->workingEntryList.at(highlightedFpcsRow)->freqUnits;
    newEntry->bitPattern = window_fpcs->workingEntryList.at(highlightedFpcsRow)->bitPattern;
    newEntry->numOfPhasesOrFreqs = window_fpcs->workingEntryList.at(highlightedFpcsRow)->numOfPhasesOrFreqs;
    newEntry->plainTextRepresentation = window_fpcs->workingEntryList.at(highlightedFpcsRow)->plainTextRepresentation;

    //set that new entry to be the workingEntry
    window_fpcs->workingEntry = newEntry;
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
    ui->how_many_different_phase_or_freq_spin_box->setValue(window_fpcs->workingEntry->numOfPhasesOrFreqs);

    if(window_fpcs->workingEntry->codingType == "BOTH"){
        ui->both_pattern_type_radio_button->setChecked(true);
        ui->both_pattern_type_radio_button->click();
        ui->freq_pattern_type_radio_button->setChecked(false);
        ui->phase_pattern_type_radio_button->setChecked(false);
    }else if(window_fpcs->workingEntry->codingType == "FREQUENCY"){
        ui->both_pattern_type_radio_button->setChecked(false);
        ui->freq_pattern_type_radio_button->setChecked(true);
        ui->freq_pattern_type_radio_button->click();
        ui->phase_pattern_type_radio_button->setChecked(false);
    }else if(window_fpcs->workingEntry->codingType == "PHASE"){
        ui->both_pattern_type_radio_button->setChecked(false);
        ui->freq_pattern_type_radio_button->setChecked(false);
        ui->phase_pattern_type_radio_button->setChecked(true);
        ui->phase_pattern_type_radio_button->click();
    }else{
        output_to_console("Error, unknown codingType for working Entry");
        qDebug() << "Error, unknown codingType for working Entry";
    }

    ui->pattern_nonBinary_values_shown_text_editor->setText(window_fpcs->workingEntry->plainTextRepresentation);
    ui->pattern_binary_pattern_shown_text_editor->setText(window_fpcs->workingEntry->bitPattern);

    //enter in the phase values
    int count = 0;
    for(QString phase:window_fpcs->workingEntry->phases){
        ui->phase_freq_pattern_entry_table->item(count, 0)->setText(phase + "°");
        count++;
    }

    //enter in the freq values
    count = 0;
    for(QString freq:window_fpcs->workingEntry->freqs){
        ui->phase_freq_pattern_entry_table->item(count, 1)->setText(freq);
        count++;
    }

    //enter in the freq values
    count = 0;
    for(QString freqUnits:window_fpcs->workingEntry->freqUnits){
        ui->phase_freq_pattern_entry_table->item(count, 2)->setText(freqUnits + "hz");
        count++;
    }
}

void MainWindow::on_host_name_edit_line_textChanged(const QString &arg1)
{
    window_ftpManager->hostName = arg1;
}

void MainWindow::on_comboBox_activated(const QString &arg1)
{
    if(arg1 == "2A"){
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
    }else if(arg1 == "2B"){
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
    }else if(arg1 == "3"){
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
    }else if(arg1 == "4A"){
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
    }else if(arg1 == "4B"){
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
    }else if(arg1 == "5"){
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
    }else if(arg1 == "7"){
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
    }else if(arg1 == "11"){
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
    }else if(arg1 == "13"){
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
        on_phase_freq_pattern_entry_table_cellClicked(0, 3);
        on_phase_freq_pattern_entry_table_cellClicked(1, 3);
    }else{
        output_to_console("Error, unknown Barker Code selected");
        qDebug() << "Error, unknown Barker Code selected";
    }
}

void MainWindow::on_select_yatg_file_push_button_clicked()
{
    //pdwLineEdit

    window_yatg->uploadingMultipleFiles = false;

    //prompt the user to select a file from the local system
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "/fileFolder/uploads", tr("CSV Files (*.csv);;Text Files (*.txt)"));

    //check if the user selected "cancel"
    if(filePath.size() == 0){
        output_to_console("cancelled selecting file");
        return;
    }

    window_yatg->workingFilePath = filePath; //set the filepath

    //get the file name without the directory by sepperating the string into a list of strings sepperated by "/" then using the last element in the list
    QStringList list = filePath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    ui->pdwNameLineEdit->setText(list.last());

    ui->selected_yatg_file_line_editor->setText(window_yatg->workingFilePath); //update the EditLine Box so the user can see the filePath they just chose
    ui->select_multiple_files_by_folder_line_editor->clear();
}

void MainWindow::on_select_multiple_files_by_folder_push_button_clicked()
{
    window_yatg->uploadingMultipleFiles = true;

    //prompt the user to select a folder
    QString folderName = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                           "/fileFolder/uploads", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    //check if the user selected "cancel"
    if(folderName.size() == 0){
        output_to_console("cancelled selecting folder");
        return;
    }

    window_yatg->workingFilePath = folderName; //set the filepath

    ui->select_multiple_files_by_folder_line_editor->setText(window_yatg->workingFilePath);
    ui->selected_yatg_file_line_editor->clear();

}

void MainWindow::on_upload_yatg_file_to_uxg_push_button_clicked()
{
    timer.start();

    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    bool success = true;

    if(window_yatg->uploadingMultipleFiles){
        output_to_console("Uploading multiple files to UXG...");
        success = window_yatg->upload_multiple_files_to_uxg();
    }else{
        output_to_console("Uploading file to UXG...");
        success = window_yatg->upload_file_to_uxg();
    }
    if(!success){
        output_to_console("File unable to upload to UXG : ERROR");
        qDebug() << "File unable to upload to UXG : ERROR";
        return;
    }
    /*
     * If you send 5 SCPI's in a row, then send an *OPC? you will know when it's done with those 5 scpi commands because once it gets to the *OPC? it
     * immediately responds with "1". The uxg's scpi queue is FIFO so in the ready Read we know that if it sent back "1", it is finished uploading the
     * pdw data we sent over SCPI.
     */

    //this breakpoint marks when our program is done creating the csv file, uploading it to the UXG using FTP, and telling the UXg to convert it into pdw binary
    output_to_console("time is took to upload the file : " + QString::number(timer.elapsed()) + " milliseconds");
    timer.restart();

    window_ftpManager->waitingForPdwUpload = true;
    output_to_console("Waiting for UXG...");
    window_ftpManager->send_SCPI("*OPC?");
}

void MainWindow::on_playPDWPushButton_clicked()
{
    QString namer = ui->pdwNameLineEdit->text();
    output_to_console("pdwNameLineEdit : " + namer);
    QString filenamer = "'" + namer.remove(".csv",Qt::CaseInsensitive).remove(".txt",Qt::CaseInsensitive) + "'";
    output_to_console("filename : " + filenamer);
    window_ftpManager->playPDW(filenamer, ui->continuousPDWCheckBox->isChecked());
    output_to_console("playing PDW");
}

void MainWindow::on_stopPDWPushButton_clicked()
{
    output_to_console("Stopping PDW");
    if(multiplePDWsPlaying){
        window_ftpManager->send_SCPI(":STReam:STATe OFF");
        window_ftpManager->send_SCPI(":STReam:STATe ON");
    }else{
        window_ftpManager->send_SCPI(":STReam:STATe OFF");
        window_ftpManager->send_SCPI(":OUTPut OFF");
        window_ftpManager->send_SCPI(":OUTPut:MODulation OFF");
    }

}

/*when the elevation push button is clicked, it takes the information from the
 * elevation line edit and sends it to the move rotor command in the serial class
 **/
void MainWindow::on_elevationPushButton_clicked()
{
    QString el_value= ui->elevationLineEdit->text();
    bool isPosition = ui->positionRadioButton->isChecked();
    serial->moveRotor(el_value, true, isPosition);//the true is for the isElevation boolean
}

/*same as the elevation slot above but for azimuth
 * */
void MainWindow::on_azimuthPushButton_clicked()
{
    QString az_value= ui->azimuthLineEdit->text();
    bool isPosition = ui->positionRadioButton->isChecked();
    serial->moveRotor(az_value, false, isPosition);//the false is for the isElevation boolean (it's not elevation, its azimuth)
}

//if both line edits have a value then they will both change
void MainWindow::on_changeBothPushButton_clicked()
{
    QString az_value= ui->azimuthLineEdit->text();
    QString el_value= ui->elevationLineEdit->text();
    bool isPosition = ui->positionRadioButton->isChecked();
    serial->moveRotor(el_value, true, isPosition);
    serial->moveRotor(az_value, false, isPosition);
}

//reconnects the usb ports, if the gui was opened before the USBs were plugged in
void MainWindow::on_connectUSBPushButton_clicked()
{
    ui->elevationMotorSpeedRadioButton->setChecked(true);
    ui->azimuthMotorSpeedRadioButton->setChecked(false);
    serial->closeSerialPorts();
    serial->findSerialPorts();
}

//when the max speed slider is released, the value of the slider is sent to the
//changeMotorSpeed function in the RotorControl class
void MainWindow::on_maxSpeedSlider_sliderReleased()
{
    bool isElevation = ui->elevationMotorSpeedRadioButton->isChecked();
    serial->changeMotorSpeed(ui->maxSpeedSlider->value(), RotorControl::controllerParameter::maxSpeed, isElevation);
}

//when the min speed slider is released, this does the same thing as the slot above
void MainWindow::on_minSpeedSlider_sliderReleased()
{
    bool isElevation = ui->elevationMotorSpeedRadioButton->isChecked();
    serial->changeMotorSpeed(ui->minSpeedSlider->value(), RotorControl::controllerParameter::minSpeed, isElevation);
}

//when the ramp slider is released, this does the same thing as the max slider slot
void MainWindow::on_rampSlider_sliderReleased()
{
    bool isElevation = ui->elevationMotorSpeedRadioButton->isChecked();
    serial->changeMotorSpeed(ui->rampSlider->value(), RotorControl::controllerParameter::ramp, isElevation);
}

//updates the positions on the LCDs
void MainWindow::updatePositions(){
    serial->getPosition();
    bool isHeadingEqualtoPosition =false;
    if(!udpSocket->isBound){

        QList<QString> *list = udpSocket->getIPAddressAndPort();
        if(!list->at(0).isEmpty()){
            ui->console_text_editor->appendPlainText("IP Address: " + list->at(0)+ ", Port: " + list->at(1));
        }
    }
    if(!serial->stoppingMotion){
        isHeadingEqualtoPosition = serial->headingEqualsPosition();
    }
    if(multiplePDWsPlaying){
        window_ftpManager->send_SCPI(":SOURce:STReam:INFormation:SSTate?");
    }

    if((serial->advancedTestStarted||serial->simpleTestStarted)){
        if(triggerSent){
            window_ftpManager->send_SCPI(":SOURce:STReam:INFormation:SSTate?");
        }else{

            if(!positionTestHasBeenOpened&&serial->azSpeedsCorrectForTest&&serial->elSpeedsCorrectForTest){
                if(serial->advancedTestStarted){
                    startPositionTest();
                }else if(serial->simpleTestStarted){
                    startSimpleTest();
                }
                positionTestHasBeenOpened=true;
            }
            if(isHeadingEqualtoPosition){
                if(rotorInPositionCounter==5){
                    serial->stopMotion();
                    QString fileName = "";
                    if(ui->singlePDWCheckBox->isChecked()){
                        fileName = ui->singlePDWFileLineEdit->text().remove(".csv",Qt::CaseInsensitive).remove(".txt",Qt::CaseInsensitive);
                        fileName = "'" + fileName + "'";
                    }else{
                        fileName = "'" + QString::number(fileNumber) + "'";
                        fileNumber++;
                    }
                    window_ftpManager->playPDW(fileName, ui->continuousTriggerCheckBox->isChecked());

                    triggerSent=true;
                }
                rotorInPositionCounter++;
            }else{
                rotorInPositionCounter=0;
            }
        }
    }
}

/*called when the ready Read signal goes off.  reads the data and then takes the will update
 * the gui depending on what data came in.
 * */
void MainWindow::serialRead(){
    QObject *serialPort = sender();
    bool isElevation;
    if(serialPort==serial->el){
        isElevation=true;
    }else{
        isElevation=false;
    }

    RotorControl::ParameterAndWriteNumber read = serial->serialRead(isElevation);
    switch (read.parameter){
    case(RotorControl::controllerParameter::maxSpeed) :{
        if(serial->simpleTestStarted||serial->advancedTestStarted){
            ui->maxSpeedSlider->setSliderPosition(4);
            serial->write("WF22;",isElevation);
        }else if(serial->droneTestStarted&&read.writeNumber==-1){
            ui->maxSpeedSlider->setSliderPosition(10);
            serial->write("WF22;",isElevation);
        }else if(read.writeNumber!=-1){
            ui->maxSpeedSlider->setSliderPosition(read.writeNumber);
        }
        break;
    }
    case(RotorControl::controllerParameter::minSpeed) :{
        if(serial->simpleTestStarted||serial->advancedTestStarted){
            ui->minSpeedSlider->setSliderPosition(2);
            serial->write("WN02;",isElevation);
        }else if(serial->droneTestStarted&&read.writeNumber==-1){
            ui->minSpeedSlider->setSliderPosition(2);
            serial->write("WN00;",isElevation);
        }else if(read.writeNumber!=-1){
            ui->minSpeedSlider->setSliderPosition(read.writeNumber);
        }
        break;
    }
    case(RotorControl::controllerParameter::ramp) :{
        if(serial->simpleTestStarted||serial->advancedTestStarted){
            ui->rampSlider->setSliderPosition(2);
            if(isElevation){
                serial->elSpeedsCorrectForTest=true;
            }else{
                serial->azSpeedsCorrectForTest=true;
            }

            mainTimer->start(200);
        }else if(serial->droneTestStarted){
            ui->rampSlider->setSliderPosition(0);
        }
        else if(read.writeNumber!=-1){
            ui->rampSlider->setSliderPosition(read.writeNumber);
        }
        break;
    }
    case(RotorControl::controllerParameter::position) :{
        if(isElevation){
            ui->elevationLCD->display(read.writeNumber);
        }else{
            ui->azimuthLCD->display(read.writeNumber);
        }

        break;
    }
    case(RotorControl::controllerParameter::elevationMode) :{
        output_to_console("Correct Elevation Mode Set");
        break;
    }
    case(RotorControl::controllerParameter::pulseDivider) :{
        serial->write("WI00;",isElevation);
        break;
    }
    case(RotorControl::controllerParameter::CWLimit) :{
        serial->write("WH00;",isElevation);
        break;
    }
    case(RotorControl::controllerParameter::CCWLimit) :{
        if(isElevation){
            ui->console_text_editor->appendPlainText("Finished Fixing Elevation");
        }else{
            ui->console_text_editor->appendPlainText("Finished Fixing Azimuth");
        }
        break;
    }
    case(RotorControl::controllerParameter::notReady) :{
        break;
    }
    }
}

//stops the motion of both rotors
void MainWindow::on_stopButton_clicked()
{
    serial->stoppingMotion=true;
    stopMotionTimer->start(1500);
    serial->stopMotion();
}

//moves the max, min, and ramp sliders on the gui depending on if the elevation motor speed radio button is checked
void MainWindow::on_elevationMotorSpeedRadioButton_toggled(bool checked)
{
    serial->getMaxMinAndRampValues(checked);
}

void MainWindow::on_positionRadioButton_toggled(bool)
{
    ui->elevationLineEdit->clear();
    ui->azimuthLineEdit->clear();
}

void MainWindow::on_fixElevationPushButton_clicked()
{
    serial->write("WK01080;",true);
}

void MainWindow::on_fixAzimuthPushButton_clicked()
{
    serial->write("WK01080;",false);
}

void MainWindow::resendTimerTimeout(){
    QObject *timer = sender();
    if(timer == serial->elResendDataTimer){
        serial->timerTimeout(true);
    }else if(timer == serial->azResendDataTimer){
        serial->timerTimeout(false);
    }
}

//immediately stops all rotor motion
void MainWindow::stopMotion(){
    serial->stoppingMotion = false;
    stopMotionTimer->stop();
    serial->setHeadingsToCurrentPosition();
}

//test functions

//stuff for test, remove later
void MainWindow::on_startTestButton_clicked()
{
    if(ui->recommendedSpeedRadioButton->isChecked()){
        mainTimer->stop();
        serial->simpleTestStarted=true;
        serial->write("WG24;",true);
        serial->write("WG24;",false);
    }else{
        serial->simpleTestStarted = true;
        startSimpleTest();
    }
}

void MainWindow::startSimpleTest(){
    serial->advancedTestStarted=false;
    QList<QString> azimuthValues = ui->MaxMinAzimuthLineEdit->text().split(',');
    azimuthValues.replace(0,QString::number(azimuthValues.at(0).toInt()+180));
    azimuthValues.replace(1,QString::number(azimuthValues.at(1).toInt()+180));
    QList<QString> elevationValues = ui->MaxMinElevationLineEdit->text().split(',');
    elevationValues.replace(0,QString::number(elevationValues.at(0).toInt()+180));
    elevationValues.replace(1,QString::number(elevationValues.at(1).toInt()+180));
    QList<QString> resolution =ui->ResolutionLineEdit->text().split(',');

    serial->startSimpleTest(azimuthValues.at(0).toDouble(),azimuthValues.at(1).toDouble(),elevationValues.at(0).toDouble()
                            ,elevationValues.at(1).toDouble(),resolution.at(0).toDouble(),resolution.at(1).toDouble());
}

QString MainWindow::getFileName(bool isTestOptionsLineEdit){
    QString fileName;
    if(isTestOptionsLineEdit){
        fileName = ui->openTestLineEdit->text();
    }else{
        fileName=ui->testFileLineEdit->text();
    }

    QString file;
    if(fileName.startsWith("C:")){
        file = fileName;
    }else{
        QString filePath = QCoreApplication::applicationDirPath();

        file=filePath + "/"+ fileName;
    }
    return file;
}

/*when the open test pushbutton is clicked, stream the file specified in the line edit
 * and turn the file into either a simple test, or advanced test.  The simple test just records the
 * max and min azimuth, max and min elevation, and step values to create a simple grid test.
 * The advanced test takes in a List of positions and moves through them
 * */
void MainWindow::on_OpenTestPushButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "/fileFolder/downloads", tr("Text Files (*.txt)"));
    if(filePath.size() == 0){
        output_to_console("cancelled selecting folder");
        return;
    }
    ui->openTestLineEdit->setText(filePath);
}

void MainWindow::startPositionTest(){
    QFile file(getFileName(true));

    if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QTextStream stream(&file);
        QString line;
        line = stream.readLine();
        if(line == "simple"){
            QList<QString> azimuthValues;
            QList<QString> elevationValues;
            QList<QString> stepValues;

            do{
                line = stream.readLine();
                if(line.startsWith("az:")){
                    QString cutLine=line.mid(3,line.length()-1);
                    azimuthValues = cutLine.split(',');
                    azimuthValues.replace(0,QString::number(azimuthValues.at(0).toInt()+180));
                    azimuthValues.replace(1,QString::number(azimuthValues.at(1).toInt()+180));
                }else if(line.startsWith("el:")){
                    QString cutLine=line.mid(3,line.length()-1);
                    elevationValues = cutLine.split(',');
                    elevationValues.replace(0,QString::number(elevationValues.at(0).toInt()+180));
                    elevationValues.replace(1,QString::number(elevationValues.at(1).toInt()+180));
                }else if(line.startsWith("step:")){
                    QString cutLine=line.mid(5,line.length()-1);
                    stepValues = cutLine.split(',');
                }
            }while(!line.isNull());
            //simpleTestStarted = true;
            serial->advancedTestStarted=false;
            serial->simpleTestStarted=true;
            serial->startSimpleTest(azimuthValues.at(0).toDouble(),azimuthValues.at(1).toDouble(),elevationValues.at(0).toDouble()
                                    ,elevationValues.at(1).toDouble(),stepValues.at(0).toDouble(),stepValues.at(1).toDouble());
        }else if(line=="advanced"){
            line = stream.readLine();
            QString stepValues=line.mid(5,line.length()-1);
            ui->tableResolutionLineEdit->setText(stepValues);
            on_openTestFilePushButton_clicked();
            do{
                line=stream.readLine();
                if(line!=""){
                    QList<QString> positions = line.split(',');
                    int az = positions.at(0).toInt()+180;
                    int el = positions.at(1).toInt()+180;
                    QString fixedPositions = QString::number(az)+ "," + QString::number(el);
                    testPositions.push_back(fixedPositions);
                }
            }while(!line.isNull());
            //advancedTestStarted = true;
            serial->simpleTestStarted=false;
            serial->advancedTestStarted=true;
            serial->startAdvancedTest(testPositions);
            testPositions.clear();
        }
        file.close();
    }else{
        QString error = file.errorString();
        output_to_console(error);
        qDebug() << error;
    }
}

void MainWindow::on_openTestFilePushButton_clicked()
{
    QFile file(getFileName(false));

    if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QTextStream stream(&file);
        QString line;
        line = stream.readLine();
        if(line == "advanced"){
            line = stream.readLine();
            if(line.startsWith("step:")){
                QString stepValues=line.mid(5,line.length()-1);
                ui->tableResolutionLineEdit->setText(stepValues);
            }else{
                qDebug() << "unknown resolution";
                file.close();
                return;
            }
            on_clearTablePushButton_clicked();//clear table
            ui->testCreatorTableWidget->blockSignals(true);
            do{
                line = stream.readLine();
                if(line!=""){
                    QList<QString> list = line.split(',');
                    int row = tableCreatorRowOffset-((list.at(1).toInt()+90)/tableElResolution)-1;
                    int column = (list.at(0).toInt()+90)/tableAzResolution;
                    if(list.at(0).toInt()!=ui->testCreatorTableWidget->horizontalHeaderItem(column)->text().toInt()){
                        column--;
                    }else if(list.at(1).toInt()!=ui->testCreatorTableWidget->verticalHeaderItem(row)->text().toInt()){
                        row--;
                    }
                    ui->testCreatorTableWidget->item(row,column)->setBackground(QColor(243,112,33));
                    ui->testCreatorTableWidget->item(row,column)->setText(QString::number(currentTableNumber));
                    tableWidgetItemList.append(ui->testCreatorTableWidget->item(row,column));
                    currentTableNumber++;
                }
            }while(!line.isNull());
            ui->testCreatorTableWidget->scrollToItem(tableWidgetItemList.at(0));
            ui->testCreatorTableWidget->blockSignals(false);
        }else{
            qDebug() << "can't parse text file that doesn't start with ""advanced""";
        }
    }else{
        qDebug() << "could not find file";
    }
    file.close();
}

void MainWindow::on_createTextFilePushButton_clicked()
{
    QFile file(getFileName(false));
    ui->openTestLineEdit->setText(ui->testFileLineEdit->text());
    if(file.open(QIODevice::WriteOnly)){
        QTextStream stream(&file);
        stream <<"advanced\r\n";
        stream << "step:"<<QString::number(tableAzResolution)<<","<<QString::number(tableElResolution)<<"\r\n";
        for(int i=0; i< tableWidgetItemList.length();i++){

            QString elPos = QString::number(ui->testCreatorTableWidget->model()->headerData(tableWidgetItemList.at(i)->row(),Qt::Vertical).toInt());
            QString azPos = QString::number(ui->testCreatorTableWidget->model()->headerData(tableWidgetItemList.at(i)->column(),Qt::Horizontal).toInt());

            stream << azPos << "," << elPos << "\r\n";
        }
        output_to_console("Test File Created");
    }else{
        output_to_console("Could not open");
        qDebug() << "Could not open";
    }

    file.close();
}

//sets the ramp value on az and el to 1 so the rotor can quickly speeds up
//and tells the program to start looking for az,az.speed,el,el.speed commands over UDP
void MainWindow::on_startDroneTestPushButton_clicked()
{
    serial->droneTestStarted = true;
    QString comboBoxText = ui->refreshRateComboBox->currentText();
    comboBoxText.chop(3);
    serial->refreshRate = comboBoxText.toDouble();
    if(ui->recommendedSpeedRadioButton->isChecked()){
        serial->write("WG210;",true);
        serial->write("WG210;",false);
    }
}

//stops any test the is currently running
void MainWindow::on_stopTestPushButton_clicked()
{
    output_to_console("Stopping All Processes");
    stopMotion();
    multiplePDWsPlaying=false;
    serial->simpleTestStarted = false;
    serial->advancedTestStarted = false;
    serial->droneTestStarted = false;
    triggerSent=false;
    positionTestHasBeenOpened=false;
    serial->elSpeedsCorrectForTest=false;
    serial->azSpeedsCorrectForTest=false;
    rotorInPositionCounter=0;
    serial->azimuthGoingUp=true;
    fileNumber=1;
    if(window_ftpManager->tcpSocket->state()== QAbstractSocket::ConnectedState){
        window_ftpManager->send_SCPI(":OUTPut OFF");
        window_ftpManager->send_SCPI(":OUTPut:MODulation OFF");
        window_ftpManager->send_SCPI(":STReam:STATe OFF");
    }
    serial->lastDroneAzPosition=0;
    serial->lastDroneElPosition=0;
}

//table functions

void MainWindow::on_changeTableResolutionPushButton_clicked()
{
    QList<QString> resolution = ui->tableResolutionLineEdit->text().split(',');
    resetTable(resolution.at(0).toInt(), resolution.at(1).toInt());
}

//when the select box or erase box radio button is toggled, clear the blue selection.
//Makes everything look nicer and easier to understand
void MainWindow::on_selectBoxesRadioButton_toggled(bool)
{
    ui->testCreatorTableWidget->clearSelection();
}

void MainWindow::resetTable(int azResolution, int elResolution){
    ui->testCreatorTableWidget->blockSignals(true);
    this->tableAzResolution = azResolution;
    this->tableElResolution = elResolution;
    int numberOfRows = 180/elResolution+1;
    int numberOfColumns = 180/azResolution+1;
    ui->testCreatorTableWidget->setColumnCount(numberOfColumns);
    ui->testCreatorTableWidget->setRowCount(numberOfRows);
    tableCreatorRowOffset=numberOfRows;
    int originx = 90/azResolution;
    int originy = 90/elResolution;
    int azHeader = 0;
    int elHeader = 0;
    for(int i=originx; i>=0;i--){
        ui->testCreatorTableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem);
        ui->testCreatorTableWidget->model()->setHeaderData(i,Qt::Horizontal,azHeader);
        azHeader-=azResolution;
        for(int j=0; j<ui->testCreatorTableWidget->columnCount(); j++){
            ui->testCreatorTableWidget->setItem(i,j, new QTableWidgetItem);
        }
    }
    azHeader = 0;
    for(int i=originx; i<ui->testCreatorTableWidget->columnCount();i++){
        ui->testCreatorTableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem);
        ui->testCreatorTableWidget->model()->setHeaderData(i,Qt::Horizontal,azHeader);
        azHeader+=azResolution;
        for(int j=0; j<ui->testCreatorTableWidget->columnCount(); j++){
            ui->testCreatorTableWidget->setItem(i,j, new QTableWidgetItem);
        }
    }
    for(int i=originy; i>=0;i--){
        ui->testCreatorTableWidget->setVerticalHeaderItem(i,new QTableWidgetItem);
        ui->testCreatorTableWidget->model()->setHeaderData(i,Qt::Vertical,elHeader);
        elHeader+=elResolution;
        for(int j=0; j<ui->testCreatorTableWidget->rowCount(); j++){
            ui->testCreatorTableWidget->setItem(i,j, new QTableWidgetItem);
        }
    }
    elHeader=0;
    for(int i=originy; i<ui->testCreatorTableWidget->rowCount();i++){
        ui->testCreatorTableWidget->setVerticalHeaderItem(i,new QTableWidgetItem);
        ui->testCreatorTableWidget->model()->setHeaderData(i,Qt::Vertical,elHeader);
        elHeader-=elResolution;
        for(int j=0; j<ui->testCreatorTableWidget->rowCount(); j++){
            ui->testCreatorTableWidget->setItem(i,j, new QTableWidgetItem);
        }
    }
    tableWidgetItemList.clear();
    currentTableNumber=0;
    QTableWidgetItem *origin = ui->testCreatorTableWidget->item(90/elResolution,90/azResolution);
    ui->testCreatorTableWidget->scrollToItem(origin);
    //set the origin to black to make it easy to see
    origin->setBackground(Qt::black);
    ui->testCreatorTableWidget->blockSignals(false);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event){
    if(watched == ui->testCreatorTableWidget->viewport()){
        ui->testCreatorTableWidget->blockSignals(true);
        if(event->type()== QMouseEvent::MouseButtonRelease){
            QMouseEvent *newEvent = static_cast<QMouseEvent*> (event);
            if(newEvent->button()==Qt::LeftButton){
                return leftMouseButtonReleaseEvent(newEvent);
            }else{
                rightMouseButtonPressed=false;
            }
        }else if(event->type()==QMouseEvent::MouseButtonPress){
            QMouseEvent *newEvent = static_cast<QMouseEvent*> (event);
            if(newEvent->button() == Qt::RightButton){
                rightMouseButtonPressed=true;
                int row = ui->testCreatorTableWidget->itemAt(newEvent->x(),newEvent->y())->row();
                int column = ui->testCreatorTableWidget->itemAt(newEvent->x(),newEvent->y())->column();
                on_testCreatorTableWidget_cellEntered(row, column);
            }
        }
        ui->testCreatorTableWidget->blockSignals(false);
    }
    return false;
}

bool MainWindow::leftMouseButtonReleaseEvent(QMouseEvent*){
    if(ui->selectBoxesRadioButton->isChecked()){
        QList<QTableWidgetItem *> list = ui->testCreatorTableWidget->selectedItems();
        int columnCount = list.last()->column()-list.first()->column();
        int rowCount = list.last()->row()-list.first()->row();
        bool rowChange = false;
        for(int i=list.first()->row()+rowCount; i>= list.first()->row();i--){
            int columnOffset = columnCount;
            for(int j=list.first()->column(); j<= list.first()->column()+columnCount;j++){
                if(!rowChange){
                    if(ui->testCreatorTableWidget->item(i,j)->background().color()!=QColor(243,112,33)){
                        ui->testCreatorTableWidget->item(i,j)->setBackground(QColor(243,112,33));
                        ui->testCreatorTableWidget->item(i,j)->setText(QString::number(currentTableNumber));
                        tableWidgetItemList.append(ui->testCreatorTableWidget->item(i,j));
                        currentTableNumber++;
                    }
                }else{
                    int changedColumn = j+columnOffset;
                    if(ui->testCreatorTableWidget->item(i,changedColumn)->background().color()!=QColor(243,112,33)){
                        ui->testCreatorTableWidget->item(i,changedColumn)->setBackground(QColor(243,112,33));
                        ui->testCreatorTableWidget->item(i,changedColumn)->setText(QString::number(currentTableNumber));
                        tableWidgetItemList.append(ui->testCreatorTableWidget->item(i,changedColumn));
                        currentTableNumber++;
                    }
                    columnOffset-=2;
                }
            }
            rowChange = !rowChange;
        }
    }else if(ui->eraseBoxesRadioButton->isChecked()){
        QList<QTableWidgetItem *> list = ui->testCreatorTableWidget->selectedItems();
        int firstChangedIndex = list.last()->text().toInt();
        for(int i=0; i<list.length() ;i++){
            if(list.at(i)->background().color()==QColor(243,112,33)){
                tableWidgetItemList.removeOne(list.at(i));
                if(list.at(i)==ui->testCreatorTableWidget->item(90/tableElResolution,90/tableAzResolution)){
                    list.at(i)->setBackground(Qt::black);
                }else{
                    QBrush *brush = new QBrush();
                    list.at(i)->setBackground(*brush);
                }
                list.at(i)->setText("");
            }
            if(list.at(i)->text().toInt()<firstChangedIndex){
                firstChangedIndex = list.at(i)->text().toInt();
            }
        }
        for(int i=firstChangedIndex; i< tableWidgetItemList.length();i++){
            if(ui->testCreatorTableWidget->item(tableWidgetItemList.at(i)->row(),tableWidgetItemList.at(i)->column())->background().color()==QColor(243,112,33)){
                tableWidgetItemList.at(i)->setText(QString::number(i));
            }
        }
        currentTableNumber = tableWidgetItemList.length();
    }
    return false;
}

void MainWindow::on_clearTablePushButton_clicked()
{
    ui->testCreatorTableWidget->blockSignals(true);
    QList<QString> resolution = ui->tableResolutionLineEdit->text().split(',');
    resetTable(resolution.at(0).toInt(), resolution.at(1).toInt());
    ui->testCreatorTableWidget->clearSelection();
    tableWidgetItemList.clear();
    currentTableNumber=0;
    ui->testCreatorTableWidget->blockSignals(false);
}

void MainWindow::on_testCreatorTableWidget_cellEntered(int row, int column)
{
    ui->testCreatorTableWidget->blockSignals(true);
    if(rightMouseButtonPressed){
        if(ui->selectBoxesRadioButton->isChecked()){
            if(ui->testCreatorTableWidget->item(row,column)->background().color()!=QColor(243,112,33)){
                ui->testCreatorTableWidget->item(row,column)->setBackground(QColor(243,112,33));
                ui->testCreatorTableWidget->item(row,column)->setText(QString::number(currentTableNumber));
                tableWidgetItemList.append(ui->testCreatorTableWidget->item(row,column));
                currentTableNumber++;
            }
        }else if(ui->eraseBoxesRadioButton->isChecked()){
            if(ui->testCreatorTableWidget->item(row,column)->background().color()==QColor(243,112,33)){
                int removedIndex = ui->testCreatorTableWidget->item(row,column)->text().toInt();
                tableWidgetItemList.removeOne(ui->testCreatorTableWidget->item(row,column));
                QBrush *brush = new QBrush();
                ui->testCreatorTableWidget->item(row,column)->setBackground(*brush);
                ui->testCreatorTableWidget->item(row,column)->setText("");
                for(int i = removedIndex; i<tableWidgetItemList.length();i++){
                    tableWidgetItemList.at(i)->setText(QString::number(i));
                }
                currentTableNumber=tableWidgetItemList.length();
            }
        }
    }
    ui->testCreatorTableWidget->blockSignals(false);
}

void MainWindow::on_testCreatorTableWidget_itemChanged(QTableWidgetItem *item)
{
    if(!tableWidgetItemList.isEmpty()){
        int indexChanged = tableWidgetItemList.indexOf(item);
        int newIndex = tableWidgetItemList.at(indexChanged)->text().toInt();
        if(newIndex>=tableWidgetItemList.length()||newIndex<0){
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setAttribute(Qt::WA_DeleteOnClose);
            msgBox->setStandardButtons(QMessageBox::Ok);
            msgBox->setText("Invalid Number");
            msgBox->open( this, SLOT(msgBoxClosed(QAbstractButton*)));
            tableWidgetItemList.at(indexChanged)->setText(QString::number(indexChanged));
            return;
        }
        tableWidgetItemList.move(indexChanged,newIndex);
        ui->testCreatorTableWidget->blockSignals(true);
        for(int i=0; i <tableWidgetItemList.length();i++){
            tableWidgetItemList.at(i)->setText(QString::number(i));
        }
        ui->testCreatorTableWidget->blockSignals(false);
    }
}

//when new udp data is ready to read, this will read the data.  It also helps manage tests
//so if a certain string is taken in, like "done", it will tell the rotor to move to its next
//postion
void MainWindow::UdpRead(){
    QString readData = udpSocket->readData();
    ui->console_text_editor->appendPlainText(readData);
    if(serial->droneTestStarted){
        if(!(readData=="finished")){
            QList<QString> data = readData.split(',');
            serial->cantKeepUp = serial->manageDroneTest(data.at(0).toDouble(),data.at(1).toDouble()
                                                         ,serial->refreshRate);
            if(serial->cantKeepUp){
                QMessageBox *msgBox = new QMessageBox(this);
                msgBox->setAttribute(Qt::WA_DeleteOnClose);
                msgBox->setStandardButtons(QMessageBox::Ok);
                msgBox->setText("Can't Keep Up");
                msgBox->open( this, SLOT(msgBoxClosed(QAbstractButton*)));
            }
        }else{
            serial->droneTestStarted = false;
            ui->console_text_editor->appendPlainText("Drone Test Finished");
        }
    }
}

void MainWindow::on_turnStreamOffPushButton_clicked()
{
    window_ftpManager->send_SCPI(":STReam:STATe OFF");
    window_ftpManager->send_SCPI(":STReam:STATe ON");
}

void MainWindow::on_usingRotorCheckBox_toggled(bool checked)
{
    if(checked){
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        bool serialPortFound = serial->findSerialPorts();
        if(serialPortFound){
            ui->moveTypeGroupBox->setEnabled(true);
            ui->FixSerialGroupBox->setEnabled(true);
            ui->motorSpeedGroupBox->setEnabled(true);
            ui->moveMotorGroupBox->setEnabled(true);
            ui->azimuthLCD->setEnabled(true);
            ui->elevationLCD->setEnabled(true);
        }else{
            ui->usingRotorCheckBox->setChecked(false);
            output_to_console("No Serial Port Found");
        }
        QGuiApplication::restoreOverrideCursor();
    }else{

        ui->moveTypeGroupBox->setEnabled(false);
        ui->FixSerialGroupBox->setEnabled(false);
        ui->motorSpeedGroupBox->setEnabled(false);
        ui->moveMotorGroupBox->setEnabled(false);
        ui->azimuthLCD->setEnabled(false);
        ui->elevationLCD->setEnabled(false);
        ui->rampSlider->setSliderPosition(0);
        ui->maxSpeedSlider->setSliderPosition(1);
        ui->minSpeedSlider->setSliderPosition(1);
        ui->azimuthLCD->display(0);
        ui->elevationLCD->display(0);
        ui->elevationMotorSpeedRadioButton->setChecked(true);
        ui->azimuthMotorSpeedRadioButton->setChecked(false);
        serial->closeSerialPorts();
    }
}

void MainWindow::on_startPositionTestPushButton_clicked()
{
    serial->advancedTestStarted=true;
    if(ui->recommendedSpeedRadioButton->isChecked()){
        mainTimer->stop();
        serial->write("WG24;",true);
        serial->write("WG24;",false);
    }else{
        startPositionTest();
    }
}

void MainWindow::on_openFilePushButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "/fileFolder/downloads", tr("Text Files (*.txt)"));
    if(filePath.size() == 0){
        output_to_console("cancelled selecting folder");
        return;
    }
    ui->openTestLineEdit->setText(filePath);
}

/*
 * This method parses through a .txt or .csv file filled with names of PDW files to play.
 * Every new line in the file is assumed to be a new pdw file name to be played.
 */
void MainWindow::on_play_multiple_pdws_push_button_clicked()
{
    output_to_console("Please select file containing a list of names of pdw files to be played in order on the UXG.");
    //prompt the user to select a file from the local system
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "/fileFolder", tr("Text Files (*.txt);;CSV Files (*.csv)"));

    //check if the user selected "cancel"
    if(filePath.size() == 0){
        output_to_console("cancelled selecting file");
        return;
    }
    ui->play_multiple_pdws_line_edit->setText(filePath);
    output_to_console("File Chosen: " + filePath);

    QFile fileForBatchPlay;
    QTextStream streamerForBatchFile;

    fileForBatchPlay.setFileName(filePath); //set this file path + name to be the working file
    bool exists = fileForBatchPlay.exists(filePath);
    if(exists){
        if(!fileForBatchPlay.open(QFile::ReadWrite | QFile::Text | QFile::ExistingOnly)) //Options: ExistingOnly, NewOnly, Append
        {
            output_to_console(" Could not open Batchfile : " + filePath);
            qDebug() << " Could not open Batchfile : " + filePath;
            return;
        }else{
            output_to_console("File opened.");
        }
    }else{
        output_to_console("Batch File Not Found in Local System : " + filePath);
        qDebug() << "Batch File Not Found in Local System : " << filePath;
        return;
    }
    //create the QTextStreamer to operate on this file until further notice
    streamerForBatchFile.setDevice(&fileForBatchPlay);
    streamerForBatchFile.seek(0); //make sure the TextStream starts at the beginning of the file

    //loop through the plays
    QString line;
    do{
        line = streamerForBatchFile.readLine();
        if(line!=""){
            pdwFileNames.append(line);
        }
    }while(!line.isNull());
    fileForBatchPlay.close();
    if(pdwFileNames.isEmpty()){
        output_to_console("File was Empty");
    }else{
        multiplePDWsPlaying=true;
    }
    QString fileName = "'" + pdwFileNames.at(0) + "'";
    window_ftpManager->playPDW(fileName,ui->continuousPDWCheckBox->isChecked());
    output_to_console("playing PDW");
}

void MainWindow::on_create_yatg_template_file_pushbutton_clicked()
{
    //prompt the user to select a folder
    QString folderName = QFileDialog::getExistingDirectory(this, tr("Please Select a Directory to Save the File to."),
                                                           "/fileFolder", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    //check if the user selected "cancel"
    if(folderName.size() == 0){
        output_to_console("cancelled selecting folder");
        return;
    }
    QFile exampleFile;
    QTextStream streamerForExampleFile;

    folderName.append("/ExampleYATGFile.csv");

    exampleFile.setFileName(folderName); //set this file path + name to be the working file
    bool exists = exampleFile.exists(folderName);
    if(exists){
        output_to_console("The file titled 'ExampleYATGFile.csv' already exists in that directory. Please delete it and try again.");
        return;
    }else{
        if(!exampleFile.open(QFile::ReadWrite | QFile::Text | QFile::NewOnly)) //Options: ExistingOnly, NewOnly, Append
        {
            output_to_console("Unable to create the example file.");
        }else{
            output_to_console("Creating Example File...");
        }
    }

    //create the QTextStreamer to operate on this file until further notice
    streamerForExampleFile.setDevice(&exampleFile);
    streamerForExampleFile.seek(0); //make sure the TextStream starts at the beginning of the file

    //loop through the plays

    QString header = "FREQ (MHZ),PRI (s),PW(s),COUNT,ATTEN (DB),PHASE,MOP,CW,Chirp Shape,Chirp Rate (Hz/us)";
    header.append("\n");
    streamerForExampleFile << header;
    QString data = "10,1,1,1,-8,0,0,0,RAMP,0.00E+00";
    data.append("\n");
    streamerForExampleFile << data;
    streamerForExampleFile << data;
    QString comment = "#\n#The '#' symbol is used for comments and ignored by the W.H.O.P.P.E.R. Program.\n#\n";
    streamerForExampleFile << comment;
    comment = "#chirp rate:\n";
    streamerForExampleFile << comment;
    comment = "#Range: +/- 95.9765625 THz/us\n";
    streamerForExampleFile << comment;
    comment = "#Resolution: 21.827872842550278 Hz/us\n#\n";
    streamerForExampleFile << comment;
    comment = "#relative power:\n";
    streamerForExampleFile << comment;
    comment = "#Range: -161.6 dB to 31.07 dB\n";
    streamerForExampleFile << comment;
    comment = "#Resolution: 0.0085 dB\n#\n";
    streamerForExampleFile << comment;
    comment = "#phase:\n";
    streamerForExampleFile << comment;
    comment = "#Range: 0 deg – 360 deg\n";
    streamerForExampleFile << comment;
    comment = "#Resolution: 0.087890625 deg\n#\n";
    streamerForExampleFile << comment;
    comment = "#frequency:\n";
    streamerForExampleFile << comment;
    comment = "#Range: see model datasheet, package dependant\n";
    streamerForExampleFile << comment;
    comment = "#Resolution: 1/1024 Hz\n#\n";
    streamerForExampleFile << comment;
    comment = "#PRI:\n";
    streamerForExampleFile << comment;
    comment = "#Range: 0 ps to 213.504 days\n";
    streamerForExampleFile << comment;
    comment = "#Field Resolution: 1 ps\n#\n";
    streamerForExampleFile << comment;
    comment = "#PW:\n";
    streamerForExampleFile << comment;
    comment = "#Range: 4 ns to 4.294967295 s\n";
    streamerForExampleFile << comment;
    comment = "#Field Resolution: 1 ns\n";
    streamerForExampleFile << comment;
    comment = "#Resolution: 2 ns\n";
    streamerForExampleFile << comment;

    exampleFile.close();
    output_to_console("ExampleYATGFile.csv created successfully.");
}

void MainWindow::on_stopAllCurrentProcessesButton_clicked()
{
    on_stopTestPushButton_clicked();
    QGuiApplication::restoreOverrideCursor();
    QGuiApplication::restoreOverrideCursor();
    QGuiApplication::restoreOverrideCursor();
    QGuiApplication::restoreOverrideCursor();
    QGuiApplication::restoreOverrideCursor();
    QGuiApplication::restoreOverrideCursor();
}

void MainWindow::on_elevationModePushButton_clicked()
{
    ui->azimuthLCD->display(0);
    ui->elevationLCD->display(0);
    serial->setElevationMode(true);
    output_to_console("Set to Elevation Mode");
}

void MainWindow::on_notElevationModePushButton_clicked()
{
    ui->azimuthLCD->display(0);
    ui->elevationLCD->display(0);
    serial->setElevationMode(false);
    output_to_console("Set to Standard Mode");
}

void MainWindow::on_testOptionsHelpButton_clicked()
{
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setStandardButtons(QMessageBox::Ok);

    const QString fileName = QCoreApplication::applicationDirPath() + "/Capture";
    msgBox->setIconPixmap(QPixmap(fileName));
    msgBox->open( this, SLOT(msgBoxClosed(QAbstractButton*)));
}


void MainWindow::on_help_editing_pattern_push_button_clicked()
{
    //open a help pop-up
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setStandardButtons(QMessageBox::Ok);
    QString helpText = "How to Load a Frequency Phase Coding Setup Table for use-cases like Barker codes, Modulation On Pulse, etc. : \n\n   In this segment view, you are only editing one pattern (row) of the current table. You can cancel all the work on the current pattern by clicking the 'Cancel' button or press the 'Update/Add Pattern to Current Table' button when you are finished to add this pattern to the table. Note that the length of the pattern is how many subpulses the signal it is applied to will be broken up into. \n\n   In other words, if you have a 10ms PW, and add five entries from the table on the left the the pattern, your resulting signal will be split into 5 subpulses each 2ms long with the phase or frequency shift of each subpulse applied to their corresponding segments. \n\n   The number of Phases/Frequencies is the first value to choose since once the editing of the pattern begins, this value should not be changed. The number of different Frequencies/Phases only matters if you are concerned with the size of your pattern/table of patterns. See the Keysight documentation for the UXG to understand the binary limitations on the size of a .fpcs file. \n\n   Adding a Barker Code using the dropdown menu will apply that sequence using the first two entries in the table on the left as the template. The first entry at the top of the table serves as the '0' in the barker code. \n\n   The phases and frequencies for each entry can be changed as well as the units. See the Keysight documentation for the UXG to reference limitations of these values.";
    msgBox->setInformativeText(helpText);
    msgBox->open( this, SLOT(msgBoxClosed(QAbstractButton*)));
}
