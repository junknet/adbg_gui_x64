#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "CPUView.h"
#include "MAPView.h"
#include <QListView>
#include <QMainWindow>
#include <QTcpSocket>
#include <cstdint>
#include <qglobal.h>
#include <qlistview.h>
#include <qtcpsocket.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

  private slots:
    void on_tabWidget_tabBarClicked(int index);

  private slots:
    void on_actionopen_triggered();

    void on_actioncontinue_triggered();

    void on_actionstop_triggered();

    void on_actionmaps_triggered();

    void on_actionregs_triggered();

    void on_actionclose_triggered();

    void socketHandle();

    void socketClose();

    void msg_cpu_jump_slot(uint64_t addr);
    void msg_dump_jump_slot(uint64_t addr);
    void msg_dump_addWatch_slot(uint64_t addr);

    void msg_step_slot();
    void msg_bp_slot();

    void on_lineEdit_textChanged(const QString &arg1);

  private:
    uint8_t getMsg();
    uint64_t getData8();
    QByteArray getDataN(int n);

  private:
    Ui::MainWindow *ui;
    QTcpSocket *socketClient_;
    user_regs_struct mRegs;
    DisassView *disassView;
    DumpView *dumpView;
    RegsView *regsView;
    MapView *mapView;
    StackView *stackView;
    char msgBuff_[5];
};
#endif // MAINWINDOW_H
