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
    udpSocket->closeUdpSocket();
        serial->closeSerialPorts();
}

void MainWindow::setup(){
    udpSocket = new UdpSocket(this);
    serial = new RotorControl(this);//initialize the serial pointer
    serial->findSerialPorts();//connects the serial ports between the rotor and the
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
    QTableWidgetItem *originOffset = ui->testCreatorTableWidget->item(102,105);
    ui->testCreatorTableWidget->scrollToItem(originOffset);
    //set the origin to black to make it easy to see
    ui->testCreatorTableWidget->item(90,90)->setBackground(Qt::black);
}

void MainWindow::fpcs_setup(){
    //configured on setup
    window_fpcs = new Fpcs(this);
    ui->table_directory_line_edit->setText(window_fpcs->settings.defaultFilePath); //make the directory text line start out with default file path
    ui->default_name_line_edit->setText(window_fpcs->settings.defaultTableName);
}

void MainWindow::on_specify_table_name_radio_button_clicked()
{
    window_fpcs->settings.usingCustomTableName = true;
    //set the progress bar to 0%
    ui->create_progress_bar->reset();
    ui->loaded_table_progress_bar->reset();
}

void MainWindow::on_specified_table_name_line_edit_textChanged(const QString &arg1)
{
    if(window_fpcs->settings.usingCustomTableName == true){
        //if they checked the radio button for using a custom table name, let's go ahead and change our Table name to whatever they typed in
        window_fpcs->settings.customTableName = arg1;
    }
}

/*
 * When the "use default name" button is pressed, our settings for which name to use is updated, and the default name is shown as well as the default
 * filePath
 */
void MainWindow::on_use_default_name_radio_button_clicked()
{
    window_fpcs->settings.usingCustomTableName = false; //when this is false, we use the defaultTableName and defaultFilePath

    ui->default_name_line_edit->setText(window_fpcs->settings.defaultTableName);

    //set the progress bar to 0%
    ui->create_progress_bar->reset();
    ui->loaded_table_progress_bar->reset();
}

void MainWindow::on_change_table_directory_push_button_clicked()
{
    //prompt the user to select a folder
    QString folderName = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    window_fpcs->settings.customFilePath = folderName;

    window_fpcs->settings.usingCustomFilePath = true;

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
        window_fpcs->settings.usingCustomFilePath = true;

    }else{ //checked
        window_fpcs->settings.usingCustomFilePath = false; //we are using the default file path
        ui->table_directory_line_edit->setText(window_fpcs->settings.defaultFilePath); //show the default file path on the editLine
    }

    //set the progress bar to 0%
    ui->create_progress_bar->reset();
    ui->loaded_table_progress_bar->reset();
}

//Dialog Buttons for Creating Table
void MainWindow::on_create_new_table_button_box_accepted()
{
    window_fpcs->data_dump_onto_file();
    window_fpcs->close_file(); //close the file (which also flushes the buffer) before exiting

    window_fpcs->settings.usingExistingTable = false;
    bool fileInitialized = window_fpcs->initialize_workingFile();
    if(fileInitialized){
        //make the progress bar move
        ui->loaded_table_progress_bar->reset();
        for(int j=0; j<=100; j++){
            ui->create_progress_bar->setValue(j);
            QThread::msleep(3);
        }
        ui->current_table_line_edit->setText(window_fpcs->workingFile.fileName());
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
    if(window_fpcs->settings.usingExistingTableLocal == true){
    //prompt the user to select a folder from the local system
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"),
                "/fileFolder/downloads", tr("CSV Files (*.csv);;Text Files (*.txt)"));

    window_fpcs->settings.existingTableFilePath = filePath; //set the filePath

    ui->select_file_line_edit->setText(window_fpcs->settings.existingTableFilePath); //update the EditLine Box so the user can see the filePath they just chose

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

//TODO when an existing pattern is selected by checking the checkbox in it's row of the visualizer, a function is called that updates the fields of window_fpcs's workingEntry-> the streamer's
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
        QString enteredValue = ui->phase_freq_pattern_entry_table->item(row, column)->text().remove("hz");
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

//TODO
/*void MainWindow::uploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    // Display the progress of the upload
    ui->progressBar->setValue(100 * bytesSent/bytesTotal);
}*/

/*
 * An important part of the functionality of this function is that window_fpcs->workingFile.fileName() is the filename and directory whereas we set
 * the uploadingFileOrFolderName field to just be the name of the file without the directory. The reason behind this is so that the included directory
 * is needed for the ftp process (since it utilizes windows's ftp client routine and has no idea what the "current directory" is) whereas when the
 * ftp process is finished we send scpi commands to the UXG that only need the fileName (since it's on the UXG now that the ftp process is finished).
 */
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
    window_ftpManager->abortTcpSocket();
    window_ftpManager->connect(5025);  //K-N5193A-90114
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
    //TODO call a method window_fpcs->updateTableVisualizer(); which will check that bool and update the table view to be either binary bits or the layman's [phase,freq],... view
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

    if(!window_ftpManager->UXGSetupFinished&&allRead== "1\n"){
        window_ftpManager->UXGSetup();
    }
    else if(serial->simpleTestStarted&&allRead == "ARMED\n"){
        window_ftpManager->send_SCPI(":OUTPut OFF");
        window_ftpManager->send_SCPI(":OUTPut:MODulation OFF");
        window_ftpManager->send_SCPI(":STReam:STATe OFF");
        triggerSent=false;
        if(serial->manageSimpleGridTest()){
            ui->console_text_editor->appendPlainText("Test Finished");
            serial->simpleTestStarted=false;
            on_stopTestPushButton_clicked();

        }

    }else if(serial->advancedTestStarted&&allRead== "ARMED\n"){

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

    //this process is specifically for when the user is downloading a uxg fpcs file to then edit.
    /*if(window_ftpManager->downloadState == window_ftpManager->exportingTable){
        window_ftpManager->downloadState = window_ftpManager->finished;
        output_to_console("file is exported, ready for ftp");
        qDebug() << "file is exported, ready for ftp";
        //download the table from the uxg into the downloads folder
        window_ftpManager->current_state = FtpManager::state::downloading;
        //then feed it into the process
        window_ftpManager->start_process(window_fpcs->settings.existingTableFilePath); //note that start_process() only needs the name of the file
        //note we can immediately use the file after starting the process since it's a blocking call
        bool fileInitialized = window_fpcs->initialize_workingFile();
        if(fileInitialized){
            //make the progress bar move
            ui->create_progress_bar->reset();
            for(int j=0; j<=100; j++){
                ui->loaded_table_progress_bar->setValue(j);
                QThread::msleep(3);
            }
            ui->current_table_line_edit->setText(window_fpcs->workingFile.fileName());
            update_table_visualization();
        }else{
            output_to_console("File unable to initialize");
            qDebug() << "File unable to initialize";
        }
    }*/
}

void MainWindow::on_uxg_fpcs_files_combo_box_currentTextChanged(const QString &arg1)
{
    window_fpcs->settings.existingTableFilePath = arg1;
    window_fpcs->settings.existingTableFilePath = window_fpcs->settings.existingTableFilePath.remove('"').remove(" ");
}

/*
 * Dialog Buttons for loading Table
 * Assumes a file name has been chosen before pressed, but also checks if that's the case.
 *
 */
void MainWindow::on_select_existing_table_button_box_accepted()
{
    window_fpcs->data_dump_onto_file();
    window_fpcs->close_file(); //close the file (which also flushes the buffer) before exiting

    window_fpcs->settings.usingExistingTable = true;
    window_fpcs->workingEntryList.clear();

    if(window_fpcs->settings.usingExistingTableLocal == false){
        /*
         * extra steps required here to import the file from the uxg, then we initialize the file as if it already existed locally
         * tell the uxg to export the fpcs file as a csv
         */

        //select the chosen table as the current table on the UXG
        QString scpiCommand = ":SOURce:PULM:STReam:FPCSetup:SELect "; //Note this command will be rejected if the UXG is not in PDW Streaming Mode
        scpiCommand.append('"');
        scpiCommand.append(window_fpcs->settings.existingTableFilePath.append(".fpcs"));
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
        scpiCommand.append(window_fpcs->settings.existingTableFilePath.remove(".fpcs").append(".csv"));
        scpiCommand.append('"');
        output_to_console(scpiCommand);
        qDebug() << "now sending: " << scpiCommand;
        window_ftpManager->send_SCPI(scpiCommand);

        //-------------------------
        output_to_console("file is exported, ready for ftp");
        qDebug() << "file is exported, ready for ftp";
        //download the table from the uxg into the downloads folder
        window_ftpManager->current_state = FtpManager::state::downloading;
        //then feed it into the process
        window_ftpManager->start_process(window_fpcs->settings.existingTableFilePath); //note that start_process() only needs the name of the file
        //note we can immediately use the file after starting the process since it's a blocking call
        window_fpcs->settings.existingTableFilePath = QDir::currentPath() + "/fileFolder/downloads/" + window_fpcs->settings.existingTableFilePath;
        bool fileInitialized = window_fpcs->initialize_workingFile();
        if(fileInitialized){
            //make the progress bar move
            ui->create_progress_bar->reset();
            for(int j=0; j<=100; j++){
                ui->loaded_table_progress_bar->setValue(j);
                QThread::msleep(3);
            }
            ui->current_table_line_edit->setText(window_fpcs->workingFile.fileName());
            update_table_visualization();
        }else{
            output_to_console("File unable to initialize");
            qDebug() << "File unable to initialize";
        }
        //--------------------------------

    }else{
        bool fileInitialized = window_fpcs->initialize_workingFile();
        if(fileInitialized){
            //make the progress bar move
            ui->create_progress_bar->reset();
            for(int j=0; j<=100; j++){
                ui->loaded_table_progress_bar->setValue(j);
                QThread::msleep(3);
            }
            ui->current_table_line_edit->setText(window_fpcs->workingFile.fileName());
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
    window_ftpManager->start_process(window_fpcs->settings.existingTableFilePath); //note that start_process() only needs the name of the file
    //we can immediately begin using the file after this call because when the window_ftpManager current_state is downloading,
    //the start_process function will use a blocking function before returning

    //By this point the exported csv file representation of the fpcs file is downloaded over ftp into this program's downloads folder
    QFile tempFile;
    QString tempFilePath = QDir::currentPath() + "/fileFolder/downloads/" + window_fpcs->settings.existingTableFilePath;
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

    window_fpcs->settings.existingTableFilePath = tempFilePath;
    bool fileInitialized = window_fpcs->initialize_workingFile();
    if(fileInitialized){
        //make the progress bar move
        ui->create_progress_bar->reset();
        for(int j=0; j<=100; j++){
            ui->loaded_table_progress_bar->setValue(j);
            QThread::msleep(10);
        }
        ui->current_table_line_edit->setText(window_fpcs->workingFile.fileName());
    }else{
        output_to_console("File unable to initialize");
        qDebug() << "File unable to initialize";
    }

    return true;
    //TODO Update table visualization with the new data
}*/

void MainWindow::on_delete_table_from_uxg_push_button_clicked(){
    //make sure the correct radio button is selected
    if(window_fpcs->settings.usingExistingTableLocal == false){
        //delete the fpcs file under that name
        QString command = "MEMory:DELete:NAME ";
        command.append('"');
        command.append(window_fpcs->settings.existingTableFilePath.append(".fpcs"));
        command.append('"');
        window_ftpManager->send_SCPI(command);
        //delete the csv file under that name
        command.clear();
        command = "MEMory:DELete:NAME ";
        command.append('"');
        command.append(window_fpcs->settings.existingTableFilePath.remove(".fpcs").append(".csv"));
        command.append('"');
        window_ftpManager->send_SCPI(command);
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
        //qDebug() << "format 0 updating Table Visualization";
//        for(int i=0; i< window_fpcs->workingEntryList.length();i++){
//            ui->table_visualization_table_widget->item(rowIndex,0)
//                    ->setText(window_fpcs->workingEntryList.at(i).plainTextRepresentation);
//            rowIndex++;
//        }
        for(Entry *entry: window_fpcs->workingEntryList){
            //qDebug() << "plainText : " << entry.plainTextRepresentation;
            ui->table_visualization_table_widget->item(rowIndex,0)->setText(entry->plainTextRepresentation);
            rowIndex++;
        }
    }else{
        //the box is checked so display in binary format
        rowIndex = 0;
        //qDebug() << "format 1 updating Table Visualization";
//        for(int i=0; i<window_fpcs->workingEntryList.length(); i++){
//            qDebug() << window_fpcs->workingEntryList.at(i).plainTextRepresentation;
//            ui->table_visualization_table_widget->item(rowIndex,0)->setText(window_fpcs->workingEntryList.at(i).bitPattern);
//            rowIndex++;
//        }
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
    window_yatg->uploadingMultipleFiles = false;

    //prompt the user to select a folder from the local system
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"),
                "/fileFolder/uploads", tr("CSV Files (*.csv);;Text Files (*.txt)"));
    window_yatg->workingFilePath = filePath; //set the filepath

    ui->selected_yatg_file_line_editor->setText(window_yatg->workingFilePath); //update the EditLine Box so the user can see the filePath they just chose
    ui->select_multiple_files_by_folder_line_editor->clear();
}

void MainWindow::on_select_multiple_files_by_folder_push_button_clicked()
{
    window_yatg->uploadingMultipleFiles = true;

    //prompt the user to select a folder
    QString folderName = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                "/fileFolder/uploads", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    window_yatg->workingFilePath = folderName; //set the filepath

    ui->select_multiple_files_by_folder_line_editor->setText(window_yatg->workingFilePath);
    ui->selected_yatg_file_line_editor->clear();

}

void MainWindow::on_upload_yatg_file_to_uxg_push_button_clicked()
{
    bool success = true;
    if(window_yatg->uploadingMultipleFiles){
        success = window_yatg->upload_multiple_files_to_uxg();
    }else{
        success = window_yatg->upload_file_to_uxg();
    }
    if(!success){
        output_to_console("File unable to upload to UXG : ERROR");
        qDebug() << "File unable to upload to UXG : ERROR";
    }
}

void MainWindow::on_playPDWPushButton_clicked()
{
    QString filename = "'" + ui->pdwNameLineEdit->text() + "'";
    window_ftpManager->playPDW(filename, ui->continuousPDWCheckBox->isChecked());
}

void MainWindow::on_stopPDWPushButton_clicked()
{
    window_ftpManager->send_SCPI(":STReam:STATe OFF");
    window_ftpManager->send_SCPI(":OUTPut OFF");
    window_ftpManager->send_SCPI(":OUTPut:MODulation OFF");
}

//serial functions

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
//                    if(ui->continuousTriggerCheckBox->isChecked()){
//                        window_ftpManager->send_SCPI("STReam:TRIGger:PLAY:FILE:TYPE CONTinuous");
//                        window_ftpManager->send_SCPI(":STReam:TRIGger:PLAY:FILE:TYPE:CONTinuous TRIGger");
//                    }else{
//                        window_ftpManager->send_SCPI(":STReam:TRIGger:PLAY:FILE:TYPE SINGle");
//                    }
                    QString fileName = "";
                    if(ui->singlePDWCheckBox->isChecked()){
                        fileName = "'" + ui->singlePDWFileLineEdit->text() + "'";
                    }else{
                        fileName = "'" + QString::number(fileNumber) + ".csv'";
                        fileNumber++;
                    }
                    window_ftpManager->playPDW(fileName, ui->continuousTriggerCheckBox->isChecked());
//                    window_ftpManager->send_SCPI(":STReam:SOURce:FILE " + fileName);
//                    window_ftpManager->send_SCPI(":OUTPut ON");
//                    window_ftpManager->send_SCPI(":OUTPut:MODulation ON");
//                    window_ftpManager->send_SCPI(":STReam:STATe ON");
//                    window_ftpManager->send_SCPI("*TRG");

                    triggerSent=true;
                }
                rotorInPositionCounter++;
            }else{
                rotorInPositionCounter=0;
            }
        }
    }

}
/*called when the readyRead signal goes off.  reads the data and then takes the will update
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
            ui->maxSpeedSlider->setSliderPosition(7);
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
            serial->write("WN02;",isElevation);
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
        }else if(serial->advancedTestStarted){
            ui->rampSlider->setSliderPosition(2);
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
        serial->write("WK01080;",isElevation);
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
        ui->console_text_editor->appendPlainText("Finished Fixing Elevation");


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

void MainWindow::on_positionRadioButton_toggled(bool checked)
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

QString MainWindow::getFileName(){
    QString fileName = ui->testFileLineEdit->text();
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
{   serial->advancedTestStarted=true;
    if(ui->recommendedSpeedRadioButton->isChecked()){
        mainTimer->stop();
        //simpleTestStarted=true;
        //tcpSocket->UXGSetup();
        serial->write("WG24;",true);
        serial->write("WG24;",false);
    }else{
        startPositionTest();
    }

}

void MainWindow::startPositionTest(){
    QFile file(getFileName());

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
    QFile file(getFileName());

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
    QFile file(getFileName());

    if(file.open(QIODevice::WriteOnly)){
        QTextStream stream(&file);
        stream <<"advanced\r\n";
        stream << "step:"<<QString::number(tableAzResolution)<<","<<QString::number(tableElResolution)<<"\r\n";
        for(int i=0; i< tableWidgetItemList.length();i++){

            QString elPos = QString::number(ui->testCreatorTableWidget->model()->headerData(tableWidgetItemList.at(i)->row(),Qt::Vertical).toInt());
            QString azPos = QString::number(ui->testCreatorTableWidget->model()->headerData(tableWidgetItemList.at(i)->column(),Qt::Horizontal).toInt());

            stream << azPos << "," << elPos << "\r\n";
        }
    }else{
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
        serial->write("WG27;",true);
        serial->write("WG27;",false);
    }




}



//stops any test the is currently running
void MainWindow::on_stopTestPushButton_clicked()
{
    stopMotion();
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
}

//table functions

void MainWindow::on_changeTableResolutionPushButton_clicked()
{
    QList<QString> resolution = ui->tableResolutionLineEdit->text().split(',');
    resetTable(resolution.at(0).toInt(), resolution.at(1).toInt());

}

//when the select box or erase box radio button is toggled, clear the blue selection.
//Makes everything look nicer and easier to understand
void MainWindow::on_selectBoxesRadioButton_toggled(bool checked)
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


bool MainWindow::leftMouseButtonReleaseEvent(QMouseEvent *event){
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
        //return true;

    }else if(ui->eraseBoxesRadioButton->isChecked()){
        QList<QTableWidgetItem *> list = ui->testCreatorTableWidget->selectedItems();
        int firstChangedIndex = list.last()->text().toInt();

        for(int i=0; i<list.length() ;i++){
            if(list.at(i)->background().color()==QColor(243,112,33)){
                tableWidgetItemList.removeOne(list.at(i));
                QBrush *brush = new QBrush();
                list.at(i)->setBackground(*brush);
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
        //return true;
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
            serial->cantKeepUp = serial->manageDroneTest(data.at(0).toInt(),data.at(1).toInt(),
                                                 data.at(2).toInt(),data.at(3).toInt(),serial->refreshRate);
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




