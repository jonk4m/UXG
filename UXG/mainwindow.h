#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <fpcs.h>
#include "QFileDialog" //adds functionality for the user to select a directory
#include "QMessageBox"
#include "QTime"
#include "QThread"
#include <QtNetwork>
#include <QNetworkReply>
#include <QUrl>
#include "QTcpSocket"
#include "ftpmanager.h"
#include "QTextStream"
#include "QStringList"
#include "QTableWidget"
#include "QTableWidgetItem"
#include "yatg.h"
#include "rotorcontrol.h"
#include "udpsocket.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void fpcs_setup();
    Ui::MainWindow *ui;
    UdpSocket *udpSocket;
    Fpcs *window_fpcs; //each fpcs also has it's own fpcs_settings struct titled "settings"
    FtpManager *window_ftpManager;
    YATG *window_yatg;
    int highlightedFpcsRow = -1;
    bool fpcsWasOpenedDuringUse = false; //this way the window doesn't prompt the user to save their current table when they exit the window unless they have loaded or created one
    QElapsedTimer timer;

    void setup();
     //serial fields
     //test fields
     QList<QString> pdwFileNames;
     bool multiplePDWsPlaying=false;
     QList<QString> testPositions;
     bool positionTestHasBeenOpened=false;
     bool triggerSent=false;
     int fileNumber=1;
     //serial fields
     RotorControl *serial;
     int rotorInPositionCounter=0;
     //table fields
     int currentTableNumber = 0;
     QList<QTableWidgetItem *> tableWidgetItemList;
     int tableAzResolution=1;
     int tableElResolution=1;
     int tableCreatorRowOffset=180;
     bool rightMouseButtonPressed=false;
     //test functions
     void startPositionTest();
     void startSimpleTest();
     //table functions
     QString getFileName(bool isTestOptionsLineEdit);
     void resetTable(int azResolution, int elResolution);
     bool leftMouseButtonReleaseEvent(QMouseEvent *event);
     bool eventFilter(QObject *object, QEvent *event);

private slots:

    void on_specify_table_name_radio_button_clicked();

    void on_use_default_name_radio_button_clicked();

    //void on_specified_table_name_line_edit_textChanged(const QString &arg1);

    void on_change_table_directory_push_button_clicked();

    void on_create_new_table_button_box_accepted();

    void on_create_new_table_button_box_helpRequested();

    void on_select_existing_table_button_box_helpRequested();

    void on_select_file_push_button_clicked();

    void on_select_existing_table_button_box_accepted();

    void on_update_current_table_with_pattern_push_button_clicked();

    void on_add_another_pattern_to_this_table_push_button_clicked();

    void on_phase_pattern_type_radio_button_clicked();

    void on_freq_pattern_type_radio_button_clicked();

    void on_both_pattern_type_radio_button_clicked();

    void on_how_many_different_phase_or_freq_spin_box_valueChanged(int arg1);

    void on_phase_freq_pattern_entry_table_cellChanged(int row, int column);

    void add_button_in_pattern_table_setup(int row);

    void on_phase_freq_pattern_entry_table_cellClicked(int row, int column);

    void on_remove_last_entry_pushbutton_clicked();

    void on_qprocess_upload_push_button_clicked();

    void on_download_all_files_from_uxg_push_button_clicked();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_download_all_files_from_uxg_push_button_2_clicked();

    void on_select_local_file_radio_button_clicked();

    void on_select_file_from_uxg_radio_button_clicked();

    void on_cancel_editing_pattern_push_button_clicked();

    void on_clear_pattern_push_button_clicked();

    void on_power_off_uxg_push_button_clicked();

    void on_initiate_uxg_self_test_push_button_clicked();

    void on_delete_all_files_on_uxg_push_button_clicked();

    void on_binary_data_view_push_button_clicked(bool checked);

    void on_socket_readyRead();

    void on_uxg_fpcs_files_combo_box_currentTextChanged(const QString &arg1);

    void on_delete_table_from_uxg_push_button_clicked();

    void on_create_or_select_table_tab_widget_currentChanged(int index);

    void on_binary_data_view_push_button_stateChanged(int arg1);

    void on_delete_selected_row_push_button_clicked();

    void on_edit_selected_row_push_button_clicked();

    void output_to_console(QString);

    void on_table_visualization_table_widget_cellClicked(int row, int column);

    void on_host_name_edit_line_textChanged(const QString &arg1);

    void on_comboBox_activated(const QString &arg1);

    void on_select_yatg_file_push_button_clicked();

    void on_select_multiple_files_by_folder_push_button_clicked();

    void on_upload_yatg_file_to_uxg_push_button_clicked();

        //serial slots
    void on_elevationPushButton_clicked();

    void on_azimuthPushButton_clicked();

    void on_changeBothPushButton_clicked();

    void on_connectUSBPushButton_clicked();

    void on_maxSpeedSlider_sliderReleased();

    void on_minSpeedSlider_sliderReleased();

    void on_rampSlider_sliderReleased();

    void serialRead();

    void updatePositions();

    void on_stopButton_clicked();

    void on_elevationMotorSpeedRadioButton_toggled(bool checked);

    void on_positionRadioButton_toggled(bool checked);

    void on_fixElevationPushButton_clicked();

    void on_fixAzimuthPushButton_clicked();

    void stopMotion();

    void resendTimerTimeout();

    //test slots
    void on_openTestFilePushButton_clicked();

    void on_startTestButton_clicked();


    void on_OpenTestPushButton_clicked();

    void on_startDroneTestPushButton_clicked();

    void on_stopTestPushButton_clicked();

    //table slots
    void on_selectBoxesRadioButton_toggled(bool checked);

    void on_clearTablePushButton_clicked();

    void on_testCreatorTableWidget_cellEntered(int row, int column);

    void on_testCreatorTableWidget_itemChanged(QTableWidgetItem *item);

    void on_createTextFilePushButton_clicked();

    void on_changeTableResolutionPushButton_clicked();

    //Udp slots
    void UdpRead();

    void on_turnStreamOffPushButton_clicked();

    void on_playPDWPushButton_clicked();

    void on_stopPDWPushButton_clicked();

    void on_usingRotorCheckBox_toggled(bool checked);

    void on_startPositionTestPushButton_clicked();

    void on_openFilePushButton_clicked();

    void on_play_multiple_pdws_push_button_clicked();

    void on_create_yatg_template_file_pushbutton_clicked();

    void on_stopAllCurrentProcessesButton_clicked();

    void on_elevationModePushButton_clicked();

    void on_notElevationModePushButton_clicked();

    void on_testOptionsHelpButton_clicked();

private:

    //bool pre_initialize_uxg_file();

    //These functions are specifically for table visualization purposes
    void update_table_visualization();

    void update_pattern_edit_visualization();

    QTimer *mainTimer;

    QTimer *stopMotionTimer;

};
#endif // MAINWINDOW_H
