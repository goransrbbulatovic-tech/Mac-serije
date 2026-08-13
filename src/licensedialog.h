#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

class LicenseDialog : public QDialog {
    Q_OBJECT
public:
    explicit LicenseDialog(QWidget *parent=nullptr, bool firstRun=true);
private slots:
    void onActivate();
    void onSerialChanged(const QString &text);
private:
    void setupUi(bool firstRun);
    QLineEdit *m_serialInput;
    QLabel *m_statusLabel;
    QPushButton *m_activateBtn;
};
