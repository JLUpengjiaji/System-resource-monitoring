#ifndef PROCESSLIST_H
#define PROCESSLIST_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QTimer>
#include <QMap>
#include "SystemInfo.h"

class ProcessList : public QWidget
{
    Q_OBJECT

public:
    enum Columns
    {
        NameColumn,
        PidColumn,
        CpuColumn,
        MemoryColumn,
        ColumnCount
    };

    explicit ProcessList(QWidget *parent = nullptr);
    ~ProcessList();

    void updateProcessList(const QVector<ProcessInfo>& processes);
    void setFilter(const QString& filter);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onHeaderClicked(int column);
    void onFilterTextChanged(const QString& text);

private:
    void setupUI();
    void updateTable();
    bool processMatchesFilter(const ProcessInfo& process, const QString& filter);
    QString formatMemory(quint64 memory);

    QTableWidget* m_table;
    QLineEdit* m_filterEdit;
    QLabel* m_filterLabel;
    QLabel* m_countLabel;

    QVector<ProcessInfo> m_allProcesses;
    QVector<ProcessInfo> m_filteredProcesses;
    QString m_filterText;

    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
};

#endif // PROCESSLIST_H
