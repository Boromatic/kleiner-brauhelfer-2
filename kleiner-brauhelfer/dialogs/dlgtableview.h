#ifndef DLGTABLEVIEW_H
#define DLGTABLEVIEW_H

#include <QDialog>
#include <QMap>

#include "widgets/tableview.h"

class SqlTableModel;

namespace Ui {
class DlgTableView;
}

class DlgTableView : public QDialog
{
    Q_OBJECT

public:

    static void restoreView();

    explicit DlgTableView(SqlTableModel* model,
                          QList<TableView::ColumnDefinition> columns,
                          QMap<int, QVariant> defaultValues = QMap<int, QVariant>(),
                          int filterColumn = -1,
                          QWidget *parent = nullptr);
    ~DlgTableView();

private slots:
    void on_tbFilter_textChanged(const QString &txt);
    void on_btnAdd_clicked();
    void on_btnDelete_clicked();
    void on_btnMoveUp_clicked();
    void on_moveDown_clicked();

private:
    Ui::DlgTableView *ui;
    QMap<int, QVariant> mDefaultValues;
};

#endif // DLGTABLEVIEW_H
