#include "timeline_view.h"

#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <algorithm>
#include <unordered_map>

TimelineView::TimelineView(QWidget* parent) : QWidget(parent) {
    setMinimumSize(400, 200);
}

void TimelineView::setResult(const sched::SimulationResult& r,
                             const std::vector<sched::Process>& procs) {
    result_ = r;
    processes_ = procs;
    has_data_ = true;
    updateGeometry();
    update();
}

void TimelineView::clear() {
    has_data_ = false;
    result_ = {};
    processes_.clear();
    update();
}

QSize TimelineView::sizeHint() const {
    if (!has_data_) return {600, 200};
    int n_ticks = static_cast<int>(result_.timeline.size());
    int n_procs = static_cast<int>(processes_.size());
    int w = label_w_ + std::max(n_ticks, 10) * cell_w_ + 20;
    int h = header_h_ + gantt_h_ + n_procs * cell_h_ + 60;
    return {w, h};
}

QSize TimelineView::minimumSizeHint() const {
    return sizeHint();
}

static QColor stateColor(sched::State s) {
    switch (s) {
        case sched::State::RUNNING:  return QColor("#2e7d32");
        case sched::State::READY:    return QColor("#fbc02d");
        case sched::State::BLOCKED:  return QColor("#c62828");
        case sched::State::FINISHED: return QColor("#bdbdbd");
        case sched::State::NEW:      return QColor("#eceff1");
    }
    return Qt::white;
}

static QColor pidColor(int pid) {
    static const QColor palette[] = {
        QColor("#1976d2"), QColor("#388e3c"), QColor("#f57c00"),
        QColor("#7b1fa2"), QColor("#c2185b"), QColor("#0097a7"),
        QColor("#5d4037"), QColor("#455a64"),
    };
    if (pid < 0) return QColor("#e0e0e0");
    return palette[pid % (sizeof(palette) / sizeof(palette[0]))];
}

void TimelineView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), Qt::white);
    p.setRenderHint(QPainter::Antialiasing, false);

    if (!has_data_) {
        p.setPen(Qt::gray);
        p.drawText(rect(), Qt::AlignCenter, "Execute uma simulação para visualizar a timeline.");
        return;
    }

    int n_ticks = static_cast<int>(result_.timeline.size());
    int n_procs = static_cast<int>(processes_.size());

    std::unordered_map<int, int> pid_to_row;
    for (int i = 0; i < n_procs; ++i) pid_to_row[processes_[i].pid] = i;

    QFont font = p.font();
    font.setPointSize(8);
    p.setFont(font);

    int x0 = label_w_;
    int y0 = header_h_;

    p.setPen(Qt::black);
    for (int t = 0; t <= n_ticks; ++t) {
        int x = x0 + t * cell_w_;
        if (t % 1 == 0) {
            p.drawText(QRect(x - cell_w_ / 2, 0, cell_w_, header_h_),
                       Qt::AlignCenter, QString::number(t));
        }
    }

    int gantt_y = y0;
    p.drawText(QRect(0, gantt_y, label_w_ - 4, gantt_h_),
               Qt::AlignVCenter | Qt::AlignRight, "CPU");
    for (int t = 0; t < n_ticks; ++t) {
        const auto& ev = result_.timeline[t];
        QRect cell(x0 + t * cell_w_, gantt_y, cell_w_, gantt_h_);
        QColor c = (ev.running_pid < 0) ? QColor("#eeeeee") : pidColor(ev.running_pid);
        p.fillRect(cell, c);
        p.setPen(Qt::black);
        p.drawRect(cell);
        QString label = (ev.running_pid < 0) ? "—" : QString("P%1").arg(ev.running_pid);
        p.setPen(ev.running_pid < 0 ? Qt::darkGray : Qt::white);
        p.drawText(cell, Qt::AlignCenter, label);
    }

    int proc_y0 = gantt_y + gantt_h_ + 6;
    p.setPen(Qt::black);
    for (int i = 0; i < n_procs; ++i) {
        int row_y = proc_y0 + i * cell_h_;
        QString lbl = QString("P%1").arg(processes_[i].pid);
        p.drawText(QRect(0, row_y, label_w_ - 4, cell_h_),
                   Qt::AlignVCenter | Qt::AlignRight, lbl);
    }

    for (int t = 0; t < n_ticks; ++t) {
        const auto& ev = result_.timeline[t];
        for (const auto& snap : ev.snapshots) {
            auto it = pid_to_row.find(snap.pid);
            if (it == pid_to_row.end()) continue;
            int row = it->second;
            QRect cell(x0 + t * cell_w_, proc_y0 + row * cell_h_, cell_w_, cell_h_);
            p.fillRect(cell, stateColor(snap.state));
            p.setPen(QColor("#cccccc"));
            p.drawRect(cell);
            QString letter;
            switch (snap.state) {
                case sched::State::RUNNING:  letter = "R"; break;
                case sched::State::READY:    letter = "P"; break;
                case sched::State::BLOCKED:  letter = "B"; break;
                case sched::State::FINISHED: letter = "F"; break;
                case sched::State::NEW:      letter = ""; break;
            }
            p.setPen(snap.state == sched::State::RUNNING || snap.state == sched::State::BLOCKED
                         ? Qt::white : Qt::black);
            p.drawText(cell, Qt::AlignCenter, letter);
        }
    }

    int legend_y = proc_y0 + n_procs * cell_h_ + 14;
    struct LegendItem { sched::State s; const char* label; };
    LegendItem items[] = {
        {sched::State::RUNNING,  "R: Executando"},
        {sched::State::READY,    "P: Pronto"},
        {sched::State::BLOCKED,  "B: Bloqueado (E/S)"},
        {sched::State::FINISHED, "F: Finalizado"},
        {sched::State::NEW,      "—: Não chegou"},
    };
    int lx = label_w_;
    for (auto& it : items) {
        QRect sw(lx, legend_y, 14, 14);
        p.fillRect(sw, stateColor(it.s));
        p.setPen(Qt::black);
        p.drawRect(sw);
        QFontMetrics fm(p.font());
        int text_w = fm.horizontalAdvance(it.label) + 6;
        p.drawText(QRect(lx + 18, legend_y - 1, text_w, 16),
                   Qt::AlignVCenter | Qt::AlignLeft, it.label);
        lx += 18 + text_w + 16;
    }
}
