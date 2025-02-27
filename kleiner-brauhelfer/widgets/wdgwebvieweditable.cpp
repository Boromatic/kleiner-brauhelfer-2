#include "wdgwebvieweditable.h"
#include "ui_wdgwebvieweditable.h"
#include <QDir>
#include <QMessageBox>
#ifdef QT_PRINTSUPPORT_LIB
#include <QPrinterInfo>
#include <QPrintPreviewDialog>
#endif
#include <QJsonDocument>
#include <QFileDialog>
#include <QDesktopServices>
#include "brauhelfer.h"
#include "settings.h"

extern Brauhelfer* bh;
extern Settings* gSettings;

double WdgWebViewEditable::gZoomFactor = 1.0;

WdgWebViewEditable::WdgWebViewEditable(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WdgWebViewEditable),
    mTempCssFile(QDir::tempPath() + "/" + QCoreApplication::applicationName() + QLatin1String(".XXXXXX.css"))
{
    ui->setupUi(this);
    ui->webview->setLinksExternal(true);
    ui->tbTemplate->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  #if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
   #if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    ui->tbTemplate->setTabStopDistance(QFontMetrics(ui->tbTemplate->font()).horizontalAdvance("  "));
   #else
    ui->tbTemplate->setTabStopDistance(2 * QFontMetrics(ui->tbTemplate->font()).width(' '));
   #endif
  #endif
    mHtmlHightLighter = new HtmlHighLighter(ui->tbTemplate->document());
    ui->tbTags->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  #if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
   #if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    ui->tbTags->setTabStopDistance(QFontMetrics(ui->tbTags->font()).horizontalAdvance("  "));
   #else
    ui->tbTags->setTabStopDistance(2 * QFontMetrics(ui->tbTags->font()).width(' '));
   #endif
  #endif
    ui->btnSaveTemplate->setPalette(gSettings->paletteErrorButton);
    mTimerWebViewUpdate.setSingleShot(true);
    connect(&mTimerWebViewUpdate, SIGNAL(timeout()), this, SLOT(updateWebView()), Qt::QueuedConnection);
    updateEditMode();
    ui->splitterEditmode->setHandleWidth(0);
    ui->splitterEditmode->setSizes({1, 0});
    gZoomFactor = gSettings->valueInGroup("General", "WebViewZoomFactor", 1.0).toDouble();
}

WdgWebViewEditable::~WdgWebViewEditable()
{
    delete ui;
    gSettings->setValueInGroup("General", "WebViewZoomFactor", gZoomFactor);
}

void WdgWebViewEditable::clear()
{
    ui->webview->setHtml("");
}

void WdgWebViewEditable::setHtmlFile(const QString& file)
{
    QString fileComplete;
    QString lang = gSettings->language();
    if (lang == "de")
    {
        fileComplete = file + ".html";
    }
    else
    {
        fileComplete = file + "_" + lang + ".html";
        if (!QFile::exists(gSettings->dataDir(1) + fileComplete))
            fileComplete = file + ".html";
    }
    htmlName = file;
    ui->cbTemplateAuswahl->setItemText(0, fileComplete);
    ui->webview->setTemplateFile(gSettings->dataDir(1) + fileComplete);
}

void WdgWebViewEditable::setPrintable(bool isPrintable)
{
    ui->btnPdf->setVisible(isPrintable);
    ui->btnPrint->setVisible(isPrintable);
}

#ifdef QT_PRINTSUPPORT_LIB
void WdgWebViewEditable::printDocument(QPrinter *printer)
{
#if defined(QT_WEBENGINECORE_LIB) && (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
    QEventLoop loop;
  #if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    ui->webview->print(printer);
    connect(ui->webview, SIGNAL(printFinished(bool)), &loop, SLOT(quit()));
  #else
    ui->webview->page()->print(printer, [&](bool) { loop.quit(); });
  #endif
    loop.exec();
    gSettings->beginGroup("General");
    gSettings->setValue("DefaultPrinter", printer->printerName());
  #if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QMarginsF margins = printer->pageLayout().margins(QPageLayout::Millimeter);
    QRectF rect(margins.left(), margins.top(), margins.right(), margins.bottom());
  #else
    QRectF rect(printer->margins().left, printer->margins().top, printer->margins().right, printer->margins().bottom);
  #endif
    gSettings->setValue("PrintMargins", rect);
    gSettings->endGroup();
#else
  Q_UNUSED(printer)
#endif
}
#endif

void WdgWebViewEditable::printPreview()
{
  #if defined(QT_PRINTSUPPORT_LIB) && defined(QT_WEBENGINECORE_LIB)
    QVariant style;
    if (gSettings->theme() == Settings::Dark)
    {
        style = mTemplateTags["Style"];
        mTemplateTags["Style"] = "style_hell.css";
        ui->webview->setUpdatesEnabled(false);

        QEventLoop loop;
        connect(ui->webview, SIGNAL(loadFinished(bool)), &loop, SLOT(quit()));
        ui->webview->renderTemplate(mTemplateTags);
        loop.exec();
    }

    gSettings->beginGroup("General");
    QPrinterInfo printerInfo = QPrinterInfo::printerInfo(gSettings->value("DefaultPrinter").toString());
    QPrinter printer(printerInfo, QPrinter::HighResolution);
  #if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    printer.setPageSize(QPageSize::A4);
  #else
    printer.setPageSize(QPrinter::A4);
  #endif
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setColorMode(QPrinter::Color);
    QRectF rect = gSettings->value("PrintMargins", QRectF(5, 10, 5, 15)).toRectF();
    QMarginsF margins(rect.left(), rect.top(), rect.width(), rect.height());
    printer.setPageMargins(margins, QPageLayout::Millimeter);
    gSettings->endGroup();

    QPrintPreviewDialog dlg(&printer, ui->webview);
    connect(&dlg, SIGNAL(paintRequested(QPrinter*)), this, SLOT(printDocument(QPrinter*)));
    dlg.exec();

    if (gSettings->theme() == Settings::Dark)
    {
        mTemplateTags["Style"] = style;
        ui->webview->renderTemplate(mTemplateTags);
        ui->webview->setUpdatesEnabled(true);
    }
  #endif
}

void WdgWebViewEditable::setPdfName(const QString& name)
{
    pdfName = name;
}

void WdgWebViewEditable::printToPdf()
{
  #if defined(QT_PRINTSUPPORT_LIB) && defined(QT_WEBENGINECORE_LIB)
    gSettings->beginGroup("General");
    QString fileName = pdfName.isEmpty() ? htmlName : pdfName;
    QString path = gSettings->value("exportPath", QDir::homePath()).toString();
    QString filePath = QFileDialog::getSaveFileName(this, tr("PDF speichern unter"),
                                     path + "/" + fileName +  ".pdf", "PDF (*.pdf)");
    if (!filePath.isEmpty())
    {
        gSettings->setValue("exportPath", QFileInfo(filePath).absolutePath());
        QRectF rect = gSettings->value("PrintMargins", QRectF(5, 10, 5, 15)).toRectF();
        QMarginsF margins = QMarginsF(rect.left(), rect.top(), rect.width(), rect.height());
        if (gSettings->theme() == Settings::Dark)
        {
            QVariant style = mTemplateTags["Style"];
            mTemplateTags["Style"] = "style_hell.css";
            ui->webview->setUpdatesEnabled(false);

            QEventLoop loop;
            connect(ui->webview, SIGNAL(loadFinished(bool)), &loop, SLOT(quit()));
            ui->webview->renderTemplate(mTemplateTags);
            loop.exec();

            ui->webview->printToPdf(filePath, margins);

            mTemplateTags["Style"] = style;
            ui->webview->renderTemplate(mTemplateTags);
            ui->webview->setUpdatesEnabled(true);
        }
        else
        {
            ui->webview->printToPdf(filePath, margins);
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    }
    gSettings->endGroup();
  #endif
}

bool WdgWebViewEditable::checkSaveTemplate()
{
    if (ui->btnSaveTemplate->isVisible())
    {
        int ret = QMessageBox::question(this, tr("Änderungen speichern?"),
                                        tr("Sollen die Änderungen gespeichert werden?"),
                                        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (ret == QMessageBox::Yes)
            on_btnSaveTemplate_clicked();
        else if (ret == QMessageBox::Cancel)
            return true;
    }
    return false;
}

void WdgWebViewEditable::on_cbEditMode_clicked(bool checked)
{
    updateEditMode();
    ui->splitterEditmode->setHandleWidth(checked ? 5 : 0);
    ui->splitterEditmode->setSizes({1, checked ? 1 : 0});
}

void WdgWebViewEditable::on_cbTemplateAuswahl_currentTextChanged(const QString &fileName)
{
    Q_UNUSED(fileName)
    updateEditMode();
}

void WdgWebViewEditable::on_tbTemplate_textChanged()
{
    if (ui->tbTemplate->hasFocus())
    {
        mTimerWebViewUpdate.start(200);
        ui->btnSaveTemplate->setVisible(true);
    }
}

void WdgWebViewEditable::on_btnSaveTemplate_clicked()
{
    QFile file(ui->btnSaveTemplate->property("file").toString());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    file.write(ui->tbTemplate->toPlainText().toUtf8());
    file.close();
    ui->btnSaveTemplate->setVisible(false);
}

void WdgWebViewEditable::on_btnRestoreTemplate_clicked()
{
    int ret = QMessageBox::question(this, tr("Template wiederherstellen?"),
                                    tr("Soll das Standardtemplate wiederhergestellt werden?"));
    if (ret == QMessageBox::Yes)
    {
        QFile file(gSettings->dataDir(1) + ui->cbTemplateAuswahl->currentText());
        QFile file2(":/data/Webview/" + ui->cbTemplateAuswahl->currentText());
        file.remove();
        if (file2.copy(file.fileName()))
            file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
        ui->btnSaveTemplate->setVisible(false);
        updateEditMode();
    }
}

void WdgWebViewEditable::updateWebView()
{
  #ifdef QT_WEBENGINECORE_LIB
    if (ui->webview->zoomFactor() != gZoomFactor)
    {
        ui->webview->setZoomFactor(gZoomFactor);
        ui->sliderZoom->setValue(gZoomFactor * 100);
        ui->lblZoom->clear();
    }
  #endif
    if (ui->cbEditMode->isChecked())
    {
        if (ui->cbTemplateAuswahl->currentIndex() == 0)
        {
            ui->webview->renderText(ui->tbTemplate->toPlainText(), mTemplateTags);
        }
        else
        {
            mTempCssFile.open();
            mTempCssFile.write(ui->tbTemplate->toPlainText().toUtf8());
            mTempCssFile.flush();
            mTemplateTags["Style"] = mTempCssFile.fileName();
            ui->webview->renderTemplate(mTemplateTags);
        }
    }
    else
    {
        ui->webview->renderTemplate(mTemplateTags);
    }
}

void WdgWebViewEditable::updateTags()
{
    if (ui->tbTags->isVisible())
    {
        QJsonDocument json = QJsonDocument::fromVariant(mTemplateTags);
        ui->tbTags->setPlainText(json.toJson());
    }
}

void WdgWebViewEditable::updateEditMode()
{
    if (checkSaveTemplate())
    {
        ui->cbEditMode->setChecked(true);
        return;
    }

    bool editMode = ui->cbEditMode->isChecked();

    ui->tbTemplate->setVisible(editMode);
    ui->tbTags->setVisible(editMode);
    ui->lblFilePath->setVisible(editMode);
    ui->btnRestoreTemplate->setVisible(editMode);
    ui->cbTemplateAuswahl->setVisible(editMode);
    ui->btnSaveTemplate->setVisible(false);
    if (editMode)
    {
        QFile file(gSettings->dataDir(1) + ui->cbTemplateAuswahl->currentText());
        ui->btnSaveTemplate->setProperty("file", file.fileName());
        ui->lblFilePath->setText("<a href=\"" + file.fileName() + "\">" + file.fileName() + "</a>");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            ui->tbTemplate->setPlainText(file.readAll());
            file.close();
        }
    }

    updateTags();
    updateWebView();
}

void WdgWebViewEditable::on_sliderZoom_valueChanged(int value)
{
    gZoomFactor = value / 100.0;
    ui->lblZoom->setText(QString::number(value)+"%");
  #ifdef QT_WEBENGINECORE_LIB
    ui->webview->setZoomFactor(gZoomFactor);
  #endif
}

void WdgWebViewEditable::on_sliderZoom_sliderReleased()
{
    ui->lblZoom->clear();
}

void WdgWebViewEditable::on_btnPrint_clicked()
{
    printPreview();
}

void WdgWebViewEditable::on_btnPdf_clicked()
{
    printToPdf();
}
