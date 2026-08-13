#pragma once
#include <QString>
#include <QObject>

class LicenseManager : public QObject {
    Q_OBJECT
public:
    static LicenseManager& instance();
    enum Status { NotActivated, Activated, InvalidSerial, AlreadyActivated };
    bool isActivated() const;
    Status activate(const QString &serial);
    QString savedSerial() const;
    void deactivate();
    static bool validateSerial(const QString &serial);
    static QString formatSerial(const QString &raw);
    static QString stripSerial(const QString &serial);
private:
    LicenseManager();
    static QString computeChecksum(const QString &stripped);
    static const QString SECRET_KEY;
    static const QString VALID_CHARS;
};
