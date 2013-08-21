#include "regtab.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QAbstractListModel>
#include <QMessageBox>
#include <QSizePolicy>
#include <QHBoxLayout>
#include <QStringBuilder>
#include <QLabel>
#include <QGridLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QFileDialog>
#include "backend.h"
#include "analyser.h"

RegTreeItem::RegTreeItem(const QString& string, int type)
    :QTreeWidgetItem(QStringList(string), type)
{
}

void RegTreeItem::SetPath(int dev_idx, int dev_addr_idx, int reg_idx, int reg_addr_idx)
{
    m_dev_idx = dev_idx;
    m_dev_addr_idx = dev_addr_idx;
    m_reg_idx = reg_idx;
    m_reg_addr_idx = reg_addr_idx;
}

RegTab::RegTab(Backend *backend, QTabWidget *parent)
    :m_backend(backend)
{
    m_splitter = new QSplitter();
    QWidget *left = new QWidget;
    m_splitter->addWidget(left);
    QVBoxLayout *left_layout = new QVBoxLayout;
    left->setLayout(left_layout);

    QGroupBox *top_group = new QGroupBox("SOC selection");
    QVBoxLayout *top_group_layout = new QVBoxLayout;
    m_soc_selector = new QComboBox;
    top_group_layout->addWidget(m_soc_selector);
    top_group->setLayout(top_group_layout);

    m_reg_tree = new QTreeWidget();
    m_reg_tree->setColumnCount(1);
    m_reg_tree->setHeaderLabel(QString("Name"));

    m_analysers_list = new QListWidget;

    m_type_selector = new QTabWidget;
    m_type_selector->addTab(m_reg_tree, "Registers");
    m_type_selector->addTab(m_analysers_list, "Analyzers");
    m_type_selector->setTabPosition(QTabWidget::West);

    left_layout->addWidget(top_group);
    left_layout->addWidget(m_type_selector);

    m_right_panel = new QVBoxLayout;
    QGroupBox *data_sel_group = new QGroupBox("Data selection");
    QHBoxLayout *data_sel_layout = new QHBoxLayout;
    m_data_selector = new QComboBox;
    m_data_selector->addItem(QIcon::fromTheme("face-sad"), "None", QVariant(DataSelNothing));
    m_data_selector->addItem(QIcon::fromTheme("document-open"), "File...", QVariant(DataSelFile));
    m_data_sel_edit = new QLineEdit;
    m_data_sel_edit->setReadOnly(true);
    m_data_soc_label = new QLabel;
    QPushButton *data_sel_reload = new QPushButton;
    data_sel_reload->setIcon(QIcon::fromTheme("view-refresh"));
    data_sel_layout->addWidget(m_data_selector);
    data_sel_layout->addWidget(m_data_sel_edit);
    data_sel_layout->addWidget(m_data_soc_label);
    data_sel_layout->addWidget(data_sel_reload);
    data_sel_group->setLayout(data_sel_layout);
    m_data_soc_label->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

    m_right_panel->addWidget(data_sel_group);
    m_right_content = new QWidget;
    QVBoxLayout *l = new QVBoxLayout;
    l->addStretch();
    m_right_content->setLayout(l);
    m_right_panel->addWidget(m_right_content);
    QWidget *w = new QWidget;
    w->setLayout(m_right_panel);
    m_splitter->addWidget(w);

    m_io_backend = m_backend->CreateDummyIoBackend();

    parent->addTab(m_splitter, "Register Tab");

    connect(m_soc_selector, SIGNAL(currentIndexChanged(const QString&)),
        this, SLOT(OnSocChanged(const QString&)));
    connect(m_backend, SIGNAL(OnSocListChanged()), this, SLOT(OnSocListChanged()));
    connect(m_reg_tree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
        this, SLOT(OnRegItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(m_reg_tree, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this,
        SLOT(OnRegItemClicked(QTreeWidgetItem *, int)));
    connect(m_data_selector, SIGNAL(activated(int)),
        this, SLOT(OnDataSelChanged(int)));
    connect(m_data_soc_label, SIGNAL(linkActivated(const QString&)), this,
        SLOT(OnDataSocActivated(const QString&)));
    connect(m_analysers_list, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
        this, SLOT(OnAnalyserChanged(QListWidgetItem *, QListWidgetItem *)));
    connect(m_analysers_list, SIGNAL(itemClicked(QListWidgetItem *)), this,
        SLOT(OnAnalyserClicked(QListWidgetItem *)));

    OnSocListChanged();
    OnDataSelChanged(DataSelNothing);
}

void RegTab::SetDataSocName(const QString& socname)
{
    if(socname.size() != 0)
    {
        m_data_soc_label->setText("<a href=\"" + socname + "\">" + socname + "</a>");
        m_data_soc_label->setTextFormat(Qt::RichText);
        m_data_soc_label->show();
    }
    else
    {
        m_data_soc_label->setText("");
        m_data_soc_label->hide();
    }
}

void RegTab::OnDataSocActivated(const QString& str)
{
    int index = m_soc_selector->findText(str);
    if(index != -1)
        m_soc_selector->setCurrentIndex(index);
}

void RegTab::OnDataSelChanged(int index)
{
    if(index == -1)
        return;
    QVariant var = m_data_selector->itemData(index);
    if(var == DataSelFile)
    {
        QFileDialog *fd = new QFileDialog(m_data_selector);
        fd->setFilter("Textual files (*.txt);;All files (*)");
        fd->setDirectory(Settings::Get()->value("regtab/loaddatadir", QDir::currentPath()).toString());
        if(fd->exec())
        {
            QStringList filenames = fd->selectedFiles();
            delete m_io_backend;
            m_io_backend = m_backend->CreateFileIoBackend(filenames[0]);
            m_data_sel_edit->setText(filenames[0]);
            SetDataSocName(m_io_backend->GetSocName());
            OnDataSocActivated(m_io_backend->GetSocName());
        }
        Settings::Get()->setValue("regtab/loaddatadir", fd->directory().absolutePath());
    }
    else
    {
        delete m_io_backend;
        m_io_backend = m_backend->CreateDummyIoBackend();
        SetDataSocName("");
    }
    OnDataChanged();
}

void RegTab::OnDataChanged()
{
    OnRegItemChanged(m_reg_tree->currentItem(), m_reg_tree->currentItem());
}

void RegTab::OnRegItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    (void) previous;
    OnRegItemClicked(current, 0);
}

void RegTab::OnRegItemClicked(QTreeWidgetItem *current, int col)
{
    (void) col;
    if(current == 0)
        return;
    RegTreeItem *item = dynamic_cast< RegTreeItem * >(current);
    if(item->type() != RegTreeRegType)
        return;
    soc_dev_t& dev = m_cur_soc.dev[item->GetDevIndex()];
    soc_dev_addr_t& dev_addr = dev.addr[item->GetDevAddrIndex()];
    soc_reg_t& reg = dev.reg[item->GetRegIndex()];
    soc_reg_addr_t& reg_addr = reg.addr[item->GetRegAddrIndex()];

    DisplayRegister(dev, dev_addr, reg, reg_addr);
}

void RegTab::OnAnalyserChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    (void) previous;
    OnAnalyserClicked(current);
}

void RegTab::OnAnalyserClicked(QListWidgetItem *current)
{
    if(current == 0)
        return;
    delete m_right_content;
    AnalyserFactory *ana = AnalyserFactory::GetAnalyserByName(current->text());
    m_right_content = ana->Create(m_cur_soc, m_io_backend)->GetWidget();
    m_right_panel->addWidget(m_right_content);
}

void RegTab::DisplayRegister(soc_dev_t& dev, soc_dev_addr_t& dev_addr,
    soc_reg_t& reg, soc_reg_addr_t& reg_addr)
{
    (void) dev;
    delete m_right_content;

    QVBoxLayout *right_layout = new QVBoxLayout;

    QString reg_name;
    reg_name.sprintf("HW_%s_%s", dev_addr.name.c_str(), reg_addr.name.c_str());
    QStringList names;
    QVector< soc_addr_t > addresses;
    names.append(reg_name);
    addresses.append(reg_addr.addr);
    if(reg.flags & REG_HAS_SCT)
    {
        names.append(reg_name + "_SET");
        names.append(reg_name + "_CLR");
        names.append(reg_name + "_TOG");
        addresses.append(reg_addr.addr + 4);
        addresses.append(reg_addr.addr + 8);
        addresses.append(reg_addr.addr + 12);
    }

    QString str;
    str += "<table align=left>";
    for(int i = 0; i < names.size(); i++)
        str += "<tr><td><b>" + names[i] + "</b></td></tr>";
    str += "</table>";
    QLabel *label_names = new QLabel;
    label_names->setTextFormat(Qt::RichText);
    label_names->setText(str);

    QString str_addr;
    str_addr += "<table align=left>";
    for(int i = 0; i < names.size(); i++)
        str_addr += "<tr><td><b>" + QString().sprintf("0x%03x", addresses[i]) + "</b></td></tr>";
    str_addr += "</table>";
    QLabel *label_addr = new QLabel;
    label_addr->setTextFormat(Qt::RichText);
    label_addr->setText(str_addr);

    QHBoxLayout *top_layout = new QHBoxLayout;
    top_layout->addStretch();
    top_layout->addWidget(label_names);
    top_layout->addWidget(label_addr);
    top_layout->addStretch();

    soc_word_t value;
    bool has_value = m_io_backend->ReadRegister(QString().sprintf("HW.%s.%s",
        dev_addr.name.c_str(), reg_addr.name.c_str()), value);

    QHBoxLayout *raw_val_layout = 0;
    if(has_value)
    {
        QLabel *raw_val_name = new QLabel;
        raw_val_name->setText("Raw value:");
        QLineEdit *raw_val_edit = new QLineEdit;
        raw_val_edit->setReadOnly(true);
        raw_val_edit->setText(QString().sprintf("0x%08x", value));
        raw_val_edit->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        raw_val_layout = new QHBoxLayout;
        raw_val_layout->addStretch();
        raw_val_layout->addWidget(raw_val_name);
        raw_val_layout->addWidget(raw_val_edit);
        raw_val_layout->addStretch();
    }

    QTableWidget *value_table = new QTableWidget;
    value_table->setRowCount(reg.field.size());
    value_table->setColumnCount(4);
    for(size_t i = 0; i < reg.field.size(); i++)
    {
        QString bits_str;
        if(reg.field[i].first_bit == reg.field[i].last_bit)
            bits_str.sprintf("%d", reg.field[i].first_bit);
        else
            bits_str.sprintf("%d:%d", reg.field[i].last_bit, reg.field[i].first_bit);
        QTableWidgetItem *item = new QTableWidgetItem(bits_str);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        value_table->setItem(i, 0, item);
        item = new QTableWidgetItem(QString(reg.field[i].name.c_str()));
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        value_table->setItem(i, 1, item);
        item = new QTableWidgetItem();
        if(has_value)
        {
            const soc_reg_field_t& field = reg.field[i];
            soc_word_t v = (value & field.bitmask()) >> field.first_bit;
            QString value_name;
            for(size_t j = 0; j < field.value.size(); j++)
                if(v == field.value[j].value)
                    value_name = field.value[j].name.c_str();
            const char *fmt = "%lu";
            // heuristic
            if((field.last_bit - field.first_bit + 1) > 16)
                fmt = "0x%lx";
            item->setText(QString().sprintf(fmt, (unsigned long)v));
            item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

            if(value_name.size() != 0)
            {
                QTableWidgetItem *t = new QTableWidgetItem(value_name);
                t->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
                t->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                value_table->setItem(i, 3, t);
            }
        }
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        value_table->setItem(i, 2, item);
    }
    value_table->setHorizontalHeaderItem(0, new QTableWidgetItem("Bits"));
    value_table->setHorizontalHeaderItem(1, new QTableWidgetItem("Name"));
    value_table->setHorizontalHeaderItem(2, new QTableWidgetItem("Value"));
    value_table->setHorizontalHeaderItem(3, new QTableWidgetItem("Meaning"));
    value_table->verticalHeader()->setVisible(false);
    value_table->resizeColumnsToContents();
    value_table->horizontalHeader()->setStretchLastSection(true);
    value_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    right_layout->addLayout(top_layout);
    if(raw_val_layout)
        right_layout->addLayout(raw_val_layout);
    //right_layout->addWidget(bits_label);
    right_layout->addWidget(value_table);
    //right_layout->addStretch();

    m_right_content = new QGroupBox("Register Description");
    m_right_content->setLayout(right_layout);
    m_right_panel->addWidget(m_right_content);
}

void RegTab::OnSocListChanged()
{
    m_soc_selector->clear();
    QStringList socs = m_backend->GetSocNameList();
    for(int i = 0; i < socs.size(); i++)
        m_soc_selector->addItem(socs[i]);
}

void RegTab::FillDevSubTree(RegTreeItem *item)
{
    soc_dev_t& sd = m_cur_soc.dev[item->GetDevIndex()];
    for(size_t i = 0; i < sd.reg.size(); i++)
    {
        soc_reg_t& reg = sd.reg[i];
        for(size_t j = 0; j < reg.addr.size(); j++)
        {
            RegTreeItem *reg_item = new RegTreeItem(reg.addr[j].name.c_str(), RegTreeRegType);
            reg_item->SetPath(item->GetDevIndex(), item->GetDevAddrIndex(), i, j);
            item->addChild(reg_item);
        }
    }
}

void RegTab::FillRegTree()
{
    for(size_t i = 0; i < m_cur_soc.dev.size(); i++)
    {
        soc_dev_t& sd = m_cur_soc.dev[i];
        for(size_t j = 0; j < sd.addr.size(); j++)
        {
            RegTreeItem *dev_item = new RegTreeItem(sd.addr[j].name.c_str(), RegTreeDevType);
            dev_item->SetPath(i, j);
            FillDevSubTree(dev_item);
            m_reg_tree->addTopLevelItem(dev_item);
        }
    }
}

void RegTab::FillAnalyserList()
{
    m_analysers_list->clear();
    m_analysers_list->addItems(AnalyserFactory::GetAnalysersForSoc(m_cur_soc.name.c_str()));
}

void RegTab::OnSocChanged(const QString& soc)
{
    m_reg_tree->clear();
    if(!m_backend->GetSocByName(soc, m_cur_soc))
        return;
    FillRegTree();
    FillAnalyserList();
}
