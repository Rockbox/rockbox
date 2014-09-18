#include "std_analysers.h"

/**
 * Clock analyser
 */

ClockAnalyser::ClockAnalyser(const SocRef& soc, IoBackend *backend)
        :Analyser(soc, backend)
{
    m_group = new QGroupBox("Clock Analyser");
    QVBoxLayout *layout = new QVBoxLayout;
    m_group->setLayout(layout);
    m_tree_widget = new QTreeWidget;
    layout->addWidget(m_tree_widget);

    m_tree_widget->setColumnCount(2);
    QStringList list;
    list << "Name" << "Frequency";
    m_tree_widget->setHeaderLabels(list);

    FillTree();
}

ClockAnalyser::~ClockAnalyser()
{
    delete m_group;
}

QWidget *ClockAnalyser::GetWidget()
{
    return m_group;
}

bool ClockAnalyser::SupportSoc(const QString& soc_name)
{
    return soc_name == "imx233";
}

QString ClockAnalyser::GetFreq(unsigned freq)
{
    if(freq >= 1000000)
    {
        if((freq % 1000000) == 0)
            return QString().sprintf("%d MHz", freq / 1000000);
        else
            return QString().sprintf("%.3f MHz", freq / 1000000.0);
    }
    if(freq >= 1000)
    {
        if((freq % 1000) == 0)
            return QString().sprintf("%d KHz", freq / 1000);
        else
            return QString().sprintf("%.3f KHz", freq / 1000.0);
    }
    return QString().sprintf("%d Hz", freq);
}

QTreeWidgetItem *ClockAnalyser::AddClock(QTreeWidgetItem *parent, const QString& name, int freq, int mul, int div)
{
    if(freq == FROM_PARENT)
    {
        int64_t f = GetClockFreq(parent);
        f *= mul;
        f /= div;
        freq = f;
    }
    QTreeWidgetItem *item = new QTreeWidgetItem(parent, QStringList() << name
        << (freq == INVALID ? "<invalid>" : freq == 0 ? "<disabled>" : GetFreq(freq)));
    item->setData(1, Qt::UserRole, freq);
    if(freq == DISABLED || freq == INVALID || (parent && parent->isDisabled()))
        item->setDisabled(true);
    if(!parent)
        m_tree_widget->addTopLevelItem(item);
    return item;
}

int ClockAnalyser::GetClockFreq(QTreeWidgetItem *item)
{
    return item->data(1, Qt::UserRole).toInt();
}

void ClockAnalyser::FillTree()
{
    m_tree_widget->clear();
    BackendHelper helper(m_io_backend, m_soc);
    soc_word_t value, value2, value3;

    QTreeWidgetItem *ring_osc = 0;
    if(helper.ReadRegisterField("POWER", "MINPWR", "ENABLE_OSC", value))
        ring_osc = AddClock(0, "ring_clk24m", value ? 24000000 : DISABLED);
    else
        ring_osc = AddClock(0, "ring_clk24m", INVALID);
    QTreeWidgetItem *xtal_osc = 0;
    if(helper.ReadRegisterField("POWER", "MINPWR", "PWD_XTAL24", value))
        xtal_osc = AddClock(0, "xtal_clk24m", value ? DISABLED : 24000000);
    else
        xtal_osc = AddClock(0, "xtal_clk24m", INVALID);
    QTreeWidgetItem *ref_xtal = 0;
    if(helper.ReadRegisterField("POWER", "MINPWR", "SELECT_OSC", value))
        ref_xtal = AddClock(value ? ring_osc : xtal_osc, "ref_xtal", FROM_PARENT);
    else
        ref_xtal = AddClock(0, "ref_xtal", INVALID);

    QTreeWidgetItem *ref_pll = 0;
    if(helper.ReadRegisterField("CLKCTRL", "PLLCTRL0", "POWER", value))
        ref_pll = AddClock(ref_xtal, "ref_pll", FROM_PARENT, 20);
    else
        ref_pll = AddClock(0, "ref_pll", INVALID);

    QTreeWidgetItem *ref_io = 0;
    if(helper.ReadRegisterField("CLKCTRL", "FRAC", "CLKGATEIO", value) &&
            helper.ReadRegisterField("CLKCTRL", "FRAC", "IOFRAC", value2))
        ref_io = AddClock(ref_pll, "ref_io", value ? DISABLED : FROM_PARENT, 18, value2);
    else
        ref_io = AddClock(ref_pll, "ref_io", INVALID);

    QTreeWidgetItem *ref_pix = 0;
    if(helper.ReadRegisterField("CLKCTRL", "FRAC", "CLKGATEPIX", value) &&
            helper.ReadRegisterField("CLKCTRL", "FRAC", "PIXFRAC", value2))
        ref_pix = AddClock(ref_pll, "ref_pix", value ? DISABLED : FROM_PARENT, 18, value2);
    else
        ref_pix = AddClock(ref_pll, "ref_pix", INVALID);

    QTreeWidgetItem *ref_emi = 0;
    if(helper.ReadRegisterField("CLKCTRL", "FRAC", "CLKGATEEMI", value) &&
            helper.ReadRegisterField("CLKCTRL", "FRAC", "EMIFRAC", value2))
        ref_emi = AddClock(ref_pll, "ref_emi", value ? DISABLED : FROM_PARENT, 18, value2);
    else
        ref_emi = AddClock(ref_pll, "ref_emi", INVALID);

    QTreeWidgetItem *ref_cpu = 0;
    if(helper.ReadRegisterField("CLKCTRL", "FRAC", "CLKGATECPU", value) &&
            helper.ReadRegisterField("CLKCTRL", "FRAC", "CPUFRAC", value2))
        ref_cpu = AddClock(ref_pll, "ref_cpu", value ? DISABLED : FROM_PARENT, 18, value2);
    else
        ref_cpu = AddClock(ref_pll, "ref_cpu", INVALID);

    QTreeWidgetItem *clk_p = 0;
    if(helper.ReadRegisterField("CLKCTRL", "CLKSEQ", "BYPASS_CPU", value))
    {
        if(!value)
        {
            if(helper.ReadRegisterField("CLKCTRL", "CPU", "DIV_CPU", value2))
                clk_p = AddClock(ref_cpu, "clk_p", FROM_PARENT, 1, value2);
            else
                clk_p = AddClock(ref_cpu, "clk_p", INVALID);
        }
        else
        {
            if(helper.ReadRegisterField("CLKCTRL", "CPU", "DIV_XTAL_FRAC_EN", value) &&
                    helper.ReadRegisterField("CLKCTRL", "CPU", "DIV_XTAL", value2))
                clk_p = AddClock(ref_xtal, "clk_p", FROM_PARENT, value ? 1024 : 1, value2);
            else
                clk_p = AddClock(ref_xtal, "clk_p", INVALID);
        }
    }
    else
        clk_p = AddClock(ref_xtal, "clk_p", INVALID);

    QTreeWidgetItem *clk_h = 0;
    if(helper.ReadRegisterField("CLKCTRL", "HBUS", "DIV_FRAC_EN", value) &&
            helper.ReadRegisterField("CLKCTRL", "HBUS", "DIV", value2))
        clk_h = AddClock(clk_p, "clk_h", FROM_PARENT, value ? 32 : 1, value2);
    else
        clk_h = AddClock(clk_p, "clk_h", INVALID);

    QTreeWidgetItem *clk_x = 0;
    if(helper.ReadRegisterField("CLKCTRL", "XBUS", "DIV", value))
        clk_x = AddClock(ref_xtal, "clk_x", FROM_PARENT, 1, value);
    else
        clk_x = AddClock(ref_xtal, "clk_x", INVALID);

    if(helper.ReadRegisterField("CLKCTRL", "XTAL", "UART_CLK_GATE", value))
        AddClock(ref_xtal, "clk_uart", value ? DISABLED : FROM_PARENT);
    else
        AddClock(ref_xtal, "clk_uart", INVALID);

    if(helper.ReadRegisterField("CLKCTRL", "XTAL", "FILT_CLK24M_GATE", value))
        AddClock(ref_xtal, "clk_filt24m", value ? DISABLED : FROM_PARENT);
    else
        AddClock(ref_xtal, "clk_filt24m", INVALID);

    if(helper.ReadRegisterField("CLKCTRL", "XTAL", "PWM_CLK24M_GATE", value))
        AddClock(ref_xtal, "clk_pwm24m", value ? DISABLED : FROM_PARENT);
    else
        AddClock(ref_xtal, "clk_pwm24m", INVALID);

    if(helper.ReadRegisterField("CLKCTRL", "XTAL", "DRI_CLK24M_GATE", value))
        AddClock(ref_xtal, "clk_dri24m", value ? DISABLED : FROM_PARENT);
    else
        AddClock(ref_xtal, "clk_dri24m", INVALID);

    if(helper.ReadRegisterField("CLKCTRL", "XTAL", "DIGCTRL_CLK1M_GATE", value))
        AddClock(ref_xtal, "clk_1m", value ? DISABLED : FROM_PARENT, 1, 24);
    else
        AddClock(ref_xtal, "clk_1m", INVALID);

    QTreeWidgetItem *clk_32k = 0;
    if(helper.ReadRegisterField("CLKCTRL", "XTAL", "TIMROT_CLK32K_GATE", value))
        clk_32k = AddClock(ref_xtal, "clk_32k", value ? DISABLED : FROM_PARENT, 1, 750);
    else
        clk_32k = AddClock(ref_xtal, "clk_32k", INVALID);

    AddClock(clk_32k, "clk_adc", FROM_PARENT, 1, 16);

    if(helper.ReadRegisterField("CLKCTRL", "CLKSEQ", "BYPASS_PIX", value) &&
            helper.ReadRegisterField("CLKCTRL", "PIX", "DIV", value2))
        AddClock(value ? ref_xtal : ref_pix, "clk_pix", FROM_PARENT, 1, value2);
    else
        AddClock(ref_xtal, "clk_p", INVALID);

    QTreeWidgetItem *clk_ssp = 0;
    if(helper.ReadRegisterField("CLKCTRL", "CLKSEQ", "BYPASS_SSP", value) &&
            helper.ReadRegisterField("CLKCTRL", "SSP", "DIV", value2) &&
            helper.ReadRegisterField("CLKCTRL", "SSP", "CLKGATE", value3))
        clk_ssp = AddClock(value ? ref_xtal : ref_io, "clk_ssp", value3 ? DISABLED : FROM_PARENT, 1, value2);
    else
        clk_ssp = AddClock(ref_xtal, "clk_p", INVALID);

    if(helper.ReadRegisterField("SSP1", "TIMING", "CLOCK_DIVIDE", value) &&
            helper.ReadRegisterField("SSP1", "TIMING", "CLOCK_RATE", value2) &&
            helper.ReadRegisterField("SSP1", "CTRL0", "CLKGATE", value3))
        AddClock(clk_ssp, "clk_ssp1", value3 ? DISABLED : FROM_PARENT, 1, value * (1 + value2));
    else
        AddClock(clk_ssp, "clk_ssp1", INVALID);

    if(helper.ReadRegisterField("SSP2", "TIMING", "CLOCK_DIVIDE", value) &&
            helper.ReadRegisterField("SSP2", "TIMING", "CLOCK_RATE", value2) &&
            helper.ReadRegisterField("SSP2", "CTRL0", "CLKGATE", value3))
        AddClock(clk_ssp, "clk_ssp2", value3 ? DISABLED : FROM_PARENT, 1, value * (1 + value2));
    else
        AddClock(clk_ssp, "clk_ssp2", INVALID);

    QTreeWidgetItem *clk_gpmi = 0;
    if(helper.ReadRegisterField("CLKCTRL", "CLKSEQ", "BYPASS_GPMI", value) &&
            helper.ReadRegisterField("CLKCTRL", "GPMI", "DIV", value2) &&
            helper.ReadRegisterField("CLKCTRL", "GPMI", "CLKGATE", value3))
        clk_gpmi = AddClock(value ? ref_xtal : ref_io, "clk_gpmi", value3 ? DISABLED : FROM_PARENT, 1, value2);
    else
        clk_gpmi = AddClock(ref_xtal, "clk_p", INVALID);

    if(helper.ReadRegisterField("CLKCTRL", "CLKSEQ", "BYPASS_EMI", value))
    {
        if(!value)
        {
            if(helper.ReadRegisterField("CLKCTRL", "EMI", "DIV_EMI", value2) &&
                    helper.ReadRegisterField("CLKCTRL", "EMI", "CLKGATE", value3))
                AddClock(ref_emi, "clk_emi", value3 ? DISABLED : FROM_PARENT, 1, value2);
            else
                AddClock(ref_emi, "clk_emi", INVALID);
        }
        else
        {
            if(helper.ReadRegisterField("CLKCTRL", "EMI", "DIV_XTAL", value2) &&
                    helper.ReadRegisterField("CLKCTRL", "EMI", "CLKGATE", value3))
                AddClock(ref_xtal, "clk_emi", value3 ? DISABLED : FROM_PARENT, 1, value2);
            else
                AddClock(ref_xtal, "clk_emi", INVALID);
        }
    }
    else
        clk_p = AddClock(ref_xtal, "clk_emi", INVALID);

    QTreeWidgetItem *ref_vid = AddClock(ref_pll, "clk_vid", FROM_PARENT);

    if(helper.ReadRegisterField("CLKCTRL", "TV", "CLK_TV108M_GATE", value) &&
            helper.ReadRegisterField("CLKCTRL", "TV", "CLK_TV_GATE", value2))
    {
        QTreeWidgetItem *clk_tv108m = AddClock(ref_vid, "clk_tv108m", value ? DISABLED : FROM_PARENT, 1, 4);
        AddClock(clk_tv108m, "clk_tv54m", value2 ? DISABLED : FROM_PARENT, 1, 2);
        AddClock(clk_tv108m, "clk_tv27m", value2 ? DISABLED : FROM_PARENT, 1, 4);
    }

    if(helper.ReadRegisterField("CLKCTRL", "PLLCTRL0", "EN_USB_CLKS", value))
        AddClock(ref_pll, "utmi_clk480m", value ? FROM_PARENT : DISABLED);
    else
        AddClock(ref_pll, "utmi_clk480m", INVALID);

    QTreeWidgetItem *xtal_clk32k = 0;
    if(helper.ReadRegisterField("RTC", "PERSISTENT0", "XTAL32_FREQ", value) &&
            helper.ReadRegisterField("RTC", "PERSISTENT0", "XTAL32KHZ_PWRUP", value2))
        xtal_clk32k = AddClock(0, "xtal_clk32k", value2 == 0 ? DISABLED : value ? 32000 : 32768);
    else
        xtal_clk32k = AddClock(0, "xtal_clk32k", INVALID);

    if(helper.ReadRegisterField("RTC", "PERSISTENT0", "CLOCKSOURCE", value))
        AddClock(value ? xtal_clk32k : ref_xtal, "clk_rtc32k", FROM_PARENT, 1, value ? 1 : 768);
    else
        AddClock(ref_xtal, "clk_rtc32k", INVALID);

    Q_UNUSED(clk_x);
    Q_UNUSED(clk_gpmi);
    Q_UNUSED(clk_h);

    m_tree_widget->expandAll();
    m_tree_widget->resizeColumnToContents(0);
}

static TmplAnalyserFactory< ClockAnalyser > g_clock_factory(true, "Clock Analyser");

/**
 * EMI analyser
 */
EmiAnalyser::EmiAnalyser(const SocRef& soc, IoBackend *backend)
    :Analyser(soc, backend)
{
    m_display_mode = DisplayCycles;
    m_group = new QGroupBox("EMI Analyser");
    QVBoxLayout *layout = new QVBoxLayout;
    m_group->setLayout(layout);
    m_panel = new QToolBox;
    m_display_selector = new QComboBox;
    m_display_selector->addItem("Cycles", DisplayCycles);
    m_display_selector->addItem("Raw Hexadecimal", DisplayRawHex);
    m_display_selector->addItem("Time", DisplayTime);
    QHBoxLayout *line_layout = new QHBoxLayout;
    line_layout->addWidget(new QLabel("Display Mode:"));
    line_layout->addWidget(m_display_selector);
    m_emi_freq_label = new QLineEdit;
    m_emi_freq_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_emi_freq_label->setReadOnly(true);
    line_layout->addStretch();
    line_layout->addWidget(new QLabel("Frequency:"));
    line_layout->addWidget(m_emi_freq_label);
    line_layout->addWidget(new QLabel("MHz"));
    line_layout->addStretch();
    layout->addLayout(line_layout);
    layout->addWidget(m_panel);

    connect(m_display_selector, SIGNAL(currentIndexChanged(int)), this,
        SLOT(OnChangeDisplayMode(int)));

    FillTable();
}

EmiAnalyser::~EmiAnalyser()
{
    delete m_group;
}

QWidget *EmiAnalyser::GetWidget()
{
    return m_group;
}

bool EmiAnalyser::SupportSoc(const QString& soc_name)
{
    return soc_name == "imx233";
}

void EmiAnalyser::OnChangeDisplayMode(int index)
{
    if(index == -1)
        return;
    m_display_mode = (DisplayMode)m_display_selector->itemData(index).toInt();
    int idx = m_panel->currentIndex();
    FillTable();
    m_panel->setCurrentIndex(idx);
}

void EmiAnalyser::NewGroup(const QString& name)
{
    QTableWidget *table = new QTableWidget;
    table->setColumnCount(3);
    table->setHorizontalHeaderItem(0, new QTableWidgetItem("Name"));
    table->setHorizontalHeaderItem(1, new QTableWidgetItem("Value"));
    table->setHorizontalHeaderItem(2, new QTableWidgetItem("Comment"));
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    m_panel->addItem(table, name);
}

void EmiAnalyser::AddLine(const QString& name, int value, const QString& unit, const QString& comment)
{
    QTableWidget *table = dynamic_cast< QTableWidget* >(m_panel->widget(m_panel->count() - 1));
    int row = table->rowCount();
    table->setRowCount(row + 1);
    table->setItem(row, 0, new QTableWidgetItem(name));
    QString val;
    if(value == INVALID)
        val = "<invalid>";
    else if(value == NONE)
        val = unit;
    else if(m_display_mode == DisplayRawHex && unit.size() == 0)
        val = QString("0x%1").arg(value, 0, 16);
    else
        val = QString("%1%2").arg(value).arg(unit);
    table->setItem(row, 1, new QTableWidgetItem(val));
    table->item(row, 1)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    table->setItem(row, 2, new QTableWidgetItem(comment));
    table->resizeColumnToContents(0);
    table->resizeColumnToContents(1);
}

void EmiAnalyser::AddCycleLine(const QString& name, unsigned raw_val, float val,
    int digits, const QString& comment)
{
    if(m_display_mode == DisplayCycles)
    {
        QString str;
        if(digits == 0)
            str = QString("%1").arg((int)val);
        else
            str = QString("%1").arg(val, 0, 'f', digits);
        str += " cycles";
        AddLine(name, NONE, str, comment);
    }
    else if(m_display_mode == DisplayRawHex)
    {
        QString str = QString("0x%1").arg(raw_val, 0, 16);
        AddLine(name, NONE, str, comment);
    }
    else if(m_display_mode == DisplayTime && m_emi_freq != 0)
    {
        float cycle_time_ns = 1000000000.0 / m_emi_freq;
        val *= cycle_time_ns;
        QString str;
        if(val >= 1000)
            str = QString::fromWCharArray(L"%1 µs").arg(val / 1000.0, 0, 'f', 2);
        else
            str = QString("%1 ns").arg(val, 0, 'f', 2);
        AddLine(name, NONE, str, comment);
    }
    else
        AddLine(name, raw_val, " cycles", comment);
}

void EmiAnalyser::FillTable()
{
    while(m_panel->count() > 0)
        m_panel->removeItem(0);
    BackendHelper helper(m_io_backend, m_soc);
    soc_word_t value;

    m_emi_freq = 0;
    if(helper.ReadRegisterField("CLKCTRL", "CLKSEQ", "BYPASS_EMI", value))
    {
        bool ret;
        if(value)
        {
            m_emi_freq = 24000000;
            ret = helper.ReadRegisterField("CLKCTRL", "EMI", "DIV_XTAL", value);
        }
        else
        {
            m_emi_freq = 480000000;
            if(helper.ReadRegisterField("CLKCTRL", "FRAC", "EMIFRAC", value))
                m_emi_freq = 18 * (int64_t)m_emi_freq / value;
            else
                m_emi_freq = 0;
            ret = helper.ReadRegisterField("CLKCTRL", "EMI", "DIV_EMI", value);
        }
        if(ret)
            m_emi_freq /= value;
        else
            m_emi_freq = 0;
    }

    m_emi_freq_label->setText(QString().sprintf("%.3f", m_emi_freq / 1000000.0));

    NewGroup("Control Parameters");
    if(helper.ReadRegisterField("EMI", "CTRL", "PORT_PRIORITY_ORDER", value))
    {
        QStringList ports;
        ports << "AXI0" << "AHB1" << "AHB2" << "AHB3";
        QString order;
        order += ports[value / 6];
        ports.erase(ports.begin() + value / 6);
        int ord[6][3] = { {0, 1, 2}, {2, 0, 1}, {1, 2, 0}, {2, 1, 0}, {1, 0, 2}, {0, 2, 1} };
        for(int i = 0; i < 3; i++)
            order += ", " + ports[ord[value % 6][i]];
        AddLine("Port Priority Order", value, "", order);
    }

    if(helper.ReadRegisterField("EMI", "CTRL", "MEM_WIDTH", value))
        AddLine("Memory Width", value ? 16 : 8, "-bit");

    if(helper.ReadRegisterField("DRAM", "CTL03", "AP", value))
        AddLine("Auto Pre-Charge", NONE, value ? "Yes" : "No");

    bool bypass_mode = false;
    if(helper.ReadRegisterField("DRAM", "CTL04", "DLL_BYPASS_MODE", value))
    {
        bypass_mode = value == 1;
        AddLine("DLL Bypass Mode", NONE, value ? "Yes" : "No");
    }

    if(helper.ReadRegisterField("DRAM", "CTL05", "EN_LOWPOWER_MODE", value))
        AddLine("Low Power Mode", NONE, value ? "Enabled" : "Disabled");

    if(helper.ReadRegisterField("DRAM", "CTL08", "SREFRESH", value))
        AddLine("Self Refresh", NONE, value ? "Yes" : "No");

    if(helper.ReadRegisterField("DRAM", "CTL08", "SDR_MODE", value))
        AddLine("Mode", NONE, value ? "SDR" : "DDR");

    if(helper.ReadRegisterField("DRAM", "CTL10", "ADDR_PINS", value))
        AddLine("Address Pins", 13 - value, "");

    if(helper.ReadRegisterField("DRAM", "CTL11", "COLUMN_SIZE", value))
        AddLine("Column Size", 12 - value, "-bit");

    if(helper.ReadRegisterField("DRAM", "CTL11", "CASLAT", value))
        AddLine("Encoded CAS", value, "", "Memory device dependent");

    if(helper.ReadRegisterField("DRAM", "CTL14", "CS_MAP", value))
    {
        QString v;
        for(int i = 0; i < 4; i++)
            if(value & (1 << i))
            {
                if(v.size() != 0)
                    v += " ";
                v += QString("%1").arg(i);
            }
        AddLine("Chip Select Pins", NONE, v, "");
    }

    if(helper.ReadRegisterField("DRAM", "CTL37", "TREF_ENABLE", value))
        AddLine("Refresh Commands", NONE, value ? "Enabled" : "Disabled", "Issue self-refresh every TREF cycles");

    NewGroup("Frequency Parameters");

    if(helper.ReadRegisterField("DRAM", "CTL13", "CASLAT_LIN_GATE", value))
    {
        if(value >= 3 && value <= 10 && value != 9)
        {
            float v = (value / 2) + 0.5 * (value % 2);
            AddCycleLine("CAS Gate", value, v, 1, "");
        }
        else
            AddLine("CAS Gate", NONE, "Reserved", "Reserved value");
    }
    if(helper.ReadRegisterField("DRAM", "CTL13", "CASLAT_LIN", value))
    {
        if(value >= 3 && value <= 10 && value != 9)
        {
            float v = (value / 2) + 0.5 * (value % 2);
            AddCycleLine("CAS Latency", value, v, 1, "");
        }
        else
            AddLine("CAS Latency", NONE, "Reserved", "Reserved value");
    }

    if(helper.ReadRegisterField("DRAM", "CTL12", "TCKE", value))
        AddCycleLine("tCKE", value, value, 0, "Minimum CKE pulse width");

    if(helper.ReadRegisterField("DRAM", "CTL15", "TDAL", value))
        AddCycleLine("tDAL", value, value, 0, "Auto pre-charge write recovery time");

    if(helper.ReadRegisterField("DRAM", "CTL31", "TDLL", value))
        AddCycleLine("tDLL", value, value, 0, "DLL lock time");

    if(helper.ReadRegisterField("DRAM", "CTL10", "TEMRS", value))
        AddCycleLine("tEMRS", value, value, 0, "Extended mode parameter set time");

    if(helper.ReadRegisterField("DRAM", "CTL34", "TINIT", value))
        AddCycleLine("tINIT", value, value, 0, "Initialisation time");

    if(helper.ReadRegisterField("DRAM", "CTL16", "TMRD", value))
        AddCycleLine("tMRD", value, value, 0, "Mode register set command time");

    if(helper.ReadRegisterField("DRAM", "CTL40", "TPDEX", value))
        AddCycleLine("tPDEX", value, value, 0, "Power down exit time");

    if(helper.ReadRegisterField("DRAM", "CTL32", "TRAS_MAX", value))
        AddCycleLine("tRAS Max", value, value, 0, "Maximum row activate time");

    if(helper.ReadRegisterField("DRAM", "CTL20", "TRAS_MIN", value))
        AddCycleLine("tRAS Min", value, value, 0, "Minimum row activate time");

    if(helper.ReadRegisterField("DRAM", "CTL17", "TRC", value))
        AddCycleLine("tRC", value, value, 0, "Activate to activate delay (same bank)");

    if(helper.ReadRegisterField("DRAM", "CTL20", "TRCD_INT", value))
        AddCycleLine("tRCD", value, value, 0, "RAS to CAS");

    if(helper.ReadRegisterField("DRAM", "CTL26", "TREF", value))
        AddCycleLine("tREF", value, value, 0, "Refresh to refresh time");

    if(helper.ReadRegisterField("DRAM", "CTL21", "TRFC", value))
        AddCycleLine("tRFC", value, value, 0, "Refresh command time");

    if(helper.ReadRegisterField("DRAM", "CTL15", "TRP", value))
        AddCycleLine("tRP", value, value, 0, "Pre-charge command time");

    if(helper.ReadRegisterField("DRAM", "CTL12", "TRRD", value))
        AddCycleLine("tRRD", value, value, 0, "Activate to activate delay (different banks)");

    if(helper.ReadRegisterField("DRAM", "CTL12", "TWR_INT", value))
        AddCycleLine("tWR", value, value, 0, "Write recovery time");

    if(helper.ReadRegisterField("DRAM", "CTL13", "TWTR", value))
        AddCycleLine("tWTR", value, value, 0, "Write to read delay");

    if(helper.ReadRegisterField("DRAM", "CTL32", "TXSNR", value))
        AddCycleLine("tXSNR", value, value, 0, "");

    if(helper.ReadRegisterField("DRAM", "CTL33", "TXSR", value))
        AddCycleLine("tXSR", value, value, 0, "Self-refresh exit time");

    NewGroup("DLL Parameters");

    if(bypass_mode)
    {
        if(helper.ReadRegisterField("DRAM", "CTL19", "DLL_DQS_DELAY_BYPASS_0", value))
            AddLine("DLL DQS Delay 0", value, "", "In 1/128 fraction of a cycle (bypass mode)");

        if(helper.ReadRegisterField("DRAM", "CTL19", "DLL_DQS_DELAY_BYPASS_0", value))
            AddLine("DLL DQS Delay 1", value, "", "In 1/128 fraction of a cycle (bypass mode)");

        if(helper.ReadRegisterField("DRAM", "CTL19", "DQS_OUT_SHIFT_BYPASS", value))
            AddLine("DQS Out Delay", value, "", "(bypass mode)");

        if(helper.ReadRegisterField("DRAM", "CTL20", "WR_DQS_SHIFT_BYPASS", value))
            AddLine("DQS Write Delay", value, "", "(bypass mode)");
    }
    else
    {
        if(helper.ReadRegisterField("DRAM", "CTL17", "DLL_START_POINT", value))
            AddLine("DLL Start Point", value, "", "Initial delay count");

        if(helper.ReadRegisterField("DRAM", "CTL17", "DLL_INCREMENT", value))
            AddLine("DLL Increment", value, "", "Delay increment");

        if(helper.ReadRegisterField("DRAM", "CTL18", "DLL_DQS_DELAY_0", value))
            AddLine("DLL DQS Delay 0", value, "", "In 1/128 fraction of a cycle");

        if(helper.ReadRegisterField("DRAM", "CTL18", "DLL_DQS_DELAY_1", value))
            AddLine("DLL DQS Delay 1", value, "", "In 1/128 fraction of a cycle");

        if(helper.ReadRegisterField("DRAM", "CTL19", "DQS_OUT_SHIFT", value))
            AddLine("DQS Out Delay", value, "", "");

        if(helper.ReadRegisterField("DRAM", "CTL20", "WR_DQS_SHIFT", value))
            AddLine("DQS Write Delay", value, "", "");
    }

}

static TmplAnalyserFactory< EmiAnalyser > g_emi_factory(true, "EMI Analyser");

/**
 * Pin analyser
 */

namespace pin_desc
{
#include "../../imxtools/misc/map.h"
}

PinAnalyser::PinAnalyser(const SocRef& soc, IoBackend *backend)
        :Analyser(soc, backend)
{
    m_group = new QGroupBox("Pin Analyser");
    QVBoxLayout *layout = new QVBoxLayout;
    m_group->setLayout(layout);
    QLabel *label = new QLabel("Package:");
    m_package_edit = new QLineEdit;
    m_package_edit->setReadOnly(true);
    m_package_edit->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    hlayout->addWidget(label);
    hlayout->addWidget(m_package_edit);
    hlayout->addStretch();
    layout->addLayout(hlayout);
    m_panel = new QToolBox;
    layout->addWidget(m_panel);

    FillList();
}

PinAnalyser::~PinAnalyser()
{
    delete m_group;
}

QWidget *PinAnalyser::GetWidget()
{
    return m_group;
}

bool PinAnalyser::SupportSoc(const QString& soc_name)
{
    return soc_name == "imx233" || soc_name == "stmp3700";
}

void PinAnalyser::FillList()
{
    BackendHelper helper(m_io_backend, m_soc);
    soc_word_t value;

    while(m_panel->count() > 0)
        m_panel->removeItem(0);

    const char *package_type[8] =
    {
        "bga169", "bga100", "lqfp100", "lqfp128", 0, 0, 0, 0
    };

    if(!helper.ReadRegisterField("DIGCTL", "STATUS", "PACKAGE_TYPE", value))
    {
        m_package_edit->setText("<read error>");
        return;
    }
    if(value >= 8 || package_type[value] == NULL)
    {
        m_package_edit->setText("<unknown package>");
        return;
    }
    const char *package = package_type[value];
    m_package_edit->setText(package);
    pin_desc::bank_map_t *map = NULL;
    for(size_t i = 0; i < sizeof(pin_desc::socs) / sizeof(pin_desc::socs[0]); i++)
        if(QString(pin_desc::socs[i].soc) == m_io_backend->GetSocName() && 
                QString(pin_desc::socs[i].ver) == package)
            map = pin_desc::socs[i].map;
    if(map == NULL)
    {
        m_package_edit->setText(QString("%1 (no map available)").arg(package));
        return;
    }

    QMap< unsigned, QColor > color_map;
    color_map[PIN_GROUP_EMI] = QColor(255, 255, 64);
    color_map[PIN_GROUP_GPIO] = QColor(171, 214, 230);
    color_map[PIN_GROUP_I2C] = QColor(191, 191, 255);
    color_map[PIN_GROUP_JTAG] = QColor(238, 75, 21);
    color_map[PIN_GROUP_PWM] = QColor(255, 236, 179);
    color_map[PIN_GROUP_SPDIF] = QColor(174, 235, 63);
    color_map[PIN_GROUP_TIMROT] = QColor(255, 112, 237);
    color_map[PIN_GROUP_AUART] = QColor(94, 255, 128);
    color_map[PIN_GROUP_ETM] = QColor(168, 53, 14);
    color_map[PIN_GROUP_GPMI] = QColor(255, 211, 147);
    color_map[PIN_GROUP_IrDA] = QColor(64, 97, 255);
    color_map[PIN_GROUP_LCD] = QColor(124, 255, 255);
    color_map[PIN_GROUP_SAIF] = QColor(255, 158, 158);
    color_map[PIN_GROUP_SSP] = QColor(222, 128, 255);
    color_map[PIN_GROUP_DUART] = QColor(192, 191, 191);
    color_map[PIN_GROUP_USB] = QColor(0, 255, 0);
    color_map[PIN_GROUP_NONE] = QColor(255, 255, 255);

    for(int bank = 0; bank < 4; bank++)
    {
        QTableWidget *table = new QTableWidget;
        table->setColumnCount(7);
        table->setHorizontalHeaderItem(0, new QTableWidgetItem("Pin"));
        table->setHorizontalHeaderItem(1, new QTableWidgetItem("Function"));
        table->setHorizontalHeaderItem(2, new QTableWidgetItem("Direction"));
        table->setHorizontalHeaderItem(3, new QTableWidgetItem("Drive"));
        table->setHorizontalHeaderItem(4, new QTableWidgetItem("Voltage"));
        table->setHorizontalHeaderItem(5, new QTableWidgetItem("Pull"));
        table->setHorizontalHeaderItem(6, new QTableWidgetItem("Value"));
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setStretchLastSection(true);
        m_panel->addItem(table, QString("Bank %1").arg(bank));
        uint32_t muxsel[2], drive[4], pull, in, out, oe;
        bool error = false;
        for(int i = 0; i < 2; i++)
            if(!helper.ReadRegister("PINCTRL", QString("MUXSEL%1").arg(bank * 2 + i), muxsel[i]))
                error = true;
        /* don't make an error for those since some do not exist */
        for(int i = 0; i < 4; i++)
            if(!helper.ReadRegister("PINCTRL", QString("DRIVE%1").arg(bank * 4 + i), drive[i]))
                drive[i] = 0;
        if(error)
            continue;
        if(!helper.ReadRegister("PINCTRL", QString("PULL%1").arg(bank), pull))
            pull = 0;
        if(!helper.ReadRegister("PINCTRL", QString("DIN%1").arg(bank), in))
            in = 0;
        if(!helper.ReadRegister("PINCTRL", QString("DOUT%1").arg(bank), out))
            out = 0;
        if(!helper.ReadRegister("PINCTRL", QString("DOE%1").arg(bank), oe))
            oe = 0;

        for(int pin = 0; pin < 32; pin++)
        {
            /* skip all-reserved pins */
            bool all_dis = true;
            for(int fn = 0; fn < 4; fn++)
                if(map[bank].pins[pin].function[fn].name != NULL)
                    all_dis = false;
            if(all_dis)
                continue;
            /* add line */
            int row = table->rowCount();
            table->setRowCount(row + 1);
            /* name */
            table->setItem(row, 0, new QTableWidgetItem(QString("B%1P%2")
                .arg(bank).arg(pin, 2, 10, QChar('0'))));
            table->item(row, 0)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            /* function */
            int fn = (muxsel[pin / 16] >> ((pin % 16) * 2)) & 3;
            table->setItem(row, 1, new QTableWidgetItem(QString(map[bank].pins[pin].function[fn].name)));
            table->item(row, 1)->setBackground(QBrush(color_map[map[bank].pins[pin].function[fn].group]));
            table->item(row, 1)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            /* direction */
            table->setItem(row, 2, new QTableWidgetItem(fn != 3 ? "" : (oe & (1 << pin)) ? "Output" : "Input"));
            table->item(row, 2)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            /* drive */
            int drv = (drive[pin / 8] >> ((pin % 8) * 4)) & 3;
            const char *strength[4] = {"4 mA", "8 mA", "12 mA", "16 mA"};
            table->setItem(row, 3, new QTableWidgetItem(QString(strength[drv])));
            table->item(row, 3)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            /* voltage */
            int volt = (drive[pin / 8] >> (((pin % 8) * 4) + 2)) & 1;
            if(m_io_backend->GetSocName() == "imx233")
                volt = 1; /* cannot change voltage on imx233 */
            const char *voltage[2] = {"1.8 V", "3.3 V"};
            table->setItem(row, 4, new QTableWidgetItem(QString(voltage[volt])));
            table->item(row, 4)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            /* pull */
            table->setItem(row, 5, new QTableWidgetItem(QString("%1").arg((pull >> pin) & 1)));
            table->item(row, 5)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            /* input */
            table->setItem(row, 6, new QTableWidgetItem(QString("%1").arg((in >> pin) & 1)));
            table->item(row, 6)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        }
    }
}

static TmplAnalyserFactory< PinAnalyser > g_pin_factory(true, "Pin Analyser");
