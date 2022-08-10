#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "CPUView.h"
#include "MAPView.h"
#include "log.h"
#include <QShortcut>
#include <QSplitter>
#include <QTcpSocket>
#include <cstdint>
#include <math.h>
#include <qdebug.h>
#include <qobjectdefs.h>
#include <qshortcut.h>
#include <string>

std::string TARGET_NAME = "com.example.myapplication";

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    resize(1000, 800);
    socketClient_ = new QTcpSocket;
    disassView = new DisassView;
    dumpView = new DumpView;
    regsView = new RegsView;
    mapView = new MapView;
    stackView = new StackView;

    auto splitter_middle = new QSplitter(Qt::Orientation::Vertical);
    auto splitter_top = new QSplitter(Qt::Orientation::Horizontal);
    auto splitter_bottom = new QSplitter(Qt::Orientation::Horizontal);

    ui->cpu_layout->addWidget(splitter_middle);
    splitter_middle->addWidget(splitter_top);
    splitter_middle->addWidget(splitter_bottom);
    splitter_middle->setStretchFactor(0, 8);
    splitter_middle->setStretchFactor(1, 2);

    splitter_top->addWidget(disassView);
    splitter_top->addWidget(regsView);
    splitter_top->setStretchFactor(0, 10);
    splitter_top->setStretchFactor(1, 4);

    splitter_bottom->addWidget(dumpView);
    splitter_bottom->addWidget(stackView);
    splitter_bottom->setStretchFactor(0, 10);
    splitter_bottom->setStretchFactor(1, 4);

    ui->map_layout->addWidget(mapView);

    ui->tabWidget->setCurrentIndex(0);

    connect(disassView, SIGNAL(msg_cpu_jump_sig(uint64_t)), this, SLOT(msg_cpu_jump_slot(uint64_t)));
    connect(dumpView, SIGNAL(msg_dump_jump_sig(uint64_t)), this, SLOT(msg_dump_jump_slot(uint64_t)));
    connect(dumpView, SIGNAL(msg_dump_addWatch_sig(uint64_t)), this, SLOT(msg_dump_addWatch_slot(uint64_t)));

    QShortcut *step_button = new QShortcut(this);
    step_button->setKey(tr("f5"));
    step_button->setAutoRepeat(false);

    QShortcut *bp_button = new QShortcut(this);
    bp_button->setKey(tr("f2"));
    bp_button->setAutoRepeat(false);

    // QShortcut *del_bp_button = new QShortcut(this);
    // del_bp_button->setKey(tr("f7"));
    // del_bp_button->setAutoRepeat(false);

    connect(step_button, SIGNAL(activated()), this, SLOT(msg_step_slot()));
    connect(bp_button, SIGNAL(activated()), this, SLOT(msg_bp_slot()));
    // connect(del_bp_button, SIGNAL(activated()), this, SLOT(msg_del_bp_slot()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionopen_triggered()
{
    socketClient_->connectToHost("127.0.0.1", 3322);
    if (!socketClient_->waitForConnected(30000))
    {
        qDebug() << "errr";
        return;
    }
    //  set debug target process name
    socketClient_->write(TARGET_NAME.c_str());

    connect(socketClient_, SIGNAL(readyRead()), this, SLOT(socketHandle()));
    connect(socketClient_, SIGNAL(disconnected()), this, SLOT(socketClose()));
}

void MainWindow::on_actioncontinue_triggered()
{
    msgBuff_[0] = MSG_CONTINUE;
    socketClient_->write(msgBuff_, 1);
    disassView->setProcessPaused(false);
}

void MainWindow::on_actionstop_triggered()
{
    msgBuff_[0] = MSG_STOP;
    socketClient_->write(msgBuff_, 1);
}

void MainWindow::on_actionmaps_triggered()
{
    socketClient_->write("cpu_all");
}

void MainWindow::on_actionclose_triggered()
{
    socketClient_->close();
}

void MainWindow::msg_step_slot()
{
    msgBuff_[0] = MSG_STEP;
    socketClient_->write(msgBuff_, 1);
}

void MainWindow::msg_cpu_jump_slot(uint64_t addr)
{
    msgBuff_[0] = MSG_CPU;
    *(uint64_t *)(msgBuff_ + 1) = addr;
    socketClient_->write(msgBuff_, 9);
}

void MainWindow::msg_dump_jump_slot(uint64_t addr)
{
    msgBuff_[0] = MSG_DUMP;
    *(uint64_t *)(msgBuff_ + 1) = addr;
    socketClient_->write(msgBuff_, 5);
}

void MainWindow::msg_dump_addWatch_slot(uint64_t addr)
{
    msgBuff_[0] = MSG_ADD_WATCH;
    *(uint64_t *)(msgBuff_ + 1) = addr;
    socketClient_->write(msgBuff_, 5);
}

void MainWindow::msg_bp_slot()
{
    uint64_t bp_addr = disassView->focusAddr & (~1);
    if (!disassView->bpList_.contains(bp_addr))
    {
        disassView->bpList_.append(bp_addr);
        msgBuff_[0] = MSG_ADD_BP;
        logd("add bp addr: {:x}", bp_addr);
    }
    else
    {
        disassView->bpList_.removeAll(bp_addr);
        msgBuff_[0] = MSG_DEL_BP;
        logd("del bp addr: {:x}", bp_addr);
    }
    *(uint64_t *)(msgBuff_ + 1) = bp_addr;
    socketClient_->write(msgBuff_, 5);
    disassView->viewport()->update();
}

void MainWindow::on_actionregs_triggered()
{
}

void MainWindow::on_tabWidget_tabBarClicked(int index)
{
    switch (index)
    {
    case 1:
        msgBuff_[0] = MSG_MAPS;
        socketClient_->write(msgBuff_, 1);
        break;
    default:
        break;
    }
}

void MainWindow::socketHandle()
{
    uint8_t msg;
    QByteArray data;
    while ((msg = getMsg()))
    {
        switch (msg)
        {
        case MSG_REGS:
            data = getDataN(sizeof(user_regs_struct));
            memcpy(&mRegs, data.data(), sizeof(user_regs_struct));
            regsView->setRegs(mRegs);
            regsView->setDebugFlag(true);
            regsView->viewport()->update();

            disassView->setCurrentPc(mRegs.pc);
            disassView->setCurrentCPSR(mRegs.pstate);
            disassView->setProcessPaused(true);
            disassView->viewport()->update();
            break;
        case MSG_CPU:
            disassView->setStartAddr(getData8());
            data = getDataN(0x400);
            memcpy(disassView->data, data.data(), 0x400);
            disassView->setDebugFlag(true);
            disassView->disassInstr();
            disassView->viewport()->update();
            break;
        case MSG_MAPS: {
            logd("map msg!");
            data = getDataN(getData8());
            auto map_str = QString(data);
            mapView->setMap(map_str);
            break;
        }
        case MSG_DUMP:
            dumpView->setStartAddr(getData8());
            data = getDataN(0x400);
            memcpy(dumpView->data, data.data(), 0x400);
            dumpView->setDebugFlag(true);
            dumpView->viewport()->update();
            break;
        default:
            qDebug() << "no handle msg : " << msg;
            return;
        }
    }
}

void MainWindow::socketClose()
{
    // clear all cpuview
    disassView->setDebugFlag(false);
    disassView->viewport()->update();
    disassView->bpList_.clear();
    dumpView->setDebugFlag(false);
    dumpView->viewport()->update();
    regsView->setDebugFlag(false);
    regsView->viewport()->update();
    stackView->setDebugFlag(false);
    stackView->viewport()->update();
}

uint8_t MainWindow::getMsg()
{
    auto data = socketClient_->read(1);
    if (data.length() == 0)
    {
        return 0;
    }
    else
    {
        return data[0];
    }
}

uint64_t MainWindow::getData8()
{
    auto data = socketClient_->read(8);
    return *(uint64_t *)data.data();
}

QByteArray MainWindow::getDataN(int n)
{

    auto recive_len = 0;
    QByteArray data;
    while (true)
    {
        data.append(socketClient_->read(n - data.length()));
        if (data.length() == n)
        {
            return data;
        }
        socketClient_->waitForReadyRead();
    }
}

void MainWindow::on_lineEdit_textChanged(const QString &arg1)
{
    QString qwe = arg1;
    mapView->filterMap(qwe);
}
