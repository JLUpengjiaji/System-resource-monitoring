#include "ProcessList.h"
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QFontMetrics>
#include <QDebug>
#include <algorithm>

ProcessList::ProcessList(QWidget *parent)
    : QWidget(parent)
    , m_sortColumn(CpuColumn)
    , m_sortOrder(Qt::DescendingOrder)
{
    setupUI();
}

ProcessList::~ProcessList()
{
}

void ProcessList::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    QHBoxLayout* filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(5);

    m_filterLabel = new QLabel(tr("Search:"), this);
    filterLayout->addWidget(m_filterLabel);

    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(tr("Filter by process name..."));
    m_filterEdit->setClearButtonEnabled(true);
    filterLayout->addWidget(m_filterEdit, 1);

    m_countLabel = new QLabel(tr("0 processes"), this);
    filterLayout->addWidget(m_countLabel);

    mainLayout->addLayout(filterLayout);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(ColumnCount);
    m_table->setHorizontalHeaderLabels({
        tr("Process Name"),
        tr("PID"),
        tr("CPU (%)"),
        tr("Memory")
    });

    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(NameColumn, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(PidColumn, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(CpuColumn, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(MemoryColumn, QHeaderView::Fixed);

    m_table->setColumnWidth(PidColumn, 80);
    m_table->setColumnWidth(CpuColumn, 90);
    m_table->setColumnWidth(MemoryColumn, 120);

    m_table->verticalHeader()->setDefaultSectionSize(22);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(false);
    m_table->horizontalHeader()->setSortIndicatorShown(true);
    m_table->horizontalHeader()->setSectionsClickable(true);

    m_table->horizontalHeader()->setSortIndicator(m_sortColumn, m_sortOrder);

    mainLayout->addWidget(m_table, 1);

    connect(m_filterEdit, &QLineEdit::textChanged, this, &ProcessList::onFilterTextChanged);
    connect(m_table->horizontalHeader(), &QHeaderView::sectionClicked, this, &ProcessList::onHeaderClicked);

    setMinimumHeight(200);
}

void ProcessList::updateProcessList(const QVector<ProcessInfo>& processes)
{
    m_allProcesses = processes;
    updateTable();
}

void ProcessList::setFilter(const QString& filter)
{
    m_filterEdit->setText(filter);
}

void ProcessList::onHeaderClicked(int column)
{
    if (column == m_sortColumn) {
        m_sortOrder = (m_sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    } else {
        m_sortColumn = column;
        m_sortOrder = Qt::DescendingOrder;
    }

    m_table->horizontalHeader()->setSortIndicator(m_sortColumn, m_sortOrder);
    updateTable();
}

void ProcessList::onFilterTextChanged(const QString& text)
{
    m_filterText = text;
    updateTable();
}

void ProcessList::updateTable()
{
    m_filteredProcesses.clear();

    QString lowerFilter = m_filterText.toLower();
    for (const ProcessInfo& process : m_allProcesses) {
        if (processMatchesFilter(process, lowerFilter)) {
            m_filteredProcesses.append(process);
        }
    }

    std::sort(m_filteredProcesses.begin(), m_filteredProcesses.end(),
        [this](const ProcessInfo& a, const ProcessInfo& b) -> bool {
            bool result = false;
            switch (m_sortColumn) {
            case NameColumn:
                result = a.name.compare(b.name, Qt::CaseInsensitive) < 0;
                break;
            case PidColumn:
                result = a.pid < b.pid;
                break;
            case CpuColumn:
                result = a.cpuUsage < b.cpuUsage;
                break;
            case MemoryColumn:
                result = a.memoryUsage < b.memoryUsage;
                break;
            default:
                result = false;
                break;
            }

            if (m_sortOrder == Qt::DescendingOrder) {
                result = !result;
            }

            return result;
        });

    m_table->setRowCount(m_filteredProcesses.size());

    for (int row = 0; row < m_filteredProcesses.size(); row++) {
        const ProcessInfo& process = m_filteredProcesses[row];

        QTableWidgetItem* nameItem = new QTableWidgetItem(process.name);
        nameItem->setData(Qt::UserRole, process.name);
        m_table->setItem(row, NameColumn, nameItem);

        QTableWidgetItem* pidItem = new QTableWidgetItem(QString::number(process.pid));
        pidItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        pidItem->setData(Qt::UserRole, process.pid);
        m_table->setItem(row, PidColumn, pidItem);

        QTableWidgetItem* cpuItem = new QTableWidgetItem(QString::number(process.cpuUsage, 'f', 1));
        cpuItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        cpuItem->setData(Qt::UserRole, process.cpuUsage);

        if (process.cpuUsage > 50.0) {
            cpuItem->setForeground(QColor(255, 100, 100));
        } else if (process.cpuUsage > 25.0) {
            cpuItem->setForeground(QColor(255, 200, 100));
        }

        m_table->setItem(row, CpuColumn, cpuItem);

        QTableWidgetItem* memoryItem = new QTableWidgetItem(formatMemory(process.memoryUsage));
        memoryItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        memoryItem->setData(Qt::UserRole, process.memoryUsage);

        double memoryMB = process.memoryUsage / (1024.0 * 1024.0);
        if (memoryMB > 500.0) {
            memoryItem->setForeground(QColor(255, 100, 100));
        } else if (memoryMB > 200.0) {
            memoryItem->setForeground(QColor(255, 200, 100));
        }

        m_table->setItem(row, MemoryColumn, memoryItem);
    }

    m_countLabel->setText(tr("%1 processes").arg(m_filteredProcesses.size()));
}

bool ProcessList::processMatchesFilter(const ProcessInfo& process, const QString& filter)
{
    if (filter.isEmpty()) {
        return true;
    }

    QString processName = process.name.toLower();
    QString processPid = QString::number(process.pid);

    return processName.contains(filter) || processPid.contains(filter);
}

QString ProcessList::formatMemory(quint64 memory)
{
    double memoryKB = memory / 1024.0;
    double memoryMB = memoryKB / 1024.0;
    double memoryGB = memoryMB / 1024.0;

    if (memoryGB >= 1.0) {
        return QString::number(memoryGB, 'f', 2) + tr(" GB");
    } else if (memoryMB >= 1.0) {
        return QString::number(memoryMB, 'f', 1) + tr(" MB");
    } else if (memoryKB >= 1.0) {
        return QString::number(memoryKB, 'f', 0) + tr(" KB");
    } else {
        return QString::number(memory) + tr(" B");
    }
}

bool ProcessList::eventFilter(QObject* obj, QEvent* event)
{
    return QWidget::eventFilter(obj, event);
}
