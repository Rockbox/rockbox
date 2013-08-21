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
};

class RegTreeItem : public QTreeWidgetItem
{
public:
    RegTreeItem(const QString& string, int type);

    void SetPath(int dev_idx, int dev_addr_idx, int reg_idx = -1, int reg_addr_idx = -1);
    int GetDevIndex() const { return m_dev_idx; }
    int GetDevAddrIndex() const { return m_dev_addr_idx; }
    int GetRegIndex() const { return m_reg_idx; }
    int GetRegAddrIndex() const { return m_reg_addr_idx; }
private:
    int m_dev_idx, m_dev_addr_idx, m_reg_idx, m_reg_addr_idx;
};

class RegTab : public QObject
{
    Q_OBJECT
public:
    RegTab(Backend *backend, QTabWidget *parent);

protected:
    void FillDevSubTree(RegTreeItem *item);
    void FillRegTree();
    void FillAnalyserList();
    void UpdateSocList();
    void DisplayRegister(soc_dev_t& dev, soc_dev_addr_t& dev_addr,
        soc_reg_t& reg, soc_reg_addr_t& reg_addr);
    void SetDataSocName(const QString& socname);
    QComboBox *m_soc_selector;
    Backend *m_backend;
    QTreeWidget *m_reg_tree;
    soc_t m_cur_soc;
    QVBoxLayout *m_right_panel;
    QWidget *m_right_content;
    QSplitter *m_splitter;
    QLineEdit *m_data_sel_edit;
    QLabel *m_data_soc_label;
    QPushButton *m_data_sel_reload;
    QComboBox *m_data_selector;
    IoBackend *m_io_backend;
    QTabWidget *m_type_selector;
    QListWidget *m_analysers_list;

private slots:
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