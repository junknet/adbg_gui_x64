#include "CPUView.h"
#include "log.h"
#include "mainwindow.h"
#include <QAbstractScrollArea>
#include <QApplication>
#include <QBrush>
#include <QClipboard>
#include <QDateTime>
#include <QInputDialog>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <boost/range/irange.hpp>
#include <capstone.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <qcolor.h>
#include <qdatetime.h>
#include <qdebug.h>
#include <qnamespace.h>
#include <qtmetamacros.h>

#include <QMenu>
DisassView::DisassView(QWidget *parent) : QAbstractScrollArea()
{
    auto font = QFont("FiraCode", 8);
    auto metrics = QFontMetrics(font);
    fontWidth_ = metrics.horizontalAdvance('X');
    fontHeight_ = metrics.height();
    QAbstractScrollArea::setFont(font);

    line1_ = fontWidth_ * line1_space;
    line2_ = line1_ + fontWidth_ * line2_space;
    line3_ = line2_ + fontWidth_ * line3_space;

    lineWidth_ = fontWidth_;

    cs_open(CS_ARCH_ARM64, CS_MODE_ARM, &handle_arm64);
    cs_option(handle_arm64, CS_OPT_SKIPDATA, CS_OPT_ON);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}
void DisassView::disassInstr()
{
    cs_free(insn, count);

    //  跳转到第一行地址
    uint64_t to_addr;
    if (jump_addr_)
    {
        to_addr = jump_addr_;
        logd("jump_addr_: 0x{:x}", jump_addr_);
        jump_addr_ = 0;
    }
    else
    {
        to_addr = pcValue_;
    }

    count = cs_disasm(handle_arm64, data, 0x400, startAddr_, 0, &insn);
    disStartAddr = insn[0].address;
    disEndAddr = insn[count - 1].address;
    auto maxvalue = count - viewport()->size().height() / fontHeight_;
    verticalScrollBar()->setMaximum(maxvalue);

    for (auto i : boost::irange(0, count))
    {
        auto ins_addr = insn[i].address;
        auto delta = ins_addr > to_addr ? (ins_addr - to_addr) : (to_addr - ins_addr);
        if (delta < 2)
        {
            verticalScrollBar()->setValue(i);
            break;
        }
    }
}

void DisassView::setCurrentPc(uint64_t addr)
{
    pcValue_ = addr;
    focusAddr = pcValue_;
    logd("currentPC: 0x{:x}", pcValue_);
    if (count)
    {
        // logd("start: 0x{:x}", insn[0].address);
        // logd("end: 0x{:x}", insn[count - 1].address);
        if (pcValue_ < screenStartAddr || pcValue_ > screenEndAddr)
        {
            emit msg_cpu_jump_sig(addr);
            logd("msg_cpu_sig: 0x{:x}", addr);
        }
    }
}

void DisassView::setProcessPaused(bool paused)
{
    paused_ = paused;
    viewport()->update();
}

void DisassView::setCurrentCPSR(uint64_t flag)
{
}

void DisassView::setStartAddr(uint64_t addr)
{
    startAddr_ = addr;
    logd("set startAddr_: 0x{:x}", startAddr_);
}

void DisassView::setDebugFlag(bool flag)
{
    debuged = flag;
}

void DisassView::paintEvent(QPaintEvent *event)
{
    if (!debuged)
    {
        return;
    }

    QSize areaSize = viewport()->size();
    auto offset = verticalScrollBar()->value();
    auto lines = areaSize.height() / fontHeight_;
    screenStartAddr = insn[offset].address;
    screenEndAddr = offset + lines >= count ? insn[count - 1].address : insn[offset + lines].address;
    if (screenStartAddr == disStartAddr || screenEndAddr == disEndAddr)
    {
        jumpTo(screenStartAddr);
        return;
    }

    QPainter painter(viewport());
    // painter.setPen(QPen(Qt::blue));

    int line_y = 0;
    for (auto line : boost::irange(0, lines))
    {
        if (line + offset >= count)
        {
            break;
        }

        line_y = line * fontHeight_;
        auto line_addr = insn[offset + line].address;

        // 先画出焦点行底色 背景色有先后顺序
        if (line_addr == focusAddr)
        {
            auto rect = QRectF(line1_, line_y, areaSize.width(), fontHeight_);
            auto brush = QBrush(focus_color);
            painter.fillRect(rect, brush);
        }

        //  断点指令用红色染色
        if (bpList_.contains(line_addr))
        {
            auto rect = QRectF(line1_, line_y, line2_ - line1_, fontHeight_);
            auto brush = QBrush(bp_bgcolor);
            painter.fillRect(rect, brush);
        }

        if (line_addr == pcValue_ && paused_)
        {

            if (!bpList_.contains(line_addr))
            {
                auto rect = QRectF(line1_, line_y, line2_ - line1_, fontHeight_);
                auto brush = QBrush(pc_bgcolor);
                painter.fillRect(rect, brush);
            }

            //  画出PC指示箭头
            auto pc_offset = 3;
            auto rect = QRectF(fontWidth_ * pc_offset, line_y, fontWidth_ * 2, fontHeight_);
            auto brush = QBrush(Qt::blue);
            painter.fillRect(rect, brush);
            painter.setPen(Qt::white);
            painter.drawText(fontWidth_ * pc_offset, line_y, fontWidth_ * 2, fontHeight_, Qt::AlignTop, "PC");
            painter.setPen(Qt::blue);
            painter.drawLine(fontWidth_ * (pc_offset + 2), line_y + fontHeight_ / 2, line1_, line_y + fontHeight_ / 2);
        }

        auto addr = QString("%1").arg(line_addr, 16, 16, QLatin1Char('0'));
        painter.setPen(addr_color);
        painter.drawText(line1_ + lineWidth_, line_y, addr.length() * fontWidth_, fontHeight_, Qt::AlignTop, addr);

        auto mnemonic = QString(insn[line + offset].mnemonic);
        if (mnemonic.startsWith("b"))
        {
            painter.setPen(jump_color);
        }
        else if (mnemonic == "push" || mnemonic == "pop")
        {
            painter.setPen(func_color);
        }
        else
        {
            painter.setPen(normal_color);
        }
        painter.drawText(line2_ + lineWidth_, line_y, mnemonic.length() * fontWidth_, fontHeight_, Qt::AlignTop,
                         mnemonic);
        painter.setPen(normal_color);

        auto op_str = QString(insn[line + offset].op_str);
        if (op_str.startsWith("{"))
        {
            painter.setPen(bigbrack_color);
            painter.drawText(line3_ + lineWidth_, line_y, op_str.length() * fontWidth_, fontHeight_, 0, op_str);
        }
        else
        {
            auto ops = op_str.split(",", Qt::SkipEmptyParts);
            auto op_pos_x = 0;
            for (auto &op : ops)
            {
                auto op_trim = op.trimmed();
                painter.setPen(regsName_.contains(op_trim) ? regs_color : normal_color);
                painter.drawText(line3_ + lineWidth_ * (1 + op_pos_x), line_y, op.length() * fontWidth_, fontHeight_, 0,
                                 op);
                op_pos_x += op.length();
                if (op != ops[ops.length() - 1])
                {
                    painter.setPen(Qt::red);
                    painter.drawText(line3_ + lineWidth_ * (1 + op_pos_x), line_y, fontWidth_, fontHeight_, 0, ",");
                    op_pos_x++;
                }
            }
        }
    }

    painter.setPen(QPen(Qt::red));
    painter.drawLine(line1_, 0, line1_, areaSize.height());
    painter.drawLine(line2_, 0, line2_, areaSize.height());
    painter.drawLine(line3_, 0, line3_, areaSize.height());
}
#include <QMessageBox>
void DisassView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
    {
        // 弹出一个菜单, 菜单项是 QAction 类型
        QMenu menu;
        QAction *act = menu.addAction("C++");
        connect(act, &QAction::triggered, this, [=]() { QMessageBox::information(this, "title", "您选择的是C++..."); });
        menu.addAction("Java");
        menu.addAction("Python");
        menu.exec(QCursor::pos()); // 右键菜单被模态显示出来了
    }

    if (!debuged)
    {
        return;
    }
    focusIndex_ = verticalScrollBar()->value() + event->pos().y() / fontHeight_;
    focusAddr = insn[focusIndex_].address;
    logd("focusAddr: 0x{:x}", focusAddr);
    viewport()->update();
}

void DisassView::keyPressEvent(QKeyEvent *event)
{
    auto key = event->key();
    if (key == Qt::Key_G)
    {
        QString color_str;
        auto text = QInputDialog::getText(this, "", "");
        uint64_t addr = text.toUInt(nullptr, 16);
        if (!addr)
        {
            qDebug() << "addr toUInt error!";
            return;
        }
        jumpTo(addr);
    }
    else if (key == Qt::Key_Down)
    {
        focusAddr = insn[focusIndex_ + 1].address;
        focusIndex_++;
        viewport()->update();
    }
    else if (key == Qt::Key_Up)
    {
        focusAddr = insn[focusIndex_ - 1].address;
        focusIndex_--;
        viewport()->update();
    }
}

void DisassView::resizeEvent(QResizeEvent *)
{
    auto maxvalue = count - viewport()->size().height() / fontHeight_;
    verticalScrollBar()->setMaximum(maxvalue);
}

void DisassView::jumpTo(uint64_t addr)
{
    emit msg_cpu_jump_sig(addr);
}

DumpView::DumpView(QWidget *parent) : QAbstractScrollArea()
{
    auto font = QFont("FiraCode", 8);
    auto metrics = QFontMetrics(font);
    fontWidth_ = metrics.horizontalAdvance('X');
    fontHeight_ = metrics.height();
    QAbstractScrollArea::setFont(font);

    line1_ = fontWidth_ * 15;
    line2_ = fontWidth_ * 25;
    verticalScrollBar()->setMaximum(maxLine_);
}

bool is_printable(uint8_t data)
{
    return (data >= 0x20) && (data <= 0x7E);
}

void DumpView::paintEvent(QPaintEvent *event)
{
    if (!debuged)
    {
        return;
    }

    QSize areaSize = viewport()->size();
    auto offset = verticalScrollBar()->value() * 16;
    auto max_line = areaSize.height() / fontHeight_ + 1;

    QPainter painter(viewport());
    auto addr_width = 18;

    for (auto line : boost::irange(0, max_line))
    {
        QString print_text;
        auto addr = QString("%1").arg(startAddr_ + offset + line * 16, 16, 16, QLatin1Char('0'));
        painter.drawText(0, line * fontHeight_, addr.length() * fontWidth_, fontHeight_, 0, addr);

        for (auto i : boost::irange(0, 16))
        {
            auto byte_data = (uint8_t)data[offset + line * 16 + i];
            print_text += (is_printable(byte_data) ? char(byte_data) : '.');
            auto draw_data = QString("%1").arg(byte_data, 2, 16, QLatin1Char('0'));
            painter.drawText((addr_width + i * 3) * fontWidth_, line * fontHeight_, draw_data.length() * fontWidth_,
                             fontHeight_, 0, draw_data);
        }
        painter.drawText((addr_width + 16 * 3) * fontWidth_, line * fontHeight_, print_text.length() * fontWidth_,
                         fontHeight_, 0, print_text);
    }
    painter.setPen(QPen(QColor(106, 255, 124)));
    auto line = fontWidth_ * (addr_width - 1);
    auto step = fontWidth_ * 4 * 3;
    for (auto i : boost::irange(0, 5))
    {
        auto step_line = line + i * step;
        if (i > 0)
        {
            step_line += fontWidth_ * 0.5;
        }
        painter.drawLine(step_line, 0, step_line, areaSize.height());
    }
}

void DumpView::mousePressEvent(QMouseEvent *event)
{
    int y = event->pos().y() / fontHeight_;
    int x = (event->pos().x() / fontWidth_ - 12) / 3;
    if (x >= 0 && x <= 15)
    {
        auto index = (verticalScrollBar()->value() + y) * 16 + x;
        auto value = *(uint64_t *)&data[index];
        auto clipboard = QApplication::clipboard();
        clipboard->setText(QString("0x%1").arg(value, 4, 16, QLatin1Char('0')));
    }
}
void DumpView::keyPressEvent(QKeyEvent *event)
{
    auto key = event->key();
    if (key == Qt::Key_G)
    {
        auto text = QInputDialog::getText(this, "", "");
        uint64_t addr = text.toUInt(nullptr, 16);
        if (!addr)
        {
            qDebug() << "addr toUInt error!";
            return;
        }
        emit msg_dump_jump_sig(addr);
    }
    else if (key == Qt::Key_B)
    {
        auto text = QInputDialog::getText(this, "add watch", "");
        uint64_t addr = text.toUInt(nullptr, 16);
        if (!addr)
        {
            qDebug() << "addr toUInt error!";
            return;
        }
        emit msg_dump_addWatch_sig(addr);
    }
}
void DumpView::setDebugFlag(bool flag)
{
    debuged = flag;
    verticalScrollBar()->setValue(0);
}

void DumpView::setStartAddr(uint64_t addr)
{
    startAddr_ = addr;
    logd("{:x}", addr);
}

RegsView::RegsView(QWidget *parent) : QAbstractScrollArea()
{
    auto font = QFont("FiraCode", 8);
    auto metrics = QFontMetrics(font);
    fontWidth_ = metrics.horizontalAdvance('X');
    fontHeight_ = metrics.height();
    QAbstractScrollArea::setFont(font);
}

void RegsView::setRegs(user_regs_struct reg)
{
    currentRegs_ = reg;
    regChanged.clear();
    flagChanged.clear();

    for (int i = 0; i < 32; i++)
    {
        if (currentRegs_.regs[i] != lastRegs_.regs[i])
        {
            regChanged.push_back(i);
        }
    }

    for (int i = 0; i < flagName_.size(); i++)
    {
        auto c_flag = (currentRegs_.pstate >> (63 - i)) & 1;
        auto l_flag = (lastRegs_.pstate >> (63 - i)) & 1;
        if (c_flag != l_flag)
        {
            flagChanged.push_back(i);
        }
    }
    lastRegs_ = currentRegs_;
}
void RegsView::setDebugFlag(bool flag)
{
    debuged = flag;
}

void RegsView::paintEvent(QPaintEvent *event)
{
    if (!debuged)
    {
        return;
    }
    QPainter painter(viewport());
    auto line = 0;
    auto reg_index = 0;
    for (auto reg_name : regsName_)
    {

        painter.drawText(0, line * fontHeight_, reg_name.length() * fontWidth_, fontHeight_, 0, reg_name);
        auto reg_value = QString("%1").arg(currentRegs_.regs[reg_index], 16, 16, QLatin1Char('0'));

        // draw reg_value
        painter.save();
        if (regChanged.contains(reg_index))
        {
            painter.setPen(Qt::red);
        }
        painter.drawText(fontWidth_ * 4, line * fontHeight_, reg_value.length() * fontWidth_, fontHeight_, 0,
                         reg_value);
        painter.restore();

        line++;
        reg_index++;
    }
    // 绘制PC
    line++;
    auto reg_value = QString("%1").arg(currentRegs_.pc, 16, 16, QLatin1Char('0'));
    painter.drawText(0, line * fontHeight_, 2 * fontWidth_, fontHeight_, 0, "PC");
    painter.drawText(fontWidth_ * 4, line * fontHeight_, reg_value.length() * fontWidth_, fontHeight_, 0, reg_value);

    line += 2;
    auto flag_index = 0;
    for (auto flag_name : flagName_)
    {
        painter.drawText(0, line * fontHeight_, flag_name.length() * fontWidth_, fontHeight_, 0, flag_name);

        painter.save();
        if (flagChanged.contains(flag_index))
        {
            painter.setPen(Qt::red);
        }
        auto flag_value = QString("%1").arg((currentRegs_.pstate >> (63 - flag_index)) & 1);
        painter.drawText(fontWidth_ * 4, line * fontHeight_, flag_value.length() * fontWidth_, fontHeight_, 0,
                         flag_value);
        painter.restore();

        flag_index++;
        line++;
    }
}

void RegsView::mousePressEvent(QMouseEvent *event)
{
    int y = event->pos().y() / fontHeight_;
    uint64_t *reg_p = (uint64_t *)&currentRegs_;
    if (y <= 17)
    {
        if (y > 12)
        {
            y -= 2;
        }
        auto value = *(reg_p + y);
        auto clipboard = QApplication::clipboard();
        clipboard->setText(QString("0x%1").arg(value, 4, 16, QLatin1Char('0')));
    }
}

StackView::StackView(QWidget *parent) : QAbstractScrollArea()
{
    auto font = QFont("FiraCode", 8);
    auto metrics = QFontMetrics(font);
    fontWidth_ = metrics.horizontalAdvance('X');
    fontHeight_ = metrics.height();
    QAbstractScrollArea::setFont(font);
    verticalScrollBar()->setMaximum(maxLine_);
}

void StackView::paintEvent(QPaintEvent *event)
{
    if (!debuged)
    {
        return;
    }

    QSize areaSize = viewport()->size();
    auto offset = verticalScrollBar()->value() * 4;
    auto max_line = areaSize.height() / fontHeight_ + 1;

    QPainter painter(viewport());
    for (auto line : boost::irange(0, max_line))
    {
        QString print_text;
        auto addr = QString("%1").arg(startAddr_ + offset + line * 4, 8, 16, QLatin1Char('0'));
        painter.drawText(0, line * fontHeight_, addr.length() * fontWidth_, fontHeight_, 0, addr);
        auto byte_data = *(uint64_t *)&data[offset + line * 4];
        auto value = QString("%1").arg(byte_data, 8, 16, QLatin1Char('0'));
        painter.drawText(fontWidth_ * 10, line * fontHeight_, value.length() * fontWidth_, fontHeight_, 0, value);
    }
    // painter.setPen(QPen(QColor(106, 255, 124)));
    // auto line = fontWidth_ * 11;
    // auto step = fontWidth_ * 4 * 3;
    // for (auto i : boost::irange(0, 5))
    // {
    //     auto step_line = line + i * step;
    //     if (i > 0)
    //     {
    //         step_line += fontWidth_ * 0.5;
    //     }
    //     painter.drawLine(step_line, 0, step_line, areaSize.height());
    // }
}

void StackView::setSpValue(uint64_t value)
{
    spValue_ = value;
}

void StackView::setStartAddr(uint64_t addr)
{
    startAddr_ = addr;
}

void StackView::setDebugFlag(bool flag)
{
    debuged = flag;
}
