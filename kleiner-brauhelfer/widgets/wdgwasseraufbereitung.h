#ifndef WDGWASSERAUFBEREITUNG_H
#define WDGWASSERAUFBEREITUNG_H

#include "wdgabstractproxy.h"

namespace Ui {
class WdgWasseraufbereitung;
}

class WdgWasseraufbereitung : public WdgAbstractProxy
{
    Q_OBJECT

public:
    explicit WdgWasseraufbereitung(int row, QLayout *parentLayout, QWidget *parent = nullptr);
    ~WdgWasseraufbereitung();

public slots:
    void updateValues(bool full = false);

private slots:
    void on_tbName_textChanged(const QString &text);
    void on_cbEinheit_currentIndexChanged(int index);
    void on_btnAuswahl_clicked();
    void on_tbMenge_valueChanged(double value);
    void on_tbMengeGesamt_valueChanged(double value);
    void on_tbRestalkalitaet_valueChanged(double value);
    void on_tbFaktor_valueChanged(double value);
    void on_btnAusgleichen_clicked();
    void on_btnNachOben_clicked();
    void on_btnNachUnten_clicked();
    void on_btnLoeschen_clicked();

private:
    void checkEnabled(bool force);

private:
    Ui::WdgWasseraufbereitung *ui;
    bool mEnabled;
};

#endif // WDGWASSERAUFBEREITUNG_H
