#ifndef AUX_H
#define AUX_H

#include <QEvent>
#include <QPaintEvent>
#include <QLineEdit>
#include <QValidator>
#include <QToolButton>
#include <QMenu>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QTableWidget>
#include <QToolBar>
#include <QLabel>
#include <QHBoxLayout>
#include <QItemEditorCreatorBase>
#include <QStyledItemDelegate>
#include <QComboBox>
#include <QFileDialog>
#include "settings.h"
#include "backend.h"

class SocBitRangeValidator : public QValidator
{
    Q_OBJECT
public:
    SocBitRangeValidator(QObject *parent = 0);

    virtual void fixup(QString& input) const;
    virtual State validate(QString& input, int& pos) const;
    /* validate and return the interpreted value */
    State parse(const QString& input, int& last_bit, int& first_bit) const;
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
    void setText(const QString& text);
    QString text() const;

    Q_PROPERTY(QString text READ text WRITE setText USER true)

protected slots:
    void OnWriteAct();
    void OnSetAct();
    void OnClearAct();
    void OnToggleAct();
protected:
    void ShowMode(bool show);
    void DoAutoHide();

    QHBoxLayout *m_layout;
    QToolButton *m_button;
    QLineEdit *m_edit;
    EditMode m_mode;
    bool m_has_sct;
    bool m_readonly;
    QMenu *m_menu;
};

class SocFieldItemDelegate : public QStyledItemDelegate
{
public:
    SocFieldItemDelegate(QObject *parent = 0):QStyledItemDelegate(parent), m_bitcount(32) {}
    SocFieldItemDelegate(const soc_reg_field_t& field, QObject *parent = 0)
        :QStyledItemDelegate(parent), m_bitcount(field.last_bit - field.first_bit + 1) {}

    virtual QString displayText(const QVariant& value, const QLocale& locale) const;
protected:
    int m_bitcount;
};

class SocFieldEditor : public QLineEdit
{
    Q_OBJECT
    Q_PROPERTY(uint field READ field WRITE setField USER true)
public:
    SocFieldEditor(const soc_reg_field_t& field, QWidget *parent = 0);
    virtual ~SocFieldEditor();

    uint field() const;
    void setField(uint field);
    void SetRegField(const soc_reg_field_t& field) { m_reg_field = field; }

protected:
    SocFieldValidator *m_validator;
    uint m_field;
    soc_reg_field_t m_reg_field;
};

class SocFieldEditorCreator : public QItemEditorCreatorBase
{
public:
    SocFieldEditorCreator() { m_field.first_bit = 0; m_field.last_bit = 31; }
    SocFieldEditorCreator(const soc_reg_field_t& field):m_field(field) {}

    virtual QWidget *createWidget(QWidget *parent) const;
    virtual QByteArray valuePropertyName() const;

protected:
    soc_reg_field_t m_field;
};

class SocFieldCachedValue
{
public:
    SocFieldCachedValue():m_value(0) {}
    SocFieldCachedValue(const soc_reg_field_t& field, uint value)
        :m_field(field), m_value(value) {}
    virtual ~SocFieldCachedValue() {}
    const soc_reg_field_t& field() const { return m_field; }
    uint value() const { return m_value; }
protected:
    soc_reg_field_t m_field;
    uint m_value;
};

Q_DECLARE_METATYPE(SocFieldCachedValue)

class SocFieldCachedItemDelegate : public QStyledItemDelegate
{
public:
    SocFieldCachedItemDelegate(QObject *parent = 0):QStyledItemDelegate(parent) {}

    virtual QString displayText(const QVariant& value, const QLocale& locale) const;
};

class SocFieldCachedEditor : public SocFieldEditor
{
    Q_OBJECT
    Q_PROPERTY(SocFieldCachedValue value READ value WRITE setValue USER true)
public:
    SocFieldCachedEditor(QWidget *parent = 0);
    virtual ~SocFieldCachedEditor();

    SocFieldCachedValue value() const;
    void setValue(SocFieldCachedValue field);
protected:
    SocFieldCachedValue m_value;
};

class SocFieldCachedEditorCreator : public QItemEditorCreatorBase
{
public:
    SocFieldCachedEditorCreator() {}

    virtual QWidget *createWidget(QWidget *parent) const;
    virtual QByteArray valuePropertyName() const;

protected:
};

class RegSexyDisplay : public QWidget
{
    Q_OBJECT
public:
    RegSexyDisplay(const SocRegRef& reg, QWidget *parent = 0);

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

protected:
    int marginSize() const;
    int separatorSize() const;
    int columnWidth() const;
    int headerHeight() const;
    int gapHeight() const;
    int maxContentHeight() const;
    int textSep() const;
    void paintEvent(QPaintEvent *event);

private:
    SocRegRef m_reg;
    mutable QSize m_size;
};

class GrowingTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    GrowingTextEdit(QWidget *parent = 0);

protected slots:
    void TextChanged();
};

class GrowingTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    GrowingTableWidget(QWidget *parent = 0);

protected slots:
    void DataChanged(const QModelIndex& tl, const QModelIndex& br);
};

class MyTextEditor : public QWidget
{
    Q_OBJECT
public:
    MyTextEditor(QWidget *parent = 0);
    void SetGrowingMode(bool en);
    void SetReadOnly(bool ro);
    void SetTextHtml(const QString& text);
    QString GetTextHtml();
    bool IsModified();
signals:
    void OnTextChanged();

protected slots:
    void OnInternalTextChanged();
    void OnTextBold(bool checked);
    void OnTextItalic(bool checked);
    void OnTextUnderline(bool checked);
    void OnCharFormatChanged(const QTextCharFormat& fmt);

protected:
    bool m_growing_mode;
    bool m_read_only;
    QToolBar *m_toolbar;
    QTextEdit *m_edit;
    QToolButton *m_bold_button;
    QToolButton *m_italic_button;
    QToolButton *m_underline_button;
};

class MySwitchableTextEditor : public QWidget
{
    Q_OBJECT
public:
    MySwitchableTextEditor(QWidget *parent = 0);
    QString GetTextHtml();
    void SetTextHtml(const QString& text);
    void SetEditorMode(bool en);
    MyTextEditor *GetEditor();
    QLineEdit *GetLineEdit();
    QLabel *GetLabel();
    void SetLineMode(bool en);
    bool IsModified();

protected:
    void UpdateVisibility();

    bool m_editor_mode;
    bool m_line_mode;
    QLabel *m_label;
    MyTextEditor *m_edit;
    QLineEdit *m_line;
};

class BackendSelector : public QWidget
{
    Q_OBJECT
public:
    BackendSelector(Backend *backend, QWidget *parent = 0);
    virtual ~BackendSelector();
    IoBackend *GetBackend();

signals:
    void OnSelect(IoBackend *backend);

protected:
    enum
    {
        DataSelNothing,
        DataSelFile,
#ifdef HAVE_HWSTUB
        DataSelDevice,
#endif
    };

    Backend *m_backend;
    IoBackend *m_io_backend;
    QComboBox *m_data_selector;
    QLineEdit *m_data_sel_edit;
#ifdef HAVE_HWSTUB
    QComboBox *m_dev_selector;
    HWStubBackendHelper m_hwstub_helper;
#endif
    void ChangeBackend(IoBackend *new_backend);

private slots:
#ifdef HAVE_HWSTUB
    void OnDevListChanged();
    void OnDevChanged(int index);
    void ClearDevList();
#endif
    void OnDataSelChanged(int index);
};

#endif /* AUX_H */
