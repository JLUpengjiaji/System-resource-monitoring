#include "SettingsDialog.h"
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_updatingUI(false)
{
    setupUI();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUI()
{
    setWindowTitle(tr("Settings"));
    setMinimumWidth(400);
    setMaximumWidth(600);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    m_generalGroup = new QGroupBox(tr("General Settings"), this);
    QFormLayout* generalLayout = new QFormLayout(m_generalGroup);
    generalLayout->setSpacing(10);

    QLabel* refreshLabel = new QLabel(tr("Refresh Interval (ms):"), this);
    m_refreshIntervalSpinBox = new QSpinBox(this);
    m_refreshIntervalSpinBox->setRange(500, 5000);
    m_refreshIntervalSpinBox->setSingleStep(100);
    m_refreshIntervalSpinBox->setValue(1000);
    m_refreshIntervalSpinBox->setSuffix(tr(" ms"));
    m_refreshIntervalLabel = new QLabel(tr("(1000 ms = 1 second)"), this);
    m_refreshIntervalLabel->setStyleSheet("color: gray; font-size: 10px;");

    generalLayout->addRow(refreshLabel, m_refreshIntervalSpinBox);
    generalLayout->addRow(new QLabel(), m_refreshIntervalLabel);

    mainLayout->addWidget(m_generalGroup);

    m_cpuGroup = new QGroupBox(tr("CPU Alert Settings"), this);
    QFormLayout* cpuLayout = new QFormLayout(m_cpuGroup);
    cpuLayout->setSpacing(10);

    m_cpuAlertEnabled = new QCheckBox(tr("Enable CPU Alert"), this);
    m_cpuAlertEnabled->setChecked(true);
    cpuLayout->addRow(m_cpuAlertEnabled);

    QHBoxLayout* cpuThresholdLayout = new QHBoxLayout();
    cpuThresholdLayout->setSpacing(10);

    QLabel* cpuThresholdLabel = new QLabel(tr("Threshold:"), this);
    m_cpuThresholdSlider = new QSlider(Qt::Horizontal, this);
    m_cpuThresholdSlider->setRange(1, 100);
    m_cpuThresholdSlider->setValue(80);
    m_cpuThresholdSlider->setTickPosition(QSlider::TicksBelow);
    m_cpuThresholdSlider->setTickInterval(10);

    m_cpuThresholdSpinBox = new QSpinBox(this);
    m_cpuThresholdSpinBox->setRange(1, 100);
    m_cpuThresholdSpinBox->setValue(80);
    m_cpuThresholdSpinBox->setSuffix(tr(" %"));

    m_cpuValueLabel = new QLabel(tr("Current: 0.0%"), this);
    m_cpuValueLabel->setStyleSheet("color: gray; font-size: 10px;");

    cpuThresholdLayout->addWidget(cpuThresholdLabel);
    cpuThresholdLayout->addWidget(m_cpuThresholdSlider, 1);
    cpuThresholdLayout->addWidget(m_cpuThresholdSpinBox);

    cpuLayout->addRow(cpuThresholdLayout);
    cpuLayout->addRow(new QLabel(), m_cpuValueLabel);

    mainLayout->addWidget(m_cpuGroup);

    m_memoryGroup = new QGroupBox(tr("Memory Alert Settings"), this);
    QFormLayout* memoryLayout = new QFormLayout(m_memoryGroup);
    memoryLayout->setSpacing(10);

    m_memoryAlertEnabled = new QCheckBox(tr("Enable Memory Alert"), this);
    m_memoryAlertEnabled->setChecked(true);
    memoryLayout->addRow(m_memoryAlertEnabled);

    QHBoxLayout* memoryThresholdLayout = new QHBoxLayout();
    memoryThresholdLayout->setSpacing(10);

    QLabel* memoryThresholdLabel = new QLabel(tr("Threshold:"), this);
    m_memoryThresholdSlider = new QSlider(Qt::Horizontal, this);
    m_memoryThresholdSlider->setRange(1, 100);
    m_memoryThresholdSlider->setValue(85);
    m_memoryThresholdSlider->setTickPosition(QSlider::TicksBelow);
    m_memoryThresholdSlider->setTickInterval(10);

    m_memoryThresholdSpinBox = new QSpinBox(this);
    m_memoryThresholdSpinBox->setRange(1, 100);
    m_memoryThresholdSpinBox->setValue(85);
    m_memoryThresholdSpinBox->setSuffix(tr(" %"));

    m_memoryValueLabel = new QLabel(tr("Current: 0.0%"), this);
    m_memoryValueLabel->setStyleSheet("color: gray; font-size: 10px;");

    memoryThresholdLayout->addWidget(memoryThresholdLabel);
    memoryThresholdLayout->addWidget(m_memoryThresholdSlider, 1);
    memoryThresholdLayout->addWidget(m_memoryThresholdSpinBox);

    memoryLayout->addRow(memoryThresholdLayout);
    memoryLayout->addRow(new QLabel(), m_memoryValueLabel);

    mainLayout->addWidget(m_memoryGroup);

    mainLayout->addSpacing(10);

    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply,
        this
    );

    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    m_buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Apply"));

    mainLayout->addWidget(m_buttonBox);

    connect(m_cpuThresholdSlider, &QSlider::valueChanged, this, &SettingsDialog::onCpuSliderChanged);
    connect(m_cpuThresholdSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::onCpuSpinBoxChanged);

    connect(m_memoryThresholdSlider, &QSlider::valueChanged, this, &SettingsDialog::onMemorySliderChanged);
    connect(m_memoryThresholdSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::onMemorySpinBoxChanged);

    connect(m_refreshIntervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::onRefreshIntervalChanged);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::accept);
}

void SettingsDialog::setConfig(const AlertConfig& config)
{
    m_config = config;
    m_updatingUI = true;

    m_cpuAlertEnabled->setChecked(config.cpuAlertEnabled);
    m_cpuThresholdSlider->setValue(static_cast<int>(config.cpuThreshold));
    m_cpuThresholdSpinBox->setValue(static_cast<int>(config.cpuThreshold));

    m_memoryAlertEnabled->setChecked(config.memoryAlertEnabled);
    m_memoryThresholdSlider->setValue(static_cast<int>(config.memoryThreshold));
    m_memoryThresholdSpinBox->setValue(static_cast<int>(config.memoryThreshold));

    m_refreshIntervalSpinBox->setValue(config.refreshInterval);

    m_updatingUI = false;
}

AlertConfig SettingsDialog::getConfig() const
{
    AlertConfig config = m_config;

    config.cpuAlertEnabled = m_cpuAlertEnabled->isChecked();
    config.cpuThreshold = static_cast<double>(m_cpuThresholdSlider->value());

    config.memoryAlertEnabled = m_memoryAlertEnabled->isChecked();
    config.memoryThreshold = static_cast<double>(m_memoryThresholdSlider->value());

    config.refreshInterval = m_refreshIntervalSpinBox->value();

    return config;
}

void SettingsDialog::onCpuSliderChanged(int value)
{
    if (m_updatingUI) return;
    m_updatingUI = true;
    m_cpuThresholdSpinBox->setValue(value);
    m_updatingUI = false;
}

void SettingsDialog::onMemorySliderChanged(int value)
{
    if (m_updatingUI) return;
    m_updatingUI = true;
    m_memoryThresholdSpinBox->setValue(value);
    m_updatingUI = false;
}

void SettingsDialog::onCpuSpinBoxChanged(int value)
{
    if (m_updatingUI) return;
    m_updatingUI = true;
    m_cpuThresholdSlider->setValue(value);
    m_updatingUI = false;
}

void SettingsDialog::onMemorySpinBoxChanged(int value)
{
    if (m_updatingUI) return;
    m_updatingUI = true;
    m_memoryThresholdSlider->setValue(value);
    m_updatingUI = false;
}

void SettingsDialog::onRefreshIntervalChanged(int value)
{
    Q_UNUSED(value)
}

void SettingsDialog::updateLabels()
{
}
