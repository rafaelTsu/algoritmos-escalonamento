#pragma once

#include <QAbstractTableModel>
#include <QString>
#include <vector>

#include "process.hpp"

struct ProcessRow {
    int pid = 1;
    int arrival = 0;
    int priority = 1;
    QString bursts = "C5";
};

class ProcessTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { ColPid = 0, ColArrival, ColPriority, ColBursts, ColCount };

    explicit ProcessTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void addRow();
    void removeRowAt(int row);
    void clearRows();
    void setRows(const std::vector<ProcessRow>& rows);
    const std::vector<ProcessRow>& rows() const { return rows_; }

    std::vector<sched::Process> toProcesses(QString* error = nullptr) const;

    static std::vector<sched::Burst> parseBursts(const QString& s, bool* ok = nullptr);
    static QString burstsToString(const std::vector<sched::Burst>& bs);

private:
    std::vector<ProcessRow> rows_;
};
