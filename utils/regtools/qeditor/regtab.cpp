#include "regtab.h"

#include <QSizePolicy>
#include <QStringBuilder>
#include <QFileDialog>
#include <QDebug>
#include <QStyle>
#include <QMessageBox>
#include "backend.h"
#include "analyser.h"
#include "regdisplaypanel.h"

namespace
{

enum
{
    RegTreeDevType = QTreeWidgetItem::UserType,
    RegTreeRegType
};

class DevTreeItem : public QTreeWidgetItem
{
public:
    DevTreeItem(const QString& string, const SocDevRef& ref)
        :QTreeWidgetItem(QStringList(string), RegTreeDevType), m_ref(ref) {}

    const SocDevRef& GetRef() { return m_ref; }
private:
    SocDevRef m_ref;
};

class RegTreeItem : public QTreeWidgetItem
{
public:
    RegTreeItem(const QString& string, const SocRegRef& ref)
        :QTreeWidgetItem(QStringList(string), RegTreeRegType), m_ref(ref) {}

    const SocRegRef& GetRef() { return m_ref; }
private:
    SocRegRef m_ref;
};

}

/**
 * EmptyRegTabPanel
 */
EmptyRegTabPanel::EmptyRegTabPanel(QWidget *parent)
    :QWidget(parent)
{
    QVBoxLayout *l = new QVBoxLayout;
    l->addStretch();
    setLayout(l);
}

void EmptyRegTabPanel::AllowWrite(bool en)
{
    Q_UNUSED(en);
}

QWidget *EmptyRegTabPanel::GetWidget()
{
    return this;
}


/**
 * RegTab
 */

RegTab::RegTab(Backend *backend, QWidget *parent)
    :QSplitter(parent), m_backend(backend)
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
    m_data_selector->addItem(QIcon::fromTheme("text-x-generic"), "Explore", QVariant(DataSelNothing));
    m_data_selector->addItem(QIcon::fromTheme("document-open"), "File...", QVariant(DataSelFile));
#ifdef HAVE_HWSTUB
    m_data_selector->addItem(QIcon::fromTheme("multimedia-player"), "Device...", QVariant(DataSelDevice));
#endif
    m_data_sel_edit = new QLineEdit;
    m_data_sel_edit->setReadOnly(true);
    m_readonly_check = new QCheckBox("Read-only");
    m_readonly_check->setCheckState(Qt::Checked);
    m_data_soc_label = new QLabel;
    m_dump = new QPushButton("Dump", this);
    m_dump->setIcon(QIcon::fromTheme("system-run"));
    m_data_sel_reload = new QPushButton(this);
    m_data_sel_reload->setIcon(QIcon::fromTheme("view-refresh"));
    m_data_sel_reload->setToolTip("Reload data");
    data_sel_layout->addWidget(m_data_selector);
    data_sel_layout->addWidget(m_data_sel_edit, 1);
    data_sel_layout->addStretch(0);
#ifdef HAVE_HWSTUB
    m_dev_selector = new QComboBox;
    data_sel_layout->addWidget(m_dev_selector, 1);
#endif
    data_sel_layout->addWidget(m_readonly_check);
    data_sel_layout->addWidget(m_data_soc_label);
    data_sel_layout->addWidget(m_dump);
    data_sel_layout->addWidget(m_data_sel_reload);
    data_sel_group->setLayout(data_sel_layout);
    m_data_soc_label->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

    m_right_panel->addWidget(data_sel_group, 0);
    m_right_content = 0;
    SetPanel(new EmptyRegTabPanel);
    QWidget *w = new QWidget;
    w->setLayout(m_right_panel);
    this->addWidget(w);
    this->setStretchFactor(1, 2);

    m_io_backend = m_backend->CreateDummyIoBackend();

    connect(m_soc_selector, SIGNAL(currentIndexChanged(int)),
        this, SLOT(OnSocChanged(int)));
    connect(m_backend, SIGNAL(OnSocListChanged()), this, SLOT(OnSocListChanged()));
    connect(m_reg_tree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
        this, SLOT(OnRegItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(m_data_selector, SIGNAL(activated(int)),
        this, SLOT(OnDataSelChanged(int)));
    connect(m_data_soc_label, SIGNAL(linkActivated(const QString&)), this,
        SLOT(OnDataSocActivated(const QString&)));
    connect(m_analysers_list, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
        this, SLOT(OnAnalyserChanged(QListWidgetItem *, QListWidgetItem *)));
#ifdef HAVE_HWSTUB
    connect(m_dev_selector, SIGNAL(currentIndexChanged(int)),
        this, SLOT(OnDevChanged(int)));
#endif
    connect(m_readonly_check, SIGNAL(clicked(bool)), this, SLOT(OnReadOnlyClicked(bool)));
    connect(m_dump, SIGNAL(clicked(bool)), this, SLOT(OnDumpRegs(bool)));

    OnSocListChanged();
    OnDataSelChanged(0);
}

RegTab::~RegTab()
{
#ifdef HAVE_HWSTUB
    ClearDevList();
#endif
    delete m_io_backend;
}

bool RegTab::Quit()
{
    return true;
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
        m_readonly_check->show();
        m_data_sel_reload->show();
        m_dump->hide();
        QFileDialog *fd = new QFileDialog(m_data_selector);
        fd->setFilter("Textual files (*.txt);;All files (*)");
        fd->setDirectory(Settings::Get()->value("loaddatadir", QDir::currentPath()).toString());
        if(fd->exec())
        {
            QStringList filenames = fd->selectedFiles();
            delete m_io_backend;
            m_io_backend = m_backend->CreateFileIoBackend(filenames[0]);
            m_data_sel_edit->setText(filenames[0]);
            SetDataSocName(m_io_backend->GetSocName());
            OnDataSocActivated(m_io_backend->GetSocName());
        }
        Settings::Get()->setValue("loaddatadir", fd->directory().absolutePath());
        SetReadOnlyIndicator();
    }
#ifdef HAVE_HWSTUB
    else if(var == DataSelDevice)
    {
        m_data_sel_edit->hide();
        m_readonly_check->show();
        m_dev_selector->show();
        m_data_sel_reload->hide();
        m_dump->show();
        OnDevListChanged();
    }
#endif
    else
    {
        m_data_sel_edit->show();
#ifdef HAVE_HWSTUB
        m_dev_selector->hide();
#endif
        m_readonly_check->hide();
        m_data_sel_reload->hide();
        m_dump->hide();

        delete m_io_backend;
        m_io_backend = m_backend->CreateDummyIoBackend();
        m_readonly_check->setCheckState(Qt::Checked);
        SetDataSocName("");
        UpdateSocFilename();
    }
    OnDataChanged();
}

void RegTab::UpdateSocFilename()
{
    int index = m_data_selector->currentIndex();
    if(index == -1)
        return;
    if(m_data_selector->itemData(index) != DataSelNothing)
        return;
    index = m_soc_selector->currentIndex();
    if(index == -1)
        return;
    SocRef ref = m_soc_selector->itemData(index).value< SocRef >();
    m_data_sel_edit->setText(ref.GetSocFile()->GetFilename());
}

void RegTab::SetReadOnlyIndicator()
{
    if(m_io_backend->IsReadOnly())
        m_readonly_check->setCheckState(Qt::Checked);
}

void RegTab::OnDataChanged()
{
    OnRegItemChanged(m_reg_tree->currentItem(), m_reg_tree->currentItem());
}

void RegTab::OnRegItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous);
    OnRegItemClicked(current, 0);
}

void RegTab::OnRegItemClicked(QTreeWidgetItem *current, int col)
{
    Q_UNUSED(col);
    if(current == 0)
        return;
    if(current->type() == RegTreeRegType)
    {
        RegTreeItem *item = dynamic_cast< RegTreeItem * >(current);
        DisplayRegister(item->GetRef());
    }
    else if(current->type() == RegTreeDevType)
    {
        DevTreeItem *item = dynamic_cast< DevTreeItem * >(current);
        DisplayDevice(item->GetRef());
    }
}

void RegTab::OnAnalyserChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);
    OnAnalyserClicked(current);
}

void RegTab::OnAnalyserClicked(QListWidgetItem *current)
{
    if(current == 0)
        return;
    AnalyserFactory *ana = AnalyserFactory::GetAnalyserByName(current->text());
    SetPanel(ana->Create(m_cur_soc, m_io_backend));
}

void RegTab::DisplayRegister(const SocRegRef& ref)
{
    SetPanel(new RegDisplayPanel(this, m_io_backend, ref));
}

void RegTab::DisplayDevice(const SocDevRef& ref)
{
    SetPanel(new DevDisplayPanel(this, ref));
}

void RegTab::SetPanel(RegTabPanel *panel)
{
    delete m_right_content;
    m_right_content = panel;
    m_right_content->AllowWrite(m_readonly_check->checkState() == Qt::Unchecked);
    m_right_panel->addWidget(m_right_content->GetWidget(), 1);
}

void RegTab::OnSocListChanged()
{
    m_soc_selector->clear();
    QList< SocRef > socs = m_backend->GetSocList();
    for(int i = 0; i < socs.size(); i++)
    {
        QVariant v;
        v.setValue(socs[i]);
        m_soc_selector->addItem(QString::fromStdString(socs[i].GetSoc().name), v);
    }
}

#ifdef HAVE_HWSTUB
void RegTab::OnDevListChanged()
{
    ClearDevList();
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
    SetReadOnlyIndicator();
}

void RegTab::OnDevChanged(int index)
{
    if(index == -1)
        return;
    HWStubDevice *dev = reinterpret_cast< HWStubDevice* >(m_dev_selector->itemData(index).value< void* >());
    delete m_io_backend;
    /* NOTE: make a copy of the HWStubDevice device because the one in the list
     * might get destroyed when clearing the list while the backend is still
     * active: this would result in a double free when the backend is also destroyed */
    m_io_backend = m_backend->CreateHWStubIoBackend(new HWStubDevice(dev));
    SetDataSocName(m_io_backend->GetSocName());
    OnDataSocActivated(m_io_backend->GetSocName());
    OnDataChanged();
}

void RegTab::ClearDevList()
{
    while(m_dev_selector->count() > 0)
    {
        HWStubDevice *dev = reinterpret_cast< HWStubDevice* >(m_dev_selector->itemData(0).value< void* >());
        delete dev;
        m_dev_selector->removeItem(0);
    }
}
#endif

void RegTab::FillDevSubTree(QTreeWidgetItem *_item)
{
    DevTreeItem *item = dynamic_cast< DevTreeItem* >(_item);
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

void RegTab::OnSocChanged(int index)
{
    if(index == -1)
        return;
    m_reg_tree->clear();
    m_cur_soc = m_soc_selector->itemData(index).value< SocRef >();
    FillRegTree();
    FillAnalyserList();
    UpdateSocFilename();
}

void RegTab::OnReadOnlyClicked(bool checked)
{
    if(m_io_backend->IsReadOnly())
        return SetReadOnlyIndicator();
    m_right_content->AllowWrite(!checked);
    UpdateSocFilename();
}

void RegTab::OnDumpRegs(bool c)
{
    Q_UNUSED(c);
    QFileDialog *fd = new QFileDialog(this);
    fd->setAcceptMode(QFileDialog::AcceptSave);
    fd->setFilter("Textual files (*.txt);;All files (*)");
    fd->setDirectory(Settings::Get()->value("loaddatadir", QDir::currentPath()).toString());
    if(!fd->exec())
        return;
    QStringList filenames = fd->selectedFiles();
    Settings::Get()->setValue("loaddatadir", fd->directory().absolutePath());
    BackendHelper bh(m_io_backend, m_cur_soc);
    if(!bh.DumpAllRegisters(filenames[0]))
    {
        QMessageBox::warning(this, "The register dump was not saved",
            "There was an error when dumping the registers");
    }
}
