#ifndef WDGWEITEERZUTATGABE_H
#define WDGWEITEERZUTATGABE_H

#include "wdgabstractproxy.h"

namespace Ui {
class WdgWeitereZutatGabe;
}

class WdgWeitereZutatGabe : public WdgAbstractProxy
{
    Q_OBJECT

public:
    explicit WdgWeitereZutatGabe(int row, QLayout *parentLayout, QWidget *parent = nullptr);
    ~WdgWeitereZutatGabe();
    bool isEnabled() const;
    bool isValid() const;
    QString name() const;

public slots:
    void updateValues(bool full = false);

private slots:
    void on_btnZutat_clicked();
    void on_btnLoeschen_clicked();
    void on_tbMenge_valueChanged(double value);
    void on_tbMengeTotal_valueChanged(double value);
    void on_cbZugabezeitpunkt_currentIndexChanged(int index);
    void on_tbKochdauer_valueChanged(int value);
    void on_tbExtrakt_valueChanged(double value);
    void on_btnZugeben_clicked();
    void on_cbEntnahme_clicked(bool checked);
    void on_tbZugabeNach_valueChanged(int arg1);
    void on_tbDauerTage_valueChanged(int value);
    void on_btnEntnehmen_clicked();
    void on_tbKomentar_textChanged();
    void on_tbDatumVon_dateChanged(const QDate &date);
    void on_tbDatumBis_dateChanged(const QDate &date);
    void on_btnAufbrauchen_clicked();
    void on_btnNachOben_clicked();
    void on_btnNachUnten_clicked();

private:
    void checkEnabled(bool force);

private:
    Ui::WdgWeitereZutatGabe *ui;
    bool mEnabled;
    bool mValid;
};

#endif // WDGWEITEERZUTATGABE_H
