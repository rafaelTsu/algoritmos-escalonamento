#pragma once

#include <QMainWindow>

#include "scheduler.hpp"

class QComboBox;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QTableView;
class QPlainTextEdit;
class QTableWidget;
class QLabel;
class ProcessTableModel;
class TimelineView;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onAddRow();
    void onRemoveRow();
    void onLoadFile();
    void onSaveFile();
    void onRun();
    void onAlgorithmChanged(int index);

private:
    void buildUi();
    void displayResult(const sched::SimulationResult& r,
                       const std::vector<sched::Process>& procs,
                       const sched::Config& cfg);
    void displayTickLog(const sched::SimulationResult& r);
    void displayStats(const sched::SimulationResult& r);

    QComboBox* algo_combo_ = nullptr;
    QSpinBox* quantum_spin_ = nullptr;
    QLabel* quantum_label_ = nullptr;
    QSpinBox* max_time_spin_ = nullptr;
    QLabel* max_time_label_ = nullptr;
    QCheckBox* preempt_check_ = nullptr;
    QPushButton* add_btn_ = nullptr;
    QPushButton* remove_btn_ = nullptr;
    QPushButton* load_btn_ = nullptr;
    QPushButton* save_btn_ = nullptr;
    QPushButton* run_btn_ = nullptr;
    QTableView* proc_table_ = nullptr;
    ProcessTableModel* proc_model_ = nullptr;
    TimelineView* timeline_view_ = nullptr;
    QPlainTextEdit* tick_log_ = nullptr;
    QTableWidget* stats_table_ = nullptr;
    QLabel* summary_label_ = nullptr;
};
