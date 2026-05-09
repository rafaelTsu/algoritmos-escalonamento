#include "mainwindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QTableView>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextStream>
#include <QVBoxLayout>
#include <QFile>
#include <QGroupBox>
#include <QHeaderView>

#include "process_table_model.h"
#include "timeline_view.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildUi();
    setWindowTitle("Simulador de Escalonamento — Sistemas Operacionais");
    resize(1200, 820);
}

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);

    auto* config_box = new QGroupBox("Configuração", this);
    auto* cfg_layout = new QHBoxLayout(config_box);

    cfg_layout->addWidget(new QLabel("Algoritmo:"));
    algo_combo_ = new QComboBox(this);
    algo_combo_->addItem("FCFS",         static_cast<int>(sched::Algorithm::FCFS));
    algo_combo_->addItem("SJF",          static_cast<int>(sched::Algorithm::SJF));
    algo_combo_->addItem("Prioridade",   static_cast<int>(sched::Algorithm::PRIORITY));
    algo_combo_->addItem("Round Robin",  static_cast<int>(sched::Algorithm::ROUND_ROBIN));
    cfg_layout->addWidget(algo_combo_);

    preempt_check_ = new QCheckBox("Preemptivo", this);
    cfg_layout->addWidget(preempt_check_);

    quantum_label_ = new QLabel("Quantum:", this);
    cfg_layout->addWidget(quantum_label_);
    quantum_spin_ = new QSpinBox(this);
    quantum_spin_->setRange(1, 10000);
    quantum_spin_->setValue(4);
    cfg_layout->addWidget(quantum_spin_);

    max_time_label_ = new QLabel("Tempo máx:", this);
    cfg_layout->addWidget(max_time_label_);
    max_time_spin_ = new QSpinBox(this);
    max_time_spin_->setRange(0, 100000);
    max_time_spin_->setValue(0);
    max_time_spin_->setSpecialValueText("ilimitado");
    max_time_spin_->setToolTip(
        "Limite de ticks. 0 = roda até todos terminarem. "
        "Se o limite for atingido antes, processos incompletos aparecem na tabela "
        "com seu estado final.");
    cfg_layout->addWidget(max_time_spin_);

    cfg_layout->addStretch();

    run_btn_ = new QPushButton("▶  Executar simulação", this);
    run_btn_->setStyleSheet("font-weight: bold; padding: 6px 14px;");
    cfg_layout->addWidget(run_btn_);

    root->addWidget(config_box);

    auto* proc_box = new QGroupBox("Processos", this);
    auto* proc_layout = new QVBoxLayout(proc_box);

    auto* proc_btns = new QHBoxLayout();
    add_btn_ = new QPushButton("+ Adicionar", this);
    remove_btn_ = new QPushButton("− Remover selecionado", this);
    load_btn_ = new QPushButton("Carregar arquivo…", this);
    save_btn_ = new QPushButton("Salvar arquivo…", this);
    proc_btns->addWidget(add_btn_);
    proc_btns->addWidget(remove_btn_);
    proc_btns->addStretch();
    proc_btns->addWidget(load_btn_);
    proc_btns->addWidget(save_btn_);
    proc_layout->addLayout(proc_btns);

    proc_model_ = new ProcessTableModel(this);
    proc_table_ = new QTableView(this);
    proc_table_->setModel(proc_model_);
    proc_table_->horizontalHeader()->setStretchLastSection(true);
    proc_table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    proc_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    proc_table_->setMaximumHeight(200);
    proc_layout->addWidget(proc_table_);

    auto* hint = new QLabel(
        "Bursts: lista separada por vírgula. <b>C</b><i>n</i> = CPU n unidades, "
        "<b>I</b><i>n</i> = E/S n unidades. Ex: <code>C5,I3,C2</code>. "
        "Prioridade: menor número = maior prioridade.", this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #555; font-size: 11px;");
    proc_layout->addWidget(hint);

    root->addWidget(proc_box);

    auto* result_tabs = new QTabWidget(this);

    timeline_view_ = new TimelineView(this);
    auto* tl_scroll = new QScrollArea(this);
    tl_scroll->setWidget(timeline_view_);
    tl_scroll->setWidgetResizable(true);
    result_tabs->addTab(tl_scroll, "Timeline (tick-a-tick)");

    tick_log_ = new QPlainTextEdit(this);
    tick_log_->setReadOnly(true);
    QFont mono("Monospace");
    mono.setStyleHint(QFont::TypeWriter);
    tick_log_->setFont(mono);
    result_tabs->addTab(tick_log_, "Log textual");

    stats_table_ = new QTableWidget(this);
    stats_table_->setColumnCount(8);
    stats_table_->setHorizontalHeaderLabels(
        {"PID", "Chegada", "Estado final", "Conclusão", "Turnaround",
         "Espera", "Resposta", "CPU usada"});
    stats_table_->horizontalHeader()->setStretchLastSection(true);
    stats_table_->verticalHeader()->setVisible(false);
    result_tabs->addTab(stats_table_, "Estatísticas");

    root->addWidget(result_tabs, 1);

    summary_label_ = new QLabel(this);
    summary_label_->setStyleSheet("padding: 4px; color: #333;");
    root->addWidget(summary_label_);

    setCentralWidget(central);

    connect(algo_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onAlgorithmChanged);
    connect(add_btn_, &QPushButton::clicked, this, &MainWindow::onAddRow);
    connect(remove_btn_, &QPushButton::clicked, this, &MainWindow::onRemoveRow);
    connect(load_btn_, &QPushButton::clicked, this, &MainWindow::onLoadFile);
    connect(save_btn_, &QPushButton::clicked, this, &MainWindow::onSaveFile);
    connect(run_btn_, &QPushButton::clicked, this, &MainWindow::onRun);

    onAlgorithmChanged(algo_combo_->currentIndex());
}

void MainWindow::onAlgorithmChanged(int index) {
    auto algo = static_cast<sched::Algorithm>(algo_combo_->itemData(index).toInt());
    bool is_rr = (algo == sched::Algorithm::ROUND_ROBIN);
    bool can_preempt = (algo == sched::Algorithm::SJF || algo == sched::Algorithm::PRIORITY);
    quantum_label_->setVisible(is_rr);
    quantum_spin_->setVisible(is_rr);
    preempt_check_->setVisible(can_preempt);
}

void MainWindow::onAddRow() {
    proc_model_->addRow();
}

void MainWindow::onRemoveRow() {
    auto sel = proc_table_->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;
    std::vector<int> rows;
    for (const auto& idx : sel) rows.push_back(idx.row());
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows) proc_model_->removeRowAt(r);
}

void MainWindow::onLoadFile() {
    QString path = QFileDialog::getOpenFileName(this, "Carregar processos", QString(),
                                                "Texto (*.txt);;Todos (*)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Erro", "Não foi possível abrir o arquivo.");
        return;
    }
    QTextStream in(&f);
    std::vector<ProcessRow> rows;
    int line_no = 0;
    while (!in.atEnd()) {
        line_no++;
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() < 4) {
            QMessageBox::warning(this, "Erro de parsing",
                                 QString("Linha %1: esperado 'pid chegada prioridade burst1 burst2 ...'.")
                                     .arg(line_no));
            return;
        }
        ProcessRow r;
        bool ok1, ok2, ok3;
        r.pid = parts[0].toInt(&ok1);
        r.arrival = parts[1].toInt(&ok2);
        r.priority = parts[2].toInt(&ok3);
        if (!ok1 || !ok2 || !ok3) {
            QMessageBox::warning(this, "Erro de parsing",
                                 QString("Linha %1: pid/chegada/prioridade devem ser inteiros.").arg(line_no));
            return;
        }
        QStringList burst_parts;
        for (int i = 3; i < parts.size(); ++i) burst_parts << parts[i];
        r.bursts = burst_parts.join(",");
        rows.push_back(r);
    }
    if (rows.empty()) {
        QMessageBox::information(this, "Vazio", "Nenhum processo encontrado no arquivo.");
        return;
    }
    proc_model_->setRows(rows);
}

void MainWindow::onSaveFile() {
    QString path = QFileDialog::getSaveFileName(this, "Salvar processos", "processos.txt",
                                                "Texto (*.txt)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Erro", "Não foi possível salvar.");
        return;
    }
    QTextStream out(&f);
    out << "# pid chegada prioridade bursts (separados por espaço; C=CPU, I=E/S)\n";
    for (const auto& r : proc_model_->rows()) {
        QString bursts = r.bursts;
        bursts.replace(QRegularExpression("[,;]"), " ");
        bursts = bursts.simplified();
        out << r.pid << " " << r.arrival << " " << r.priority << " " << bursts << "\n";
    }
}

void MainWindow::onRun() {
    QString err;
    auto procs = proc_model_->toProcesses(&err);
    if (procs.empty()) {
        QMessageBox::warning(this, "Entrada inválida",
                             err.isEmpty() ? "Adicione ao menos um processo." : err);
        return;
    }

    sched::Config cfg;
    cfg.algorithm = static_cast<sched::Algorithm>(
        algo_combo_->itemData(algo_combo_->currentIndex()).toInt());
    cfg.preemptive = preempt_check_->isChecked();
    cfg.quantum = quantum_spin_->value();
    cfg.max_time = max_time_spin_->value();

    auto result = sched::run(procs, cfg);
    displayResult(result, procs, cfg);
}

void MainWindow::displayResult(const sched::SimulationResult& r,
                               const std::vector<sched::Process>& procs,
                               const sched::Config& cfg) {
    timeline_view_->setResult(r, procs);
    displayTickLog(r);
    displayStats(r);

    QString algo_label = sched::algorithm_name(cfg.algorithm);
    if (cfg.algorithm == sched::Algorithm::SJF || cfg.algorithm == sched::Algorithm::PRIORITY) {
        algo_label += cfg.preemptive ? " (preemptivo)" : " (não preemptivo)";
    } else if (cfg.algorithm == sched::Algorithm::ROUND_ROBIN) {
        algo_label += QString(" (quantum=%1)").arg(cfg.quantum);
    }

    QString warning;
    if (r.time_limit_reached) {
        int incomplete = r.total_count - r.finished_count;
        warning = QString("<span style='color:#c62828; font-weight:bold;'>"
                          "  ⚠ Limite de tempo atingido — %1 processo(s) não finalizaram.</span>")
                      .arg(incomplete);
    }

    summary_label_->setText(
        QString("<b>%1</b> &nbsp;|&nbsp; duração: %2 ticks &nbsp;|&nbsp; "
                "finalizados: %3/%4 &nbsp;|&nbsp; "
                "turnaround médio: %5 &nbsp;|&nbsp; espera média: %6 &nbsp;|&nbsp; "
                "resposta média: %7 &nbsp;|&nbsp; uso de CPU: %8%%9")
            .arg(algo_label)
            .arg(r.total_time)
            .arg(r.finished_count).arg(r.total_count)
            .arg(r.avg_turnaround, 0, 'f', 2)
            .arg(r.avg_waiting, 0, 'f', 2)
            .arg(r.avg_response, 0, 'f', 2)
            .arg(r.cpu_utilization * 100.0, 0, 'f', 1)
            .arg(warning));
}

static QString stateLabel(sched::State s) {
    switch (s) {
        case sched::State::RUNNING:  return "EXEC";
        case sched::State::READY:    return "PRONTO";
        case sched::State::BLOCKED:  return "BLOQ";
        case sched::State::FINISHED: return "FIM";
        case sched::State::NEW:      return "NOVO";
    }
    return "?";
}

void MainWindow::displayTickLog(const sched::SimulationResult& r) {
    QString out;
    for (const auto& ev : r.timeline) {
        QStringList ready, blocked, finished;
        QString running = "—";
        for (const auto& s : ev.snapshots) {
            switch (s.state) {
                case sched::State::RUNNING:  running = QString("P%1").arg(s.pid); break;
                case sched::State::READY:    ready << QString("P%1").arg(s.pid); break;
                case sched::State::BLOCKED:  blocked << QString("P%1").arg(s.pid); break;
                case sched::State::FINISHED: finished << QString("P%1").arg(s.pid); break;
                case sched::State::NEW:      break;
            }
        }
        out += QString("t=%1→%2  EXEC: %3   PRONTO: [%4]   BLOQ: [%5]   FIM: [%6]")
                   .arg(ev.time_start, 3)
                   .arg(ev.time_end, 3)
                   .arg(running, -4)
                   .arg(ready.join(", "))
                   .arg(blocked.join(", "))
                   .arg(finished.join(", "));
        if (!ev.note.empty()) out += "   ← " + QString::fromStdString(ev.note);
        out += "\n";
    }
    tick_log_->setPlainText(out);
}

void MainWindow::displayStats(const sched::SimulationResult& r) {
    stats_table_->setRowCount(static_cast<int>(r.stats.size()));
    for (int i = 0; i < static_cast<int>(r.stats.size()); ++i) {
        const auto& s = r.stats[i];
        auto setCell = [&](int col, const QString& text, bool incomplete = false) {
            auto* item = new QTableWidgetItem(text);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            item->setTextAlignment(Qt::AlignCenter);
            if (incomplete) {
                item->setBackground(QColor("#fff3e0"));
                item->setForeground(QColor("#bf360c"));
            }
            stats_table_->setItem(i, col, item);
        };
        bool incomplete = !s.finished;
        QString final_state;
        switch (s.final_state) {
            case sched::State::FINISHED: final_state = "finalizado"; break;
            case sched::State::RUNNING:  final_state = "executando"; break;
            case sched::State::READY:    final_state = "pronto";     break;
            case sched::State::BLOCKED:  final_state = "bloqueado";  break;
            case sched::State::NEW:      final_state = "não chegou"; break;
        }
        setCell(0, QString("P%1").arg(s.pid), incomplete);
        setCell(1, QString::number(s.arrival), incomplete);
        setCell(2, final_state, incomplete);
        setCell(3, s.completion < 0 ? "—" : QString::number(s.completion), incomplete);
        setCell(4, s.turnaround < 0 ? "—" : QString::number(s.turnaround), incomplete);
        setCell(5, QString::number(s.waiting), incomplete);
        setCell(6, s.response < 0 ? "—" : QString::number(s.response), incomplete);
        setCell(7, QString::number(s.time_running), incomplete);
    }
    stats_table_->resizeColumnsToContents();
}
