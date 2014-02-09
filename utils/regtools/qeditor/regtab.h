#ifndef REGTAB_H
#define REGTAB_H

#include <QComboBox>
#include <QEvent>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QSplitter>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <soc_desc.hpp>
#include "backend.h"
#include "settings.h"

enum
{
    RegTreeDevType = QTreeWidgetItem::UserType,
    RegTreeRegType
};

enum
{
    DataSelNothing,
    DataSelFile,
#ifdef HAVE_HWSTUB
    DataSelDevice,
#endif
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

};

class RegTab : public QSplitter
{
    Q_OBJECT
public:
    RegTab(Backend *backend);

protected:
    void FillDevSubTree(DevTreeItem *item);
    void FillRegTree();
    void FillAnalyserList();
    void UpdateSocList();
    void DisplayRegister(const SocRegRef& ref);
    void SetDataSocName(const QString& socname);
    QComboBox *m_soc_selector;
#ifdef HAVE_HWSTUB
    QComboBox *m_dev_selector;
    HWStubBackendHelper m_hwstub_helper;
#endif
    Backend *m_backend;
    QTreeWidget *m_reg_tree;
    SocRef m_cur_soc;
    QVBoxLayout *m_right_panel;
    QWidget *m_right_content;
    QLineEdit *m_data_sel_edit;
    QLabel *m_data_soc_label;
    QPushButton *m_data_sel_reload;
    QComboBox *m_data_selector;
    IoBackend *m_io_backend;
    QTabWidget *m_type_selector;
    QListWidget *m_analysers_list;

private slots:
#ifdef HAVE_HWSTUB
    void OnDevListChanged();
    void OnDevChanged(int index);
#endif
    void OnSocChanged(const QString& text);
    void OnSocListChanged();
    void OnRegItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void OnRegItemClicked(QTreeWidgetItem *clicked, int col);
    void OnDataSelChanged(int index);
    void OnDataChanged();
    void OnDataSocActivated(const QString&);
    void OnAnalyserChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void OnAnalyserClicked(QListWidgetItem *clicked);
};

#endif /* REGTAB_H */