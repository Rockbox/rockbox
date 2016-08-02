/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Amaury Pouly
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "std_analysers.h"
#include <QDebug>

/**
 * AnalyserEx
 */

AnalyserEx::AnalyserEx(const soc_desc::soc_ref_t& soc, IoBackend *backend)
    :Analyser(soc, backend), m_helper(backend, soc)
{
}

bool AnalyserEx::ReadRegister(const QString& path, soc_word_t& val)
{
    return m_helper.ReadRegister(m_helper.ParsePath(path), val);
}

bool AnalyserEx::ReadRegisterOld(const QString& dev, const QString& reg, soc_word_t& val)
{
    return ReadRegister(dev + "." + reg, val);
}

bool AnalyserEx::ReadField(const QString& path, const QString& field, soc_word_t& val)
{
    return m_helper.ReadRegisterField(m_helper.ParsePath(path), field, val);
}

bool AnalyserEx::ReadFieldOld(const QString& dev, const QString& reg,
    const QString& field, soc_word_t& val)
{
    return ReadField(dev + "." + reg, field, val);
}

/**
 * Clock analyser
 */

ClockAnalyser::ClockAnalyser(const soc_desc::soc_ref_t& soc, IoBackend *backend)
        :AnalyserEx(soc, backend)
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
    return soc_name == "imx233"
        || soc_name == "rk27xx"
        || soc_name == "atj213x"
        || soc_name == "jz4760b";
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

QTreeWidgetItem *ClockAnalyser::AddClock(QTreeWidgetItem *parent, const QString& name,
    int freq, int mul, int div)
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
    if(m_soc.get()->name == "imx233") FillTreeIMX233();
    else if(m_soc.get()->name == "rk27xx") FillTreeRK27XX();
    else if(m_soc.get()->name == "atj213x") FillTreeATJ213X();
    else if(m_soc.get()->name == "jz4760b") FillTreeJZ4760B();
    m_tree_widget->expandAll();
    m_tree_widget->resizeColumnToContents(0);
}

void ClockAnalyser::FillTreeJZ4760B()
{
    AddClock(0, "RTCLK", 32768);
    // assume EXCLK is 12MHz, we have no way to knowing for sure but this is the
    // recommended value anyway
    QTreeWidgetItem *exclk = AddClock(0, "EXCLK", 12000000);
    // PLL0
    soc_word_t pllm, plln, pllod, pllbypass;
    QTreeWidgetItem *pll0 = 0;
    if(ReadFieldOld("CPM", "PLLCTRL0", "FEED_DIV", pllm) &&
            ReadFieldOld("CPM", "PLLCTRL0", "IN_DIV", plln) &&
            ReadFieldOld("CPM", "PLLCTRL0", "OUT_DIV", pllod) &&
            ReadFieldOld("CPM", "PLLCTRL0", "BYPASS", pllbypass))
    {
        pll0 = AddClock(exclk, "PLL0", FROM_PARENT, pllbypass ? 1 : 2 * pllm,
            pllbypass ? 1 : plln * (1 << pllod));
    }
    else
        pll0 = AddClock(exclk, "PLL0", INVALID);
    // PLL1
    soc_word_t plldiv, src_sel;
    QTreeWidgetItem *pll1 = 0;
    if(ReadFieldOld("CPM", "PLLCTRL1", "FEED_DIV", pllm) &&
            ReadFieldOld("CPM", "PLLCTRL1", "IN_DIV", plln) &&
            ReadFieldOld("CPM", "PLLCTRL1", "OUT_DIV", pllod) &&
            ReadFieldOld("CPM", "PLLCTRL1", "SRC_SEL", src_sel) &&
            ReadFieldOld("CPM", "PLLCTRL1", "PLL0_DIV", plldiv))
    {
        pll1 = AddClock(src_sel ? pll0 : exclk, "PLL1", FROM_PARENT, 2 * pllm,
            plln * (1 << pllod) * (src_sel ? plldiv : 1));
    }
    else
        pll1 = AddClock(exclk, "PLL1", INVALID);
    // system clocks
    const int NR_SYSCLK = 6;
    const char *sysclk[NR_SYSCLK] = { "CCLK", "SCLK", "PCLK", "HCLK", "H2CLK", "MCLK"};
    for(int i = 0; i < NR_SYSCLK; i++)
    {
        soc_word_t div = 0;
        std::string field = std::string(sysclk[i]) + "_DIV";
        if(ReadFieldOld("CPM", "SYSCLK", field.c_str(), div))
        {
            switch(div)
            {
                case 0: div = 1; break;
                case 1: div = 2; break;
                case 2: div = 3; break;
                case 3: div = 4; break;
                case 4: div = 6; break;
                case 5: div = 8; break;
                default: div = 0; break;
            }
        }
        if(div != 0)
            AddClock(pll0, sysclk[i], FROM_PARENT, 1, div);
        else
            AddClock(pll0, sysclk[i], INVALID);
    }
    // common to msc, i2s, lcd, uhc, otg, ssi, pcm, gpu, gps
    soc_word_t pll_div;
    if(ReadFieldOld("CPM", "SYSCLK", "PLL_DIV", pll_div))
        pll_div = pll_div ? 1 : 2;
    else
        pll_div = 1; // error
    // lcd
    soc_word_t pll_sel, div;
    if(ReadFieldOld("CPM", "LCDCLK", "DIV", div) &&
            ReadFieldOld("CPM", "LCDCLK", "PLL_SEL", pll_sel))
    {
        AddClock(pll_sel ? pll1 : pll0, "LCDCLK",
            FROM_PARENT, 1, pll_div * (div + 1));
    }
    else
        AddClock(exclk, "LCDCLK", INVALID);
}

void ClockAnalyser::FillTreeATJ213X()
{
    soc_word_t pllbypass, pllclk, en, coreclks, tmp0, tmp1, tmp2, tmp3;

    // system oscillators 32.768k and 24M
    QTreeWidgetItem *losc_clk = AddClock(0, "losc clk", 32768);
    QTreeWidgetItem *hosc_clk = AddClock(0, "hosc clk", 24000000);

    // core pll
    QTreeWidgetItem *corepll = 0;
    if(ReadFieldOld("CMU", "COREPLL", "CPEN", en) &&
        ReadFieldOld("CMU", "COREPLL", "CPBY", pllbypass) &&
        ReadFieldOld("CMU", "COREPLL", "CPCK", pllclk))
    {
        corepll = AddClock(hosc_clk, "core pll", en ? FROM_PARENT : DISABLED,
                           pllbypass ? 1 : pllclk, pllbypass ? 1 : 4);
    }
    else
    {
        corepll = AddClock(hosc_clk, "core pll", INVALID);
    }

    // dsp pll
    QTreeWidgetItem *dsppll = 0;
    if(ReadFieldOld("CMU", "DSPPLL", "DPEN", en) &&
        ReadFieldOld("CMU", "DSPPLL", "DPCK", pllclk))
    {
        dsppll = AddClock(hosc_clk, "dsp pll", en ? FROM_PARENT : DISABLED,
                           pllbypass ? 1 : pllclk, pllbypass ? 1 : 4);
    }
    else
    {
        dsppll = AddClock(hosc_clk, "dsp pll", INVALID);
    }

    // audio pll
    QTreeWidgetItem *adcpll = 0;
    QTreeWidgetItem *dacpll = 0;
    if(ReadFieldOld("CMU", "AUDIOPLL", "APEN", en) &&
        ReadFieldOld("CMU", "AUDIOPLL", "ADCCLK", tmp0) &&
        ReadFieldOld("CMU", "AUDIOPLL", "DACCLK", tmp1))
    {
        if(en)
        {
            adcpll = AddClock(hosc_clk, "audio adc pll", tmp0 ? 22579200 : 24576000);
            dacpll = AddClock(hosc_clk, "audio dac pll", tmp1 ? 22579200 : 24576000);
        }
        else
        {
            adcpll = AddClock(hosc_clk, "audio adc pll", DISABLED);
            dacpll = AddClock(hosc_clk, "audio dac pll", DISABLED);
        }
    }
    else
    {
        adcpll = AddClock(hosc_clk, "audio adc pll", INVALID);
        dacpll = AddClock(hosc_clk, "audio dac pll", INVALID);
    }

    // audio clocks
    QTreeWidgetItem *adcclk = 0;
    QTreeWidgetItem *dacclk = 0;
    if(ReadFieldOld("CMU", "AUDIOPLL", "ADCCLK", tmp0) &&
        ReadFieldOld("CMU", "AUDIOPLL", "DACCLK", tmp1))
    {
        adcclk = AddClock(adcpll, "audio adc clk", FROM_PARENT, 1, tmp0+1);
        dacclk = AddClock(dacpll, "audio dac clk", FROM_PARENT, 1, tmp1+1);
    }
    else
    {
        adcclk = AddClock(adcpll, "audio adc clk", INVALID);
        dacclk = AddClock(adcpll, "audio dac clk", INVALID);
    }

    // cpu clock
    QTreeWidgetItem *cpuclk = 0;
    if(ReadFieldOld("CMU", "BUSCLK", "CORECLKS", coreclks) &&
        ReadFieldOld("CMU", "BUSCLK", "CCLKDIV", tmp0))
    {
        if(coreclks == 0)
            cpuclk = AddClock(losc_clk, "cpu clk", FROM_PARENT, 1, tmp0+1);
        else if(coreclks == 1)
            cpuclk = AddClock(hosc_clk, "cpu clk", FROM_PARENT, 1, tmp0+1);
        else if(coreclks == 2)
            cpuclk = AddClock(corepll, "cpu clk", FROM_PARENT, 1, tmp0+1);
        else
            cpuclk = AddClock(corepll, "cpu clk", INVALID);
    }
    else
    {
        cpuclk = AddClock(corepll, "cpu clk", INVALID);
    }

    // system clock
    QTreeWidgetItem *sysclk = 0;
    if(ReadFieldOld("CMU", "BUSCLK", "SCLKDIV", tmp0))
        sysclk = AddClock(cpuclk, "system clk", FROM_PARENT, 1, tmp0+1);
    else
        sysclk = AddClock(cpuclk, "system clk", INVALID);

    // peripherial clk
    QTreeWidgetItem *pclk = 0;
    if(ReadFieldOld("CMU", "BUSCLK", "PCLKDIV", tmp0))
        pclk = AddClock(sysclk, "peripherial clk", FROM_PARENT, 1, tmp0 ? tmp0+1 : 2);
    else
        pclk = AddClock(sysclk, "peripherial clk", INVALID);

    // sdram clk
    QTreeWidgetItem *sdrclk = 0;
    if(ReadFieldOld("CMU", "DEVCLKEN", "SDRC", en) &&
        ReadFieldOld("CMU", "DEVCLKEN", "SDRM", tmp0) &&
        ReadFieldOld("SDR", "EN", "EN", tmp1) &&
        ReadFieldOld("CMU", "SDRCLK", "SDRDIV", tmp2))
    {
        en &= tmp0 & tmp1;
        sdrclk = AddClock(sysclk, "sdram clk", en ? FROM_PARENT: DISABLED, 1, tmp2+1);
    }
    else
        sdrclk = AddClock(sysclk, "sdram clk", INVALID);

    // nand clk
    QTreeWidgetItem *nandclk = 0;
    if(ReadFieldOld("CMU", "DEVCLKEN", "NAND", en) &&
        ReadFieldOld("CMU", "NANDCLK", "NANDDIV", tmp0))
        nandclk = AddClock(corepll, "nand clk", en ? FROM_PARENT : DISABLED, 1, tmp0+1);
    else
        nandclk = AddClock(corepll, "nand clk", INVALID);

    // sd clk
    QTreeWidgetItem *sdclk = 0;
    if(ReadFieldOld("CMU", "DEVCLKEN", "SD", tmp0) &&
        ReadFieldOld("CMU", "SDCLK", "CKEN" , tmp1) &&
        ReadFieldOld("CMU", "SDCLK", "D128" , tmp2) &&
        ReadFieldOld("CMU", "SDCLK", "SDDIV" , tmp3))
    {
        en = tmp0 & tmp1;
        sdclk = AddClock(corepll, "sd clk", en ? FROM_PARENT : DISABLED,
                         1, tmp2 ? 128*(tmp3+1) : (tmp3));
    }
    else
        sdclk = AddClock(corepll, "sd clk", INVALID);

    // mha clk
    QTreeWidgetItem *mhaclk = 0;
    if(ReadFieldOld("CMU", "DEVCLKEN", "MHA", en) &&
        ReadFieldOld("CMU", "MHACLK", "MHADIV", tmp1))
        mhaclk = AddClock(corepll, "mha clk", en ? FROM_PARENT : DISABLED,
                          1, tmp1+1);
    else
        mhaclk = AddClock(corepll, "mha clk", INVALID);

    // mca clk
    QTreeWidgetItem *mcaclk = 0;
    if(ReadFieldOld("CMU", "DEVCLKEN", "MCA", en) &&
        ReadFieldOld("CMU", "MCACLK", "MCADIV", tmp1))
        mcaclk = AddClock(corepll, "mca clk", en ? FROM_PARENT : DISABLED,
                          1, tmp1+1);
    else
        mcaclk = AddClock(corepll, "mca clk", INVALID);

    // backlight pwm
    QTreeWidgetItem *pwmclk = 0;
    if(ReadFieldOld("CMU", "FMCLK", "BCKE", en) &&
        ReadFieldOld("CMU", "FMCLK", "BCKS", tmp1) &&
        ReadFieldOld("CMU", "FMCLK", "BCKCON", tmp2))
    {
        if(tmp1)
        {
            // HOSC/8 input clk
            pwmclk = AddClock(hosc_clk, "pwm clk", en ? FROM_PARENT : DISABLED,
                              1, 3*(tmp2+1));
        }
        else
        {
            // LOSC input clk
            pwmclk = AddClock(losc_clk, "pwm clk", en ? FROM_PARENT : DISABLED,
                              1, tmp2+1);
        }
    }
    else
        pwmclk = AddClock(losc_clk, "pwm clk", INVALID);

    // i2c clk
    QTreeWidgetItem *i2c1clk = 0;
    QTreeWidgetItem *i2c2clk = 0;
    if(ReadFieldOld("CMU", "DEVCLKEN", "I2C", en) &&
        ReadFieldOld("I2C1", "CTL", "EN", tmp0) &&
        ReadFieldOld("I2C1", "CLKDIV", "CLKDIV", tmp1))
    {
        en &= tmp0;
        i2c1clk = AddClock(pclk, "i2c1 clk", en ? FROM_PARENT : DISABLED,
                           1, 16*(tmp1+1));
    }
    else
    {
        i2c1clk = AddClock(pclk, "i2c1 clk", INVALID);
    }

    if(ReadFieldOld("CMU", "DEVCLKEN", "I2C", en) &&
        ReadFieldOld("I2C2", "CTL", "EN", tmp0) &&
        ReadFieldOld("I2C2", "CLKDIV", "CLKDIV", tmp1))
    {
        en &= tmp0;
        i2c2clk = AddClock(pclk, "i2c2 clk", en ? FROM_PARENT : DISABLED,
                           1, 16*(tmp1+1));
    }
    else
    {
        i2c2clk = AddClock(pclk, "i2c2 clk", INVALID);
    }

    Q_UNUSED(dsppll);
    Q_UNUSED(adcclk);
    Q_UNUSED(dacclk);
    Q_UNUSED(sdrclk);
    Q_UNUSED(nandclk);
    Q_UNUSED(sdclk);
    Q_UNUSED(mhaclk);
    Q_UNUSED(mcaclk);
    Q_UNUSED(pwmclk);
    Q_UNUSED(i2c1clk);
    Q_UNUSED(i2c2clk);
}

void ClockAnalyser::FillTreeRK27XX()
{
    soc_word_t value, value2, value3, value4;
    soc_word_t bypass, clkr, clkf, clkod, pll_off;

    QTreeWidgetItem *xtal_clk = AddClock(0, "xtal clk", 24000000);

    // F = (Fref*F)/R/OD = (Fref*F)/R/OD
    QTreeWidgetItem *arm_pll = 0;
    if(ReadFieldOld("SCU", "PLLCON1", "ARM_PLL_BYPASS", bypass) &&
        ReadFieldOld("SCU", "PLLCON1", "ARM_PLL_CLKR", clkr) &&
        ReadFieldOld("SCU", "PLLCON1", "ARM_PLL_CLKF", clkf) &&
        ReadFieldOld("SCU", "PLLCON1", "ARM_PLL_CLKOD", clkod) &&
        ReadFieldOld("SCU", "PLLCON1", "ARM_PLL_POWERDOWN", pll_off))
    {
        arm_pll = AddClock(xtal_clk, "arm pll", pll_off ? DISABLED : FROM_PARENT,
                           bypass ? 1 : clkf+1, bypass ? 1 : (clkr+1)*(clkod+1));
    }
    else
    {
        arm_pll = AddClock(xtal_clk, "arm pll", INVALID);
    }

    QTreeWidgetItem *arm_clk = 0;
    QTreeWidgetItem *hclk = 0;
    QTreeWidgetItem *pclk = 0;
    if(ReadFieldOld("SCU", "DIVCON1", "ARM_SLOW_MODE", value) &&
       ReadFieldOld("SCU", "DIVCON1", "ARM_CLK_DIV", value2) &&
       ReadFieldOld("SCU", "DIVCON1", "PCLK_CLK_DIV", value3))
    {
        arm_clk = AddClock(value ? xtal_clk : arm_pll, "arm clk", FROM_PARENT, 1, value2 ? 2 : 1);
        hclk = AddClock(arm_clk, "hclk", FROM_PARENT, 1, value2 ? 1 : 2);
        pclk = AddClock(hclk, "pclk", FROM_PARENT, 1, (1<<value3));
    }
    else
    {
        arm_clk = AddClock(xtal_clk, "arm_clk", INVALID);
        hclk = AddClock(xtal_clk, "hclk", INVALID);
        pclk = AddClock(xtal_clk, "pclk", INVALID);
    }

    QTreeWidgetItem *dsp_pll = 0;
    if(ReadFieldOld("SCU", "PLLCON2", "DSP_PLL_BYPASS", bypass) &&
        ReadFieldOld("SCU", "PLLCON2", "DSP_PLL_CLKR", clkr) &&
        ReadFieldOld("SCU", "PLLCON2", "DSP_PLL_CLKF", clkf) &&
        ReadFieldOld("SCU", "PLLCON2", "DSP_PLL_CLKOD", clkod) &&
        ReadFieldOld("SCU", "PLLCON2", "DSP_PLL_POWERDOWN", pll_off))
    {
        dsp_pll = AddClock(xtal_clk, "dsp pll", pll_off ? DISABLED : FROM_PARENT,
                           bypass ? 1 : clkf+1, bypass ? 1 : (clkr+1)*(clkod+1));
    }
    else
    {
        dsp_pll = AddClock(xtal_clk, "dsp_pll", INVALID);
    }

    QTreeWidgetItem *dsp_clk = AddClock(dsp_pll, "dsp clk", FROM_PARENT);

    QTreeWidgetItem *codec_pll = 0;
    if(ReadFieldOld("SCU", "PLLCON3", "CODEC_PLL_BYPASS", bypass) &&
       ReadFieldOld("SCU", "PLLCON3", "CODEC_PLL_CLKR", clkr) &&
       ReadFieldOld("SCU", "PLLCON3", "CODEC_PLL_CLKF", clkf) &&
       ReadFieldOld("SCU", "PLLCON3", "CODEC_PLL_CLKOD", clkod) &&
       ReadFieldOld("SCU", "PLLCON3", "CODEC_PLL_POWERDOWN", pll_off))
    {
        codec_pll = AddClock(xtal_clk, "codec pll", pll_off ? DISABLED : FROM_PARENT,
                             bypass ? 1 : clkf+1, bypass ? 1 : (clkr+1)*(clkod+1));
    }
    else
    {
        codec_pll = AddClock(xtal_clk, "codec_pll", INVALID);
    }

    QTreeWidgetItem *codec_clk = 0;
    if(ReadFieldOld("SCU", "DIVCON1", "CODEC_CLK_SRC", value) &&
        ReadFieldOld("SCU", "DIVCON1", "CODEC_CLK_DIV", value2))
    {
        codec_clk = AddClock(value ? xtal_clk : codec_pll, "codec clk", FROM_PARENT, 1, value ? 1 : (value2 + 1));
    }
    else
    {
        codec_clk = AddClock(xtal_clk, "codec_clk", INVALID);
    }

    QTreeWidgetItem *lsadc_clk = 0;
    if(ReadFieldOld("SCU", "DIVCON1", "LSADC_CLK_DIV", value))
    {
        lsadc_clk = AddClock(pclk, "lsadc clk", FROM_PARENT, 1, (value+1));
    }
    else
    {
        lsadc_clk = AddClock(xtal_clk, "lsadc clk", INVALID);
    }

    QTreeWidgetItem *lcdc_clk = 0;
    if(ReadFieldOld("SCU", "DIVCON1", "LCDC_CLK", value) &&
        ReadFieldOld("SCU", "DIVCON1", "LCDC_CLK_DIV", value2) &&
        ReadFieldOld("SCU", "DIVCON1", "LCDC_CLK_DIV_SRC", value3))
    {
        if(value)
        {
            lcdc_clk = AddClock(xtal_clk, "lcdc clk", FROM_PARENT);
        }
        else
        {
            if(value3 == 0)
                lcdc_clk = AddClock(arm_pll, "lcdc clk", FROM_PARENT, 1, value2+1);
            else if(value3 == 1)
                lcdc_clk = AddClock(dsp_pll, "lcdc clk", FROM_PARENT, 1, value2+1);
            else
                lcdc_clk = AddClock(codec_pll, "lcdc clk", FROM_PARENT, 1, value2+1);
        }
    }
    else
    {
        lcdc_clk = AddClock(xtal_clk, "lcdc clk", INVALID);
    }

    QTreeWidgetItem *pwm0_clk = 0;
    if(ReadFieldOld("PWM0", "LRC", "TR", value) &&
       ReadFieldOld("PWM0", "CTRL", "PRESCALE", value3) &&
       ReadFieldOld("PWM0", "CTRL", "PWM_EN", value4))
    {
        pwm0_clk = AddClock(pclk, "pwm0 clk", value4 ? FROM_PARENT : DISABLED, 1, 2*value*(1<<value3));
    }
    else
    {
        pwm0_clk = AddClock(xtal_clk, "pwm0 clk", INVALID);
    }

    QTreeWidgetItem *pwm1_clk = 0;
    if(ReadFieldOld("PWM1", "LRC", "TR", value) &&
       ReadFieldOld("PWM1", "CTRL", "PRESCALE", value3) &&
       ReadFieldOld("PWM1", "CTRL", "PWM_EN", value4))
    {
        pwm1_clk = AddClock(pclk, "pwm1 clk", value4 ? FROM_PARENT : DISABLED, 1, 2*value*(1<<value3));
    }
    else
    {
        pwm1_clk = AddClock(xtal_clk, "pwm1 clk", INVALID);
    }

    QTreeWidgetItem *pwm2_clk = 0;
    if(ReadFieldOld("PWM2", "LRC", "TR", value) &&
       ReadFieldOld("PWM2", "CTRL", "PRESCALE", value3) &&
       ReadFieldOld("PWM2", "CTRL", "PWM_EN", value4))
    {
        pwm2_clk = AddClock(pclk, "pwm2 clk", value4 ? FROM_PARENT : DISABLED, 1, 2*value*(1<<value3));
    }
    else
    {
        pwm2_clk = AddClock(xtal_clk, "pwm2 clk", INVALID);
    }

    QTreeWidgetItem *pwm3_clk = 0;
    if(ReadFieldOld("PWM3", "LRC", "TR", value) &&
       ReadFieldOld("PWM3", "CTRL", "PRESCALE", value3) &&
       ReadFieldOld("PWM3", "CTRL", "PWM_EN", value4))
    {
        pwm3_clk = AddClock(pclk, "pwm3", value4 ? FROM_PARENT : DISABLED, 1, 2*value*(1<<value3));
    }
    else
    {
        pwm3_clk = AddClock(xtal_clk, "pwm3 clk", INVALID);
    }

    QTreeWidgetItem *sdmmc_clk = 0;
    if(ReadFieldOld("SD", "CTRL", "DIVIDER", value))
    {
        sdmmc_clk = AddClock(pclk, "sd clk", FROM_PARENT, 1, value+1);
    }
    else
    {
        sdmmc_clk = AddClock(xtal_clk, "sd clk", INVALID);
    }

    Q_UNUSED(dsp_clk);
    Q_UNUSED(codec_clk);
    Q_UNUSED(lsadc_clk);
    Q_UNUSED(lcdc_clk);
    Q_UNUSED(pwm0_clk);
    Q_UNUSED(pwm1_clk);
    Q_UNUSED(pwm2_clk);
    Q_UNUSED(pwm3_clk);
    Q_UNUSED(sdmmc_clk);
}

void ClockAnalyser::FillTreeIMX233()
{
    soc_word_t value, value2, value3;

    QTreeWidgetItem *ring_osc = 0;
    if(ReadFieldOld("POWER", "MINPWR", "ENABLE_OSC", value))
        ring_osc = AddClock(0, "ring_clk24m", value ? 24000000 : DISABLED);
    else
        ring_osc = AddClock(0, "ring_clk24m", INVALID);
    QTreeWidgetItem *xtal_osc = 0;
    if(ReadFieldOld("POWER", "MINPWR", "PWD_XTAL24", value))
        xtal_osc = AddClock(0, "xtal_clk24m", value ? DISABLED : 24000000);
    else
        xtal_osc = AddClock(0, "xtal_clk24m", INVALID);
    QTreeWidgetItem *ref_xtal = 0;
    if(ReadFieldOld("POWER", "MINPWR", "SELECT_OSC", value))
        ref_xtal = AddClock(value ? ring_osc : xtal_osc, "ref_xtal", FROM_PARENT);
    else
        ref_xtal = AddClock(0, "ref_xtal", INVALID);

    QTreeWidgetItem *ref_pll = 0;
    if(ReadFieldOld("CLKCTRL", "PLLCTRL0", "POWER", value))
        ref_pll = AddClock(ref_xtal, "ref_pll", FROM_PARENT, 20);
    else
        ref_pll = AddClock(0, "ref_pll", INVALID);

    QTreeWidgetItem *ref_io = 0;
    if(ReadFieldOld("CLKCTRL", "FRAC", "CLKGATEIO", value) &&
            ReadFieldOld("CLKCTRL", "FRAC", "IOFRAC", value2))
        ref_io = AddClock(ref_pll, "ref_io", value ? DISABLED : FROM_PARENT, 18, value2);
    else
        ref_io = AddClock(ref_pll, "ref_io", INVALID);

    QTreeWidgetItem *ref_pix = 0;
    if(ReadFieldOld("CLKCTRL", "FRAC", "CLKGATEPIX", value) &&
            ReadFieldOld("CLKCTRL", "FRAC", "PIXFRAC", value2))
        ref_pix = AddClock(ref_pll, "ref_pix", value ? DISABLED : FROM_PARENT, 18, value2);
    else
        ref_pix = AddClock(ref_pll, "ref_pix", INVALID);

    QTreeWidgetItem *ref_emi = 0;
    if(ReadFieldOld("CLKCTRL", "FRAC", "CLKGATEEMI", value) &&
            ReadFieldOld("CLKCTRL", "FRAC", "EMIFRAC", value2))
        ref_emi = AddClock(ref_pll, "ref_emi", value ? DISABLED : FROM_PARENT, 18, value2);
    else
        ref_emi = AddClock(ref_pll, "ref_emi", INVALID);

    QTreeWidgetItem *ref_cpu = 0;
    if(ReadFieldOld("CLKCTRL", "FRAC", "CLKGATECPU", value) &&
            ReadFieldOld("CLKCTRL", "FRAC", "CPUFRAC", value2))
        ref_cpu = AddClock(ref_pll, "ref_cpu", value ? DISABLED : FROM_PARENT, 18, value2);
    else
        ref_cpu = AddClock(ref_pll, "ref_cpu", INVALID);

    QTreeWidgetItem *clk_p = 0;
    if(ReadFieldOld("CLKCTRL", "CLKSEQ", "BYPASS_CPU", value))
    {
        if(!value)
        {
            if(ReadFieldOld("CLKCTRL", "CPU", "DIV_CPU", value2))
                clk_p = AddClock(ref_cpu, "clk_p", FROM_PARENT, 1, value2);
            else
                clk_p = AddClock(ref_cpu, "clk_p", INVALID);
        }
        else
        {
            if(ReadFieldOld("CLKCTRL", "CPU", "DIV_XTAL_FRAC_EN", value) &&
                    ReadFieldOld("CLKCTRL", "CPU", "DIV_XTAL", value2))
                clk_p = AddClock(ref_xtal, "clk_p", FROM_PARENT, value ? 1024 : 1, value2);
            else
                clk_p = AddClock(ref_xtal, "clk_p", INVALID);
        }
    }
    else
        clk_p = AddClock(ref_xtal, "clk_p", INVALID);

    QTreeWidgetItem *clk_h = 0;
    if(ReadFieldOld("CLKCTRL", "HBUS", "DIV_FRAC_EN", value) &&
            ReadFieldOld("CLKCTRL", "HBUS", "DIV", value2))
        clk_h = AddClock(clk_p, "clk_h", FROM_PARENT, value ? 32 : 1, value2);
    else
        clk_h = AddClock(clk_p, "clk_h", INVALID);

    QTreeWidgetItem *clk_x = 0;
    if(ReadFieldOld("CLKCTRL", "XBUS", "DIV", value))
        clk_x = AddClock(ref_xtal, "clk_x", FROM_PARENT, 1, value);
    else
        clk_x = AddClock(ref_xtal, "clk_x", INVALID);

    if(ReadFieldOld("CLKCTRL", "XTAL", "UART_CLK_GATE", value))
        AddClock(ref_xtal, "clk_uart", value ? DISABLED : FROM_PARENT);
    else
        AddClock(ref_xtal, "clk_uart", INVALID);

    if(ReadFieldOld("CLKCTRL", "XTAL", "FILT_CLK24M_GATE", value))
        AddClock(ref_xtal, "clk_filt24m", value ? DISABLED : FROM_PARENT);
    else
        AddClock(ref_xtal, "clk_filt24m", INVALID);

    if(ReadFieldOld("CLKCTRL", "XTAL", "PWM_CLK24M_GATE", value))
        AddClock(ref_xtal, "clk_pwm24m", value ? DISABLED : FROM_PARENT);
    else
        AddClock(ref_xtal, "clk_pwm24m", INVALID);

    if(ReadFieldOld("CLKCTRL", "XTAL", "DRI_CLK24M_GATE", value))
        AddClock(ref_xtal, "clk_dri24m", value ? DISABLED : FROM_PARENT);
    else
        AddClock(ref_xtal, "clk_dri24m", INVALID);

    if(ReadFieldOld("CLKCTRL", "XTAL", "DIGCTRL_CLK1M_GATE", value))
        AddClock(ref_xtal, "clk_1m", value ? DISABLED : FROM_PARENT, 1, 24);
    else
        AddClock(ref_xtal, "clk_1m", INVALID);

    QTreeWidgetItem *clk_32k = 0;
    if(ReadFieldOld("CLKCTRL", "XTAL", "TIMROT_CLK32K_GATE", value))
        clk_32k = AddClock(ref_xtal, "clk_32k", value ? DISABLED : FROM_PARENT, 1, 750);
    else
        clk_32k = AddClock(ref_xtal, "clk_32k", INVALID);

    AddClock(clk_32k, "clk_adc", FROM_PARENT, 1, 16);

    if(ReadFieldOld("CLKCTRL", "CLKSEQ", "BYPASS_PIX", value) &&
            ReadFieldOld("CLKCTRL", "PIX", "DIV", value2))
        AddClock(value ? ref_xtal : ref_pix, "clk_pix", FROM_PARENT, 1, value2);
    else
        AddClock(ref_xtal, "clk_p", INVALID);

    QTreeWidgetItem *clk_ssp = 0;
    if(ReadFieldOld("CLKCTRL", "CLKSEQ", "BYPASS_SSP", value) &&
            ReadFieldOld("CLKCTRL", "SSP", "DIV", value2) &&
            ReadFieldOld("CLKCTRL", "SSP", "CLKGATE", value3))
        clk_ssp = AddClock(value ? ref_xtal : ref_io, "clk_ssp", value3 ? DISABLED : FROM_PARENT, 1, value2);
    else
        clk_ssp = AddClock(ref_xtal, "clk_p", INVALID);

    if(ReadFieldOld("SSP1", "TIMING", "CLOCK_DIVIDE", value) &&
            ReadFieldOld("SSP1", "TIMING", "CLOCK_RATE", value2) &&
            ReadFieldOld("SSP1", "CTRL0", "CLKGATE", value3))
        AddClock(clk_ssp, "clk_ssp1", value3 ? DISABLED : FROM_PARENT, 1, value * (1 + value2));
    else
        AddClock(clk_ssp, "clk_ssp1", INVALID);

    if(ReadFieldOld("SSP2", "TIMING", "CLOCK_DIVIDE", value) &&
            ReadFieldOld("SSP2", "TIMING", "CLOCK_RATE", value2) &&
            ReadFieldOld("SSP2", "CTRL0", "CLKGATE", value3))
        AddClock(clk_ssp, "clk_ssp2", value3 ? DISABLED : FROM_PARENT, 1, value * (1 + value2));
    else
        AddClock(clk_ssp, "clk_ssp2", INVALID);

    QTreeWidgetItem *clk_gpmi = 0;
    if(ReadFieldOld("CLKCTRL", "CLKSEQ", "BYPASS_GPMI", value) &&
            ReadFieldOld("CLKCTRL", "GPMI", "DIV", value2) &&
            ReadFieldOld("CLKCTRL", "GPMI", "CLKGATE", value3))
        clk_gpmi = AddClock(value ? ref_xtal : ref_io, "clk_gpmi", value3 ? DISABLED : FROM_PARENT, 1, value2);
    else
        clk_gpmi = AddClock(ref_xtal, "clk_p", INVALID);

    if(ReadFieldOld("CLKCTRL", "CLKSEQ", "BYPASS_EMI", value))
    {
        if(!value)
        {
            if(ReadFieldOld("CLKCTRL", "EMI", "DIV_EMI", value2) &&
                    ReadFieldOld("CLKCTRL", "EMI", "CLKGATE", value3))
                AddClock(ref_emi, "clk_emi", value3 ? DISABLED : FROM_PARENT, 1, value2);
            else
                AddClock(ref_emi, "clk_emi", INVALID);
        }
        else
        {
            if(ReadFieldOld("CLKCTRL", "EMI", "DIV_XTAL", value2) &&
                    ReadFieldOld("CLKCTRL", "EMI", "CLKGATE", value3))
                AddClock(ref_xtal, "clk_emi", value3 ? DISABLED : FROM_PARENT, 1, value2);
            else
                AddClock(ref_xtal, "clk_emi", INVALID);
        }
    }
    else
        clk_p = AddClock(ref_xtal, "clk_emi", INVALID);

    QTreeWidgetItem *ref_vid = AddClock(ref_pll, "clk_vid", FROM_PARENT);

    if(ReadFieldOld("CLKCTRL", "TV", "CLK_TV108M_GATE", value) &&
            ReadFieldOld("CLKCTRL", "TV", "CLK_TV_GATE", value2))
    {
        QTreeWidgetItem *clk_tv108m = AddClock(ref_vid, "clk_tv108m", value ? DISABLED : FROM_PARENT, 1, 4);
        AddClock(clk_tv108m, "clk_tv54m", value2 ? DISABLED : FROM_PARENT, 1, 2);
        AddClock(clk_tv108m, "clk_tv27m", value2 ? DISABLED : FROM_PARENT, 1, 4);
    }

    if(ReadFieldOld("CLKCTRL", "PLLCTRL0", "EN_USB_CLKS", value))
        AddClock(ref_pll, "utmi_clk480m", value ? FROM_PARENT : DISABLED);
    else
        AddClock(ref_pll, "utmi_clk480m", INVALID);

    QTreeWidgetItem *xtal_clk32k = 0;
    if(ReadFieldOld("RTC", "PERSISTENT0", "XTAL32_FREQ", value) &&
            ReadFieldOld("RTC", "PERSISTENT0", "XTAL32KHZ_PWRUP", value2))
        xtal_clk32k = AddClock(0, "xtal_clk32k", value2 == 0 ? DISABLED : value ? 32000 : 32768);
    else
        xtal_clk32k = AddClock(0, "xtal_clk32k", INVALID);

    if(ReadFieldOld("RTC", "PERSISTENT0", "CLOCKSOURCE", value))
        AddClock(value ? xtal_clk32k : ref_xtal, "clk_rtc32k", FROM_PARENT, 1, value ? 1 : 768);
    else
        AddClock(ref_xtal, "clk_rtc32k", INVALID);

    Q_UNUSED(clk_x);
    Q_UNUSED(clk_gpmi);
    Q_UNUSED(clk_h);
}

static TmplAnalyserFactory< ClockAnalyser > g_clock_factory(true, "Clock Analyser");

/**
 * EMI analyser
 */
EmiAnalyser::EmiAnalyser(const soc_desc::soc_ref_t& soc, IoBackend *backend)
    :AnalyserEx(soc, backend)
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
    soc_word_t value;

    m_emi_freq = 0;
    if(ReadFieldOld("CLKCTRL", "CLKSEQ", "BYPASS_EMI", value))
    {
        bool ret;
        if(value)
        {
            m_emi_freq = 24000000;
            ret = ReadFieldOld("CLKCTRL", "EMI", "DIV_XTAL", value);
        }
        else
        {
            m_emi_freq = 480000000;
            if(ReadFieldOld("CLKCTRL", "FRAC", "EMIFRAC", value))
                m_emi_freq = 18 * (int64_t)m_emi_freq / value;
            else
                m_emi_freq = 0;
            ret = ReadFieldOld("CLKCTRL", "EMI", "DIV_EMI", value);
        }
        if(ret)
            m_emi_freq /= value;
        else
            m_emi_freq = 0;
    }

    m_emi_freq_label->setText(QString().sprintf("%.3f", m_emi_freq / 1000000.0));

    NewGroup("Control Parameters");
    if(ReadFieldOld("EMI", "CTRL", "PORT_PRIORITY_ORDER", value))
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

    if(ReadFieldOld("EMI", "CTRL", "MEM_WIDTH", value))
        AddLine("Memory Width", value ? 16 : 8, "-bit");

    if(ReadFieldOld("DRAM", "CTL03", "AP", value))
        AddLine("Auto Pre-Charge", NONE, value ? "Yes" : "No");

    bool bypass_mode = false;
    if(ReadFieldOld("DRAM", "CTL04", "DLL_BYPASS_MODE", value))
    {
        bypass_mode = value == 1;
        AddLine("DLL Bypass Mode", NONE, value ? "Yes" : "No");
    }

    if(ReadFieldOld("DRAM", "CTL05", "EN_LOWPOWER_MODE", value))
        AddLine("Low Power Mode", NONE, value ? "Enabled" : "Disabled");

    if(ReadFieldOld("DRAM", "CTL08", "SREFRESH", value))
        AddLine("Self Refresh", NONE, value ? "Yes" : "No");

    if(ReadFieldOld("DRAM", "CTL08", "SDR_MODE", value))
        AddLine("Mode", NONE, value ? "SDR" : "DDR");

    if(ReadFieldOld("DRAM", "CTL10", "ADDR_PINS", value))
        AddLine("Address Pins", 13 - value, "");

    if(ReadFieldOld("DRAM", "CTL11", "COLUMN_SIZE", value))
        AddLine("Column Size", 12 - value, "-bit");

    if(ReadFieldOld("DRAM", "CTL11", "CASLAT", value))
        AddLine("Encoded CAS", value, "", "Memory device dependent");

    if(ReadFieldOld("DRAM", "CTL14", "CS_MAP", value))
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

    if(ReadFieldOld("DRAM", "CTL37", "TREF_ENABLE", value))
        AddLine("Refresh Commands", NONE, value ? "Enabled" : "Disabled", "Issue self-refresh every TREF cycles");

    NewGroup("Frequency Parameters");

    if(ReadFieldOld("DRAM", "CTL13", "CASLAT_LIN_GATE", value))
    {
        if(value >= 3 && value <= 10 && value != 9)
        {
            float v = (value / 2) + 0.5 * (value % 2);
            AddCycleLine("CAS Gate", value, v, 1, "");
        }
        else
            AddLine("CAS Gate", NONE, "Reserved", "Reserved value");
    }
    if(ReadFieldOld("DRAM", "CTL13", "CASLAT_LIN", value))
    {
        if(value >= 3 && value <= 10 && value != 9)
        {
            float v = (value / 2) + 0.5 * (value % 2);
            AddCycleLine("CAS Latency", value, v, 1, "");
        }
        else
            AddLine("CAS Latency", NONE, "Reserved", "Reserved value");
    }

    if(ReadFieldOld("DRAM", "CTL12", "TCKE", value))
        AddCycleLine("tCKE", value, value, 0, "Minimum CKE pulse width");

    if(ReadFieldOld("DRAM", "CTL15", "TDAL", value))
        AddCycleLine("tDAL", value, value, 0, "Auto pre-charge write recovery time");

    if(ReadFieldOld("DRAM", "CTL31", "TDLL", value))
        AddCycleLine("tDLL", value, value, 0, "DLL lock time");

    if(ReadFieldOld("DRAM", "CTL10", "TEMRS", value))
        AddCycleLine("tEMRS", value, value, 0, "Extended mode parameter set time");

    if(ReadFieldOld("DRAM", "CTL34", "TINIT", value))
        AddCycleLine("tINIT", value, value, 0, "Initialisation time");

    if(ReadFieldOld("DRAM", "CTL16", "TMRD", value))
        AddCycleLine("tMRD", value, value, 0, "Mode register set command time");

    if(ReadFieldOld("DRAM", "CTL40", "TPDEX", value))
        AddCycleLine("tPDEX", value, value, 0, "Power down exit time");

    if(ReadFieldOld("DRAM", "CTL32", "TRAS_MAX", value))
        AddCycleLine("tRAS Max", value, value, 0, "Maximum row activate time");

    if(ReadFieldOld("DRAM", "CTL20", "TRAS_MIN", value))
        AddCycleLine("tRAS Min", value, value, 0, "Minimum row activate time");

    if(ReadFieldOld("DRAM", "CTL17", "TRC", value))
        AddCycleLine("tRC", value, value, 0, "Activate to activate delay (same bank)");

    if(ReadFieldOld("DRAM", "CTL20", "TRCD_INT", value))
        AddCycleLine("tRCD", value, value, 0, "RAS to CAS");

    if(ReadFieldOld("DRAM", "CTL26", "TREF", value))
        AddCycleLine("tREF", value, value, 0, "Refresh to refresh time");

    if(ReadFieldOld("DRAM", "CTL21", "TRFC", value))
        AddCycleLine("tRFC", value, value, 0, "Refresh command time");

    if(ReadFieldOld("DRAM", "CTL15", "TRP", value))
        AddCycleLine("tRP", value, value, 0, "Pre-charge command time");

    if(ReadFieldOld("DRAM", "CTL12", "TRRD", value))
        AddCycleLine("tRRD", value, value, 0, "Activate to activate delay (different banks)");

    if(ReadFieldOld("DRAM", "CTL12", "TWR_INT", value))
        AddCycleLine("tWR", value, value, 0, "Write recovery time");

    if(ReadFieldOld("DRAM", "CTL13", "TWTR", value))
        AddCycleLine("tWTR", value, value, 0, "Write to read delay");

    if(ReadFieldOld("DRAM", "CTL32", "TXSNR", value))
        AddCycleLine("tXSNR", value, value, 0, "");

    if(ReadFieldOld("DRAM", "CTL33", "TXSR", value))
        AddCycleLine("tXSR", value, value, 0, "Self-refresh exit time");

    NewGroup("DLL Parameters");

    if(bypass_mode)
    {
        if(ReadFieldOld("DRAM", "CTL19", "DLL_DQS_DELAY_BYPASS_0", value))
            AddLine("DLL DQS Delay 0", value, "", "In 1/128 fraction of a cycle (bypass mode)");

        if(ReadFieldOld("DRAM", "CTL19", "DLL_DQS_DELAY_BYPASS_0", value))
            AddLine("DLL DQS Delay 1", value, "", "In 1/128 fraction of a cycle (bypass mode)");

        if(ReadFieldOld("DRAM", "CTL19", "DQS_OUT_SHIFT_BYPASS", value))
            AddLine("DQS Out Delay", value, "", "(bypass mode)");

        if(ReadFieldOld("DRAM", "CTL20", "WR_DQS_SHIFT_BYPASS", value))
            AddLine("DQS Write Delay", value, "", "(bypass mode)");
    }
    else
    {
        if(ReadFieldOld("DRAM", "CTL17", "DLL_START_POINT", value))
            AddLine("DLL Start Point", value, "", "Initial delay count");

        if(ReadFieldOld("DRAM", "CTL17", "DLL_INCREMENT", value))
            AddLine("DLL Increment", value, "", "Delay increment");

        if(ReadFieldOld("DRAM", "CTL18", "DLL_DQS_DELAY_0", value))
            AddLine("DLL DQS Delay 0", value, "", "In 1/128 fraction of a cycle");

        if(ReadFieldOld("DRAM", "CTL18", "DLL_DQS_DELAY_1", value))
            AddLine("DLL DQS Delay 1", value, "", "In 1/128 fraction of a cycle");

        if(ReadFieldOld("DRAM", "CTL19", "DQS_OUT_SHIFT", value))
            AddLine("DQS Out Delay", value, "", "");

        if(ReadFieldOld("DRAM", "CTL20", "WR_DQS_SHIFT", value))
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

PinAnalyser::PinAnalyser(const soc_desc::soc_ref_t& soc, IoBackend *backend)
        :AnalyserEx(soc, backend)
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
    soc_word_t value;

    while(m_panel->count() > 0)
        m_panel->removeItem(0);

    const char *package_type[8] =
    {
        "bga169", "bga100", "lqfp100", "lqfp128", 0, 0, 0, 0
    };

    if(!ReadFieldOld("DIGCTL", "STATUS", "PACKAGE_TYPE", value))
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
            if(!ReadRegisterOld("PINCTRL", QString("MUXSELn[%1]").arg(bank * 2 + i), muxsel[i]))
                error = true;
        /* don't make an error for those since some do not exist */
        for(int i = 0; i < 4; i++)
            if(!ReadRegisterOld("PINCTRL", QString("DRIVEn[%1]").arg(bank * 4 + i), drive[i]))
                drive[i] = 0;
        if(error)
            continue;
        if(!ReadRegisterOld("PINCTRL", QString("PULLn[%1]").arg(bank), pull))
            pull = 0;
        if(!ReadRegisterOld("PINCTRL", QString("DINn[%1]").arg(bank), in))
            in = 0;
        if(!ReadRegisterOld("PINCTRL", QString("DOUTn[%1]").arg(bank), out))
            out = 0;
        if(!ReadRegisterOld("PINCTRL", QString("DOEn[%1]").arg(bank), oe))
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
