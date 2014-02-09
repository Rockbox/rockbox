#ifndef _STDANALYSER_H_
#define _STDANALYSER_H_

#include "analyser.h"

#include <QGroupBox>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QToolBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include "analyser.h"

/**
 * Clock analyser
 */

class ClockAnalyser : public Analyser
{
    Q_OBJECT
public:
    ClockAnalyser(const SocRef& soc, IoBackend *backend);
    virtual ~ClockAnalyser();
    virtual QWidget *GetWidget();
    static bool SupportSoc(const QString& soc_name);

private:
    QString GetFreq(unsigned freq);

    enum
    {
        DISABLED = 0,
        INVALID = -1,
        FROM_PARENT = -2,
    };

    QTreeWidgetItem *AddClock(QTreeWidgetItem *parent, const QString& name, int freq, int mul = 1, int div = 1);
    int GetClockFreq(QTreeWidgetItem *item);
    void FillTree();

private:
    QGroupBox *m_group;
    QTreeWidget *m_tree_widget;
};

/**
 * EMI analyser
 */
class EmiAnalyser : public Analyser
{
    Q_OBJECT
public:
    EmiAnalyser(const SocRef& soc, IoBackend *backend);
    virtual ~EmiAnalyser();
    virtual QWidget *GetWidget();

    static bool SupportSoc(const QString& soc_name);

private slots:
    void OnChangeDisplayMode(int index);

private:
    enum DisplayMode
    {
        DisplayCycles,
        DisplayRawHex,
        DisplayTime,
    };

    enum
    {
        NONE = -999999,
        INVALID = -1000000
    };

    void NewGroup(const QString& name);
    void AddLine(const QString& name, int value, const QString& unit, const QString& comment = "");
    void AddCycleLine(const QString& name, unsigned raw_val, float val, int digits, const QString& comment = "");
    void FillTable();

private:
    QGroupBox *m_group;
    QComboBox *m_display_selector;
    QToolBox *m_panel;
    DisplayMode m_display_mode;
    unsigned m_emi_freq;
    QLineEdit *m_emi_freq_label;
};

/**
 * PINCTRL analyzer
 */
class PinAnalyser : public Analyser
{
    Q_OBJECT
public:
    PinAnalyser(const SocRef& soc, IoBackend *backend);
    virtual ~PinAnalyser();
    virtual QWidget *GetWidget();

    static bool SupportSoc(const QString& soc_name);

private:
    void FillList();

private:
    QGroupBox *m_group;
    QLineEdit *m_package_edit;
    QToolBox *m_panel;
};

#endif /* _STDANALYSER_H_ */
