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
#include <QValidator>
#include <QGroupBox>
#include <QToolButton>
#include <QMenu>
#include <QCheckBox>
#include <soc_desc.hpp>
#include "backend.h"
#include "settings.h"

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

class SocFieldValidator : public QValidator
{
    Q_OBJECT
public:
    SocFieldValidator(QObject *parent = 0);
    SocFieldValidator(const soc_reg_field_t& field, QObject *parent = 0);

    virtual void fixup(QString& input) const;
    virtual State validate(QString& input, int& pos) const;
    /* validate and return the interpreted value */
    State parse(const QString& input, soc_word_t& val) const;

protected:
    soc_reg_field_t m_field;
};

class RegLineEdit : public QWidget
{
    Q_OBJECT
public:
    enum EditMode
    {
        Write, Set, Clear, Toggle
    };

    RegLineEdit(QWidget *parent = 0);
    ~RegLineEdit();
    void SetReadOnly(bool ro);
    void EnableSCT(bool en);
    void SetMode(EditMode mode);
    EditMode GetMode();
    QLineEdit *GetLineEdit();

protected slots:
    void OnWriteAct();
    void OnSetAct();
    void OnClearAct();
    void OnToggleAct();
protected:
    void ShowMode(bool show);

    QHBoxLayout *m_layout;
    QToolButton *m_button;
    QLineEdit *m_edit;
    EditMode m_mode;
    bool m_has_sct;
    bool m_readonly;
    QMenu *m_menu;
};

class RegDisplayPanel : public QGroupBox
{
    Q_OBJECT
public:
    RegDisplayPanel(QWidget *parent, IoBackend *io_backend, const SocRegRef& reg);
    void AllowWrite(bool en);

protected:
    IoBackend::WriteMode EditModeToWriteMode(RegLineEdit::EditMode mode);

    IoBackend *m_io_backend;
    const SocRegRef& m_reg;
    bool m_allow_write;
    RegLineEdit *m_raw_val_edit;

private slots:
    void OnRawRegValueReturnPressed();
};

class RegTab : public QSplitter
{
    Q_OBJECT
public:
    RegTab(Backend *backend);
    ~RegTab();

protected:
    enum
    {
        DataSelNothing,
        DataSelFile,
    #ifdef HAVE_HWSTUB
        DataSelDevice,
    #endif
    };

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
    QCheckBox *m_readonly_check;
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
    void SetReadOnlyIndicator();
    void OnSocChanged(const QString& text);
    void OnSocListChanged();
    void OnRegItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void OnRegItemClicked(QTreeWidgetItem *clicked, int col);
    void OnDataSelChanged(int index);
    void OnDataChanged();
    void OnDataSocActivated(const QString&);
    void OnAnalyserChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void OnAnalyserClicked(QListWidgetItem *clicked);
    void OnReadOnlyClicked(bool);
};

#endif /* REGTAB_H */