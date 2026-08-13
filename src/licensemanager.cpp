#include "licensemanager.h"
#include <QSettings>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>

// !! MORA BITI ISTI KAO U keygen/keygen.py !!
const QString LicenseManager::SECRET_KEY  = "SI2025-FilmoviSerije-XK7mP9qR";
const QString LicenseManager::VALID_CHARS = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

LicenseManager& LicenseManager::instance() {
    static LicenseManager inst;
    return inst;
}
LicenseManager::LicenseManager() {}

QString LicenseManager::computeChecksum(const QString &stripped) {
    QString data = stripped.left(18) + SECRET_KEY;
    QByteArray hash = QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Sha256);
    int i0 = (unsigned char)hash[0] % VALID_CHARS.length();
    int i1 = (unsigned char)hash[1] % VALID_CHARS.length();
    return QString(VALID_CHARS[i0]) + QString(VALID_CHARS[i1]);
}

bool LicenseManager::validateSerial(const QString &serial) {
    QString s = stripSerial(serial).toUpper();
    if (s.length() != 20) return false;
    for (QChar c : s)
        if (!VALID_CHARS.contains(c)) return false;
    // Check groups 0-2 only (positions 18-19 are global HMAC checksum)
    for (int g = 0; g < 3; g++) {
        int base = g * 5, sum = 0;
        for (int i = 0; i < 4; i++) sum += VALID_CHARS.indexOf(s[base+i]);
        if (VALID_CHARS.indexOf(s[base+4]) != sum % VALID_CHARS.length()) return false;
    }
    QString checksum = computeChecksum(s);
    return s[18] == checksum[0] && s[19] == checksum[1];
}

QString LicenseManager::formatSerial(const QString &raw) {
    QString r = raw.toUpper();
    if (r.length() != 20) return r;
    return r.left(5)+"-"+r.mid(5,5)+"-"+r.mid(10,5)+"-"+r.mid(15,5);
}

QString LicenseManager::stripSerial(const QString &serial) {
    QString s = serial; s.remove("-").remove(" ");
    return s.toUpper();
}

bool LicenseManager::isActivated() const {
    QSettings s("Acmigo", "AcmigoLicense");
    return validateSerial(s.value("serial","").toString());
}

LicenseManager::Status LicenseManager::activate(const QString &serial) {
    QString stripped = stripSerial(serial);
    if (!validateSerial(stripped)) return InvalidSerial;
    QSettings s("Acmigo", "AcmigoLicense");
    s.setValue("serial", stripped);
    s.setValue("activated_at", QDateTime::currentDateTime().toString(Qt::ISODate));
    s.sync();
    return Activated;
}

QString LicenseManager::savedSerial() const {
    QSettings s("Acmigo", "AcmigoLicense");
    QString raw = s.value("serial","").toString();
    if (raw.length() != 20) return "";
    return raw.left(5)+"-"+raw.mid(5,5)+"-*****-*****";
}

void LicenseManager::deactivate() {
    QSettings s("Acmigo", "AcmigoLicense");
    s.remove("serial"); s.remove("activated_at");
}
