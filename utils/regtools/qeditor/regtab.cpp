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
#include <QDebug>
#include "backend.h"
#include "analyser.h"

SocFieldValidator::SocFieldValidator(QObject *parent)
    :QValidator(parent)
{
    m_field.first_bit = 0;
    m_field.last_bit = 31;
}

SocFieldValidator::SocFieldValidator(const soc_reg_field_t& field, QObject *parent)
    :QValidator(parent), m_field(field)
{
}

void SocFieldValidator::fixup(QString& input) const
{
    input = input.trimmed();
}

QValidator::State SocFieldValidator::validate(QString& input, int& pos) const
{
    (void) pos;
    soc_word_t val;
    State state = parse(input, val);
    qDebug() << "validate(" << input << "): " << state;
    return state;
}

QValidator::State SocFieldValidator::parse(const QString& input, soc_word_t& val) const
{
    // the empty string is all alwats intermediate
    if(input.size() == 0)
        return Intermediate;
    // first check named values
    State state = Invalid;
    foreach(const soc_reg_field_value_t& value, m_field.value)
    {
        QString name = QString::fromLocal8Bit(value.name.c_str());
        // cannot be a substring if too long or empty
        if(input.size() > name.size())
            continue;
        // check equal string
        if(input == name)
        {
            state = Acceptable;
            val = value.value;
            break;
        }
        // check substring
        if(name.startsWith(input))
            state = Intermediate;
    }
    // early return for exact match
    if(state == Acceptable)
        return state;
    // do a few special cases for convenience
    if(input.compare("0x", Qt::CaseInsensitive) == 0 ||
            input.compare("0b", Qt::CaseInsensitive) == 0)
        return Intermediate;
    // try by parsing
    unsigned basis, pos;
    if(input.size() >= 2 && input.startsWith("0x", Qt::CaseInsensitive))
    {
        basis = 16;
        pos = 2;
    }
    else if(input.size() >= 2 && input.startsWith("0b", Qt::CaseInsensitive))
    {
        basis = 2;
        pos = 2;
    }
    else if(input.size() >= 2 && input.startsWith("0"))
    {
        basis = 8;
        pos = 1;
    }
    else
    {
        basis = 10;
        pos = 0;
    }
    bool ok = false;
    unsigned long v = input.mid(pos).toULong(&ok, basis);
    // if not ok, return result of name parsing
    if(!ok)
        return state;
    // if ok, check if it fits in the number of bits
    unsigned nr_bits = m_field.last_bit - m_field.first_bit + 1;
    unsigned long max = nr_bits == 32 ? 0xffffffff : (1 << nr_bits) - 1;
    if(v <= max)
    {
        val = v;
        return Acceptable;
    }

    return state;
}

RegTab::RegTab(Backend *backend)
    :m_backend(backend)
{
    QWidget *left = new QWidget;
    this->addWidget(left);
    this->setStretchFactor(0, 1);
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
#ifdef HAVE_HWSTUB
    m_data_selector->addItem(QIcon::fromTheme("multimedia-player"), "Device...", QVariant(DataSelDevice));
#endif
    m_data_sel_edit = new QLineEdit;
    m_data_sel_edit->setReadOnly(true);
    m_data_soc_label = new QLabel;
    QPushButton *data_sel_reload = new QPushButton;
    data_sel_reload->setIcon(QIcon::fromTheme("view-refresh"));
    data_sel_layout->addWidget(m_data_selector);
    data_sel_layout->addWidget(m_data_sel_edit);
#ifdef HAVE_HWSTUB
    m_dev_selector = new QComboBox;
    data_sel_layout->addWidget(m_dev_selector, 1);
#endif
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
    this->addWidget(w);
    this->setStretchFactor(1, 2);

    m_io_backend = m_backend->CreateDummyIoBackend();

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
#ifdef HAVE_HWSTUB
    connect(m_dev_selector, SIGNAL(currentIndexChanged(int)),
        this, SLOT(OnDevChanged(int)));
#endif

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
        m_data_sel_edit->show();
#ifdef HAVE_HWSTUB
        m_dev_selector->hide();
#endif
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
        SetReadOnlyIndicator();
    }
#ifdef HAVE_HWSTUB
    else if(var == DataSelDevice)
    {
        m_data_sel_edit->hide();
        m_dev_selector->show();
        OnDevListChanged();
    }
#endif
    else
    {
        m_data_sel_edit->show();
#ifdef HAVE_HWSTUB
        m_dev_selector->hide();
#endif
        delete m_io_backend;
        m_io_backend = m_backend->CreateDummyIoBackend();
        SetDataSocName("");
    }
    OnDataChanged();
}

void RegTab::SetReadOnlyIndicator()
{
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
    if(current == 0 || current->type() != RegTreeRegType)
        return;
    RegTreeItem *item = dynamic_cast< RegTreeItem * >(current);

    DisplayRegister(item->GetRef());
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
    m_right_panel->addWidget(m_right_content, 1);
}

void RegTab::DisplayRegister(const SocRegRef& ref)
{
    delete m_right_content;

    bool read_only = m_io_backend->IsReadOnly();

    QVBoxLayout *right_layout = new QVBoxLayout;

    const soc_dev_addr_t& dev_addr = ref.GetDevAddr();
    const soc_reg_t& reg = ref.GetReg();
    const soc_reg_addr_t& reg_addr = ref.GetRegAddr();

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
    BackendHelper helper(m_io_backend, m_cur_soc);
    bool has_value = helper.ReadRegister(dev_addr.name.c_str(), reg_addr.name.c_str(), value);

    QHBoxLayout *raw_val_layout = 0;
    if(has_value)
    {
        QLabel *raw_val_name = new QLabel;
        raw_val_name->setText("Raw value:");
        QLineEdit *raw_val_edit = new QLineEdit;
        raw_val_edit->setReadOnly(read_only);
        raw_val_edit->setText(QString().sprintf("0x%08x", value));
        raw_val_edit->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        raw_val_edit->setValidator(new SocFieldValidator(raw_val_edit));
        connect(raw_val_edit, SIGNAL(returnPressed()), this, SLOT(OnRawRegValueReturnPressed()));
        raw_val_layout = new QHBoxLayout;
        raw_val_layout->addStretch();
        raw_val_layout->addWidget(raw_val_name);
        raw_val_layout->addWidget(raw_val_edit);
        raw_val_layout->addStretch();
    }

    QTableWidget *value_table = new QTableWidget;
    value_table->setRowCount(reg.field.size());
    value_table->setColumnCount(4);
    int row = 0;
    foreach(const soc_reg_field_t& field, reg.field)
    {
        QString bits_str;
        if(field.first_bit == field.last_bit)
            bits_str.sprintf("%d", field.first_bit);
        else
            bits_str.sprintf("%d:%d", field.last_bit, field.first_bit);
        QTableWidgetItem *item = new QTableWidgetItem(bits_str);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        value_table->setItem(row, 0, item);
        item = new QTableWidgetItem(QString(field.name.c_str()));
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        value_table->setItem(row, 1, item);
        item = new QTableWidgetItem();
        if(has_value)
        {
            soc_word_t v = (value & field.bitmask()) >> field.first_bit;
            QString value_name;
            foreach(const soc_reg_field_value_t& rval, field.value)
                if(v == rval.value)
                    value_name = rval.name.c_str();
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
                value_table->setItem(row, 3, t);
            }
        }
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        value_table->setItem(row, 2, item);
        row++;
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

#ifdef HAVE_HWSTUB
void RegTab::OnDevListChanged()
{
    m_dev_selector->clear();
    QList< HWStubDevice* > list = m_hwstub_helper.GetDevList();
    foreach(HWStubDevice *dev, list)
    {
        QString name = QString("Bus %1 Device %2: %3").arg(dev->GetBusNumber())
            .arg(dev->GetDevAddress()).arg(dev->GetTargetInfo().bName);
        m_dev_selector->addItem(QIcon::fromTheme("multimedia-player"), name,
            QVariant::fromValue((void *)dev));
    }
    if(list.size() > 0)
        m_dev_selector->setCurrentIndex(0);
    else
        SetDataSocName("");
}

void RegTab::OnDevChanged(int index)
{
    if(index == -1)
        return;
    HWStubDevice *dev = reinterpret_cast< HWStubDevice* >(m_dev_selector->itemData(index).value< void* >());
    delete m_io_backend;
    m_io_backend = m_backend->CreateHWStubIoBackend(dev);
    SetDataSocName(m_io_backend->GetSocName());
    OnDataSocActivated(m_io_backend->GetSocName());
    OnDataChanged();
}
#endif

void RegTab::FillDevSubTree(DevTreeItem *item)
{
    const soc_dev_t& dev = item->GetRef().GetDev();
    for(size_t i = 0; i < dev.reg.size(); i++)
    {
        const soc_reg_t& reg = dev.reg[i];
        for(size_t j = 0; j < reg.addr.size(); j++)
        {
            RegTreeItem *reg_item = new RegTreeItem(reg.addr[j].name.c_str(),
                SocRegRef(item->GetRef(), i, j));
            item->addChild(reg_item);
        }
    }
}

void RegTab::FillRegTree()
{
    for(size_t i = 0; i < m_cur_soc.GetSoc().dev.size(); i++)
    {
        const soc_dev_t& dev = m_cur_soc.GetSoc().dev[i];
        for(size_t j = 0; j < dev.addr.size(); j++)
        {
            DevTreeItem *dev_item = new DevTreeItem(dev.addr[j].name.c_str(),
                SocDevRef(m_cur_soc, i, j));
            FillDevSubTree(dev_item);
            m_reg_tree->addTopLevelItem(dev_item);
        }
    }
}

void RegTab::FillAnalyserList()
{
    m_analysers_list->clear();
    m_analysers_list->addItems(AnalyserFactory::GetAnalysersForSoc(m_cur_soc.GetSoc().name.c_str()));
}

void RegTab::OnSocChanged(const QString& soc)
{
    m_reg_tree->clear();
    if(!m_backend->GetSocByName(soc, m_cur_soc))
        return;
    FillRegTree();
    FillAnalyserList();
}

void RegTab::OnRawRegValueReturnPressed()
{
    QObject *obj = sender();
    QLineEdit *edit = dynamic_cast< QLineEdit* >(obj);
    const SocFieldValidator *validator = dynamic_cast< const SocFieldValidator* >(edit->validator());
    soc_word_t val;
    QValidator::State state = validator->parse(edit->text(), val);
}
