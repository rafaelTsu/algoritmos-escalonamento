#include "process_table_model.h"

#include <QRegularExpression>
#include <QStringList>

ProcessTableModel::ProcessTableModel(QObject* parent)
    : QAbstractTableModel(parent) {
    rows_.push_back({1, 0, 1, "C5,I3,C2"});
    rows_.push_back({2, 1, 2, "C4"});
    rows_.push_back({3, 3, 3, "C7,I2,C3"});
}

int ProcessTableModel::rowCount(const QModelIndex&) const {
    return static_cast<int>(rows_.size());
}

int ProcessTableModel::columnCount(const QModelIndex&) const {
    return ColCount;
}

QVariant ProcessTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {};
    if (role != Qt::DisplayRole && role != Qt::EditRole) return {};
    const auto& r = rows_[index.row()];
    switch (index.column()) {
        case ColPid:      return r.pid;
        case ColArrival:  return r.arrival;
        case ColPriority: return r.priority;
        case ColBursts:   return r.bursts;
    }
    return {};
}

bool ProcessTableModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || role != Qt::EditRole) return false;
    auto& r = rows_[index.row()];
    bool ok = true;
    switch (index.column()) {
        case ColPid:      r.pid = value.toInt(&ok); break;
        case ColArrival:  r.arrival = value.toInt(&ok); break;
        case ColPriority: r.priority = value.toInt(&ok); break;
        case ColBursts:   r.bursts = value.toString(); break;
    }
    if (!ok) return false;
    emit dataChanged(index, index, {role});
    return true;
}

QVariant ProcessTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) return {};
    if (orientation == Qt::Horizontal) {
        switch (section) {
            case ColPid:      return "PID";
            case ColArrival:  return "Chegada";
            case ColPriority: return "Prioridade";
            case ColBursts:   return "Bursts (ex: C5,I3,C2)";
        }
    } else {
        return section + 1;
    }
    return {};
}

Qt::ItemFlags ProcessTableModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

void ProcessTableModel::addRow() {
    int next_pid = 1;
    for (const auto& r : rows_) next_pid = std::max(next_pid, r.pid + 1);
    beginInsertRows({}, rows_.size(), rows_.size());
    rows_.push_back({next_pid, 0, 1, "C5"});
    endInsertRows();
}

void ProcessTableModel::removeRowAt(int row) {
    if (row < 0 || row >= static_cast<int>(rows_.size())) return;
    beginRemoveRows({}, row, row);
    rows_.erase(rows_.begin() + row);
    endRemoveRows();
}

void ProcessTableModel::clearRows() {
    beginResetModel();
    rows_.clear();
    endResetModel();
}

void ProcessTableModel::setRows(const std::vector<ProcessRow>& rs) {
    beginResetModel();
    rows_ = rs;
    endResetModel();
}

std::vector<sched::Burst> ProcessTableModel::parseBursts(const QString& s, bool* ok) {
    if (ok) *ok = true;
    std::vector<sched::Burst> out;
    QRegularExpression sep("[,;\\s]+");
    QStringList tokens = s.split(sep, Qt::SkipEmptyParts);
    QRegularExpression re("^([CcIi]|CPU|cpu|IO|io)?\\s*:?\\s*(\\d+)$");
    for (const auto& tok : tokens) {
        auto m = re.match(tok);
        if (!m.hasMatch()) {
            if (ok) *ok = false;
            return {};
        }
        QString prefix = m.captured(1).toUpper();
        int duration = m.captured(2).toInt();
        if (duration <= 0) {
            if (ok) *ok = false;
            return {};
        }
        sched::BurstType t = sched::BurstType::CPU;
        if (prefix.startsWith("I")) t = sched::BurstType::IO;
        out.push_back({t, duration});
    }
    if (out.empty()) {
        if (ok) *ok = false;
    }
    return out;
}

QString ProcessTableModel::burstsToString(const std::vector<sched::Burst>& bs) {
    QStringList parts;
    for (const auto& b : bs) {
        parts << QString("%1%2").arg(b.type == sched::BurstType::CPU ? "C" : "I").arg(b.duration);
    }
    return parts.join(",");
}

std::vector<sched::Process> ProcessTableModel::toProcesses(QString* error) const {
    std::vector<sched::Process> result;
    result.reserve(rows_.size());
    for (size_t i = 0; i < rows_.size(); ++i) {
        const auto& r = rows_[i];
        bool ok = false;
        auto bursts = parseBursts(r.bursts, &ok);
        if (!ok || bursts.empty()) {
            if (error) *error = QString("Linha %1 (PID %2): bursts inválidos. Use formato como \"C5,I3,C2\".")
                                    .arg(i + 1).arg(r.pid);
            return {};
        }
        if (bursts.front().type != sched::BurstType::CPU) {
            if (error) *error = QString("Linha %1 (PID %2): primeiro burst deve ser CPU.")
                                    .arg(i + 1).arg(r.pid);
            return {};
        }
        sched::Process p;
        p.pid = r.pid;
        p.arrival = r.arrival;
        p.priority = r.priority;
        p.bursts = std::move(bursts);
        result.push_back(std::move(p));
    }
    return result;
}
