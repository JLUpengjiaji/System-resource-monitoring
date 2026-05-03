#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QJsonObject>

struct AlertConfig
{
    double cpuThreshold;
    double memoryThreshold;
    bool cpuAlertEnabled;
    bool memoryAlertEnabled;
    int refreshInterval;

    AlertConfig()
        : cpuThreshold(80.0)
        , memoryThreshold(85.0)
        , cpuAlertEnabled(true)
        , memoryAlertEnabled(true)
        , refreshInterval(1000)
    {}
};

class ConfigManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfigManager(QObject *parent = nullptr);
    ~ConfigManager();

    bool loadConfig();
    bool saveConfig();

    AlertConfig getAlertConfig() const;
    void setAlertConfig(const AlertConfig& config);

    QString getConfigFilePath() const;

private:
    QString m_configPath;
    AlertConfig m_alertConfig;

    QJsonObject toJson(const AlertConfig& config) const;
    AlertConfig fromJson(const QJsonObject& json) const;
};

#endif // CONFIGMANAGER_H
