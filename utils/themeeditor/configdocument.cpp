#include "configdocument.h"
#include "ui_configdocument.h"

ConfigDocument::ConfigDocument(QMap<QString, QString>& settings, QString file,
                               QWidget *parent)
                                   : TabContent(parent),
                                   ui(new Ui::ConfigDocument),
                                   filePath(file)
{
    ui->setupUi(this);

    QMap<QString, QString>::iterator i;
    for(i = settings.begin(); i != settings.end(); i++)
        addRow(i.key(), i.value());

    saved = toPlainText();

    QObject::connect(ui->addKeyButton, SIGNAL(pressed()),
                     this, SLOT(addClicked()));
}

ConfigDocument::~ConfigDocument()
{
    delete ui;
}

void ConfigDocument::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

QString ConfigDocument::title() const
{
    QStringList decompose = filePath.split("/");
    return decompose.last();
}

void ConfigDocument::save()
{

}

void ConfigDocument::saveAs()
{

}

bool ConfigDocument::requestClose()
{

}

QString ConfigDocument::toPlainText() const
{
    QString buffer = "";

    for(int i = 0; i < keys.count(); i++)
    {
        buffer += keys[i]->text();
        buffer += ":";
        buffer += values[i]->text();
        buffer += "\n";
    }

    return buffer;
}

void ConfigDocument::addRow(QString key, QString value)
{
    QHBoxLayout* layout = new QHBoxLayout();
    QLineEdit* keyEdit = new QLineEdit(key, this);
    QLineEdit* valueEdit = new QLineEdit(value, this);
    QPushButton* delButton = new QPushButton(tr("-"), this);

    layout->addWidget(keyEdit);
    layout->addWidget(valueEdit);
    layout->addWidget(delButton);

    delButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    delButton->setMaximumWidth(35);

    QObject::connect(delButton, SIGNAL(clicked()),
                     this, SLOT(deleteClicked()));

    ui->configBoxes->addLayout(layout);

    containers.append(layout);
    keys.append(keyEdit);
    values.append(valueEdit);
    deleteButtons.append(delButton);

}

void ConfigDocument::deleteClicked()
{
    QPushButton* button = dynamic_cast<QPushButton*>(sender());
    int row = deleteButtons.indexOf(button);

    deleteButtons[row]->deleteLater();
    keys[row]->deleteLater();
    values[row]->deleteLater();
    containers[row]->deleteLater();

    deleteButtons.removeAt(row);
    keys.removeAt(row);
    values.removeAt(row);
    containers.removeAt(row);

    if(saved != toPlainText())
        emit titleChanged(title() + "*");
    else
        emit titleChanged(title());
}

void ConfigDocument::addClicked()
{
    addRow(tr("Key"), tr("Value"));
}
