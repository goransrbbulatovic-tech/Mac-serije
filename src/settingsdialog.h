#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QLabel>

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent, const QString &currentKey);
    QString apiKey() const;
    static QString loadApiKey();
    static void    saveApiKey(const QString &key);
private:
    void setupUi(const QString &currentKey);
    QLineEdit *m_keyInput;
};
