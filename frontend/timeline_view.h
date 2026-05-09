#pragma once

#include <QWidget>

#include "process.hpp"

class TimelineView : public QWidget {
    Q_OBJECT

public:
    explicit TimelineView(QWidget* parent = nullptr);

    void setResult(const sched::SimulationResult& r, const std::vector<sched::Process>& procs);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    sched::SimulationResult result_;
    std::vector<sched::Process> processes_;
    bool has_data_ = false;

    int cell_w_ = 26;
    int cell_h_ = 26;
    int label_w_ = 70;
    int header_h_ = 22;
    int gantt_h_ = 30;
};
