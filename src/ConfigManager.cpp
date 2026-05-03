#include "ConfigManager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QStandardPaths>

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    m_configPath = configDir + "/config.json";
}

ConfigManager::~ConfigManager()
{
}

bool ConfigManager::loadConfig()
{
    QFile file(m_configPath);
    if (!file.exists()) {
        qDebug() << "Config file does not exist, using defaults";
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open config file:" << file.errorString();
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "Failed to parse config file:" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qDebug() << "Config file is not a valid JSON object";
        return false;
    }

    m_alertConfig = fromJson(doc.object());
    return true;
}

bool ConfigManager::saveConfig()
{
    QJsonDocument doc(toJson(m_alertConfig));

    QFile file(m_configPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open config file for writing:" << file.errorString();
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

AlertConfig ConfigManager::getAlertConfig() const
{
    return m_alertConfig;
}

void ConfigManager::setAlertConfig(const AlertConfig& config)
{
    m_alertConfig = config;
}

QString ConfigManager::getConfigFilePath() const
{
    return m_configPath;
}

QJsonObject ConfigManager::toJson(const AlertConfig& config) const
{
    QJsonObject obj;
    obj["cpuThreshold"] = config.cpuThreshold;
    obj["memoryThreshold"] = config.memoryThreshold;
    obj["cpuAlertEnabled"] = config.cpuAlertEnabled;
    obj["memoryAlertEnabled"] = config.memoryAlertEnabled;
    obj["refreshInterval"] = config.refreshInterval;
    return obj;
}

AlertConfig ConfigManager::fromJson(const QJsonObject& json) const
{
    AlertConfig config;

    if (json.contains("cpuThreshold") && json["cpuThreshold"].isDouble()) {
        config.cpuThreshold = json["cpuThreshold"].toDouble();
    }

    if (json.contains("memoryThreshold") && json["memoryThreshold"].isDouble()) {
        config.memoryThreshold = json["memoryThreshold"].toDouble();
    }

    if (json.contains("cpuAlertEnabled") && json["cpuAlertEnabled"].isBool()) {
        config.cpuAlertEnabled = json["cpuAlertEnabled"].toBool();
    }

    if (json.contains("memoryAlertEnabled") && json["memoryAlertEnabled"].isBool()) {
        config.memoryAlertEnabled = json["memoryAlertEnabled"].toBool();
    }

    if (json.contains("refreshInterval") && json["refreshInterval"].isDouble()) {
        config.refreshInterval = json["refreshInterval"].toInt();
    }

    return config;
}
