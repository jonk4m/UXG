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
    Fpcs window_fpcs; //each fpcs also has it's own fpcs_settings struct titled "settings"
    FtpManager *window_ftpManager;
    bool  waitingForFPCSFileList;
    enum DownloadState {
        finished = 0,
        exportingTable = 1,
        settingCurrentTable = 2
    };
    DownloadState downloadState = finished;

private slots:

    void on_specify_table_name_radio_button_clicked();

    void on_use_default_name_radio_button_clicked();

    void on_specified_table_name_line_edit_textChanged(const QString &arg1);

    void on_change_table_directory_push_button_clicked();

    void on_use_default_file_directory_check_box_stateChanged(int arg1);

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

    void update_pattern_table();

    void on_phase_freq_pattern_entry_table_cellChanged(int row, int column);

    void add_button_in_pattern_table_setup(int row);

    void on_phase_freq_pattern_entry_table_cellClicked(int row, int column);

    void on_remove_last_entry_pushbutton_clicked();

    void on_batch_pattern_entry_push_button_clicked();

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

private:

    bool pre_initialize_uxg_file();

    void output_to_console(QString);

    //These functions are specifically for table visualization purposes
    void update_table_visualization();

    void update_table_visualization_format();

    void translate_csv_row_into_entry_format();

};
#endif // MAINWINDOW_H
