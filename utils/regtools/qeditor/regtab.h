#ifndef REGTAB_H
#define REGTAB_H

#include <QComboBox>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QSplitter>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QGroupBox>
#include <QToolButton>
#include <QMenu>
#include <QCheckBox>
#include "backend.h"
#include "settings.h"
#include "mainwindow.h"

class RegTabPanel
{
public:
    RegTabPanel() {}
    virtual ~RegTabPanel() {}
    virtual void AllowWrite(bool en) = 0;
    virtual QWidget *GetWidget() = 0;
};

class EmptyRegTabPanel : public QWidget, public RegTabPanel
{
public:
    EmptyRegTabPanel(QWidget *parent = 0);
    void AllowWrite(bool en);
    QWidget *GetWidget();
};

class RegTab : public QSplitter, public DocumentTab
{
    Q_OBJECT
public:
    RegTab(Backend *backend, QWidget *parent = 0);
    ~RegTab();
    virtual bool Quit();

signals:
    void OnModified(bool modified);

protected:
    enum
    {
        DataSelNothing,
        DataSelFile,
    #ifdef HAVE_HWSTUB
        DataSelDevice,
    #endif
    };

    void FillDevSubTree(QTreeWidgetItem *item);
    void FillRegTree();
    void FillAnalyserList();
    void UpdateSocList();
    void DisplayRegister(const SocRegRef& ref);
    void DisplayDevice(const SocDevRef& ref);
    void SetDataSocName(const QString& socname);
    void SetPanel(RegTabPanel *panel);
    void UpdateSocFilename();

    QComboBox *m_soc_selector;
#ifdef HAVE_HWSTUB
    QComboBox *m_dev_selector;
    HWStubBackendHelper m_hwstub_helper;
#endif
    Backend *m_backend;
    QTreeWidget *m_reg_tree;
    SocRef m_cur_soc;
    QVBoxLayout *m_right_panel;
    RegTabPanel *m_right_content;
    QLineEdit *m_data_sel_edit;
    QCheckBox *m_readonly_check;
    QLabel *m_data_soc_label;
    QPushButton *m_data_sel_reload;
    QPushButton *m_dump;
    QComboBox *m_data_selector;
    IoBackend *m_io_backend;
    QTabWidget *m_type_selector;
    QListWidget *m_analysers_list;

private slots:
#ifdef HAVE_HWSTUB
    void OnDevListChanged();
    void OnDevChanged(int index);
    void ClearDevList();
#endif
    void SetReadOnlyIndicator();
    void OnSocChanged(int index);
    void OnSocListChanged();
    void OnRegItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void OnRegItemClicked(QTreeWidgetItem *clicked, int col);
    void OnDataSelChanged(int index);
    void OnDataChanged();
    void OnDataSocActivated(const QString&);
    void OnAnalyserChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void OnAnalyserClicked(QListWidgetItem *clicked);
    void OnReadOnlyClicked(bool);
    void OnDumpRegs(bool);
};

#endif /* REGTAB_H */