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
    QNetworkAccessManager *m_manager;

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

    void on_upload_table_to_uxg_pushbutton_clicked();

    void on_batch_pattern_entry_push_button_clicked();

   // void output_to_command_line_text_editor();

    void fn(QNetworkReply* reply);



private:

    //void uploadProgress(qint64 bytesSent, qint64 bytesTotal);  // Upload progress slot

    void uploadComplete(QNetworkReply *reply);  // Upload finish slot

    void auth();

    void rr();

    void po();
    void er();
    int looper = 0;


};
#endif // MAINWINDOW_H
