#pragma once
#include <capstone.h>
#include <qcolor.h>
#include <qglobal.h>
#include <qlist.h>

#include <QAbstractScrollArea>
#include <QColor>
#include <cstdint>

#define MSG_STOP 1
#define MSG_CONTINUE 2
#define MSG_MAPS 3
#define MSG_REGS 4
#define MSG_STACK 5
#define MSG_DUMP 6
#define MSG_CPU 7
#define MSG_STEP 8
#define MSG_ADD_BP 9
#define MSG_DEL_BP 10
#define MSG_ADD_WATCH 11

struct user_regs_struct
{
    unsigned long long regs[31];
    unsigned long long sp;
    unsigned long long pc;
    unsigned long long pstate;
};
class DisassView : public QAbstractScrollArea
{
    Q_OBJECT
  public:
    explicit DisassView(QWidget *parent = nullptr);
    ~DisassView() override = default;

    // protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *) override;

    void jumpTo(uint64_t addr);

    void disassInstr();
    bool isThumbMode();
    void setCurrentPc(uint64_t value);
    void setProcessPaused(bool status);
    void setCurrentCPSR(uint64_t flag);
    void setStartAddr(uint64_t addr);
    void setDebugFlag(bool flag);

    uint8_t data[0x400];
    uint64_t jump_addr_ = 0;
    uint64_t focusAddr;
    uint64_t focusIndex_;
    QList<uint64_t> bpList_;

  signals:
    void msg_cpu_jump_sig(uint64_t addr);
    // void msg_add_bp_sig(uint64_t addr);
    // void msg_del_bp_sig(uint64_t addr);

  private:
    int fontWidth_ = 0;
    int fontHeight_ = 0;

    int line1_ = 0;
    int line2_ = 0;
    int line3_ = 0;

    int line1_space = 10;
    int line2_space = 18;
    int line3_space = 10;

    int lineWidth_ = 0;
    bool debuged = false;
    bool screenScrolled = false;

    bool paused_ = false;
    csh handle_arm64;
    cs_insn *insn = nullptr;
    int count = 0;
    bool selected_ = false;
    int selectLine_ = 0;

    uint64_t pcValue_;
    uint64_t CPSR_;
    uint64_t startAddr_;
    uint64_t screenStartAddr;
    uint64_t screenEndAddr;
    uint64_t disStartAddr;
    uint64_t disEndAddr;

    QColor normal_color = QColor("#fcfcfc");
    QColor func_color = QColor("#f5f50e");
    QColor jump_color = QColor("#11f50d");
    QColor regs_color = QColor("#73adad");
    QColor focus_color = QColor("#414141");
    QColor addr_color = QColor("#a77cec");
    QColor pc_bgcolor = QColor("#bfb05100");
    QColor bp_bgcolor = QColor("#bfff0507");
    QColor bigbrack_color = QColor("#38d9f9");

    QList<QString> regsName_ = {"r0", "r1", "r2",  "r3",  "r4", "r5", "r6", "r7",
                                "r8", "r9", "r10", "r11", "ip", "sp", "lr", "pc"};
};

class DumpView : public QAbstractScrollArea
{
    Q_OBJECT

  public:
    explicit DumpView(QWidget *parent = nullptr);
    ~DumpView() override = default;

    // protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void setDebugFlag(bool flag);
    void setStartAddr(uint64_t addr);

    uint8_t data[0x400];

  signals:
    void msg_dump_jump_sig(uint64_t addr);
    void msg_dump_addWatch_sig(uint64_t addr);

  private:
    int fontWidth_ = 0;
    int fontHeight_ = 0;

    int line1_ = 0;
    int line2_ = 0;
    int line3_ = 0;
    int lineWidth_ = 2;
    int maxLine_ = 0x400 / 16;
    uint64_t startAddr_;
    bool debuged = false;
};

class RegsView : public QAbstractScrollArea
{
    Q_OBJECT

  public:
    explicit RegsView(QWidget *parent = nullptr);
    ~RegsView() override = default;

    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

    void setRegs(user_regs_struct reg);
    void setDebugFlag(bool flag);

  private:
    bool debuged = false;
    int fontWidth_ = 0;
    int fontHeight_ = 0;
    user_regs_struct lastRegs_;
    user_regs_struct currentRegs_;
    QList<int> regChanged;
    QList<int> flagChanged;
    QList<QString> regsName_ = {
        "X0",  "X1",  "X2",  "X3",  "X4",  "X5",  "X6",  "X7",  "X8",  "X9",  "X10", "X11", "X12", "X13", "X14", "X15",
        "X16", "X17", "X18", "X19", "X20", "X21", "X22", "X23", "X24", "X25", "X26", "X27", "X28", "X29", "X30", "X31",
    };
    QList<QString> flagName_ = {"N", "Z", "C", "V", "Q"};
};

class StackView : public QAbstractScrollArea
{
    Q_OBJECT

  public:
    explicit StackView(QWidget *parent = nullptr);
    ~StackView() override = default;

    void paintEvent(QPaintEvent *event) override;
    void setSpValue(uint64_t value);
    void setStartAddr(uint64_t addr);
    void setDebugFlag(bool flag);

    uint8_t data[0x3000];

  private:
    int fontWidth_ = 0;
    int fontHeight_ = 0;
    bool debuged = false;
    int maxLine_ = 0x3000 / 4;
    uint64_t startAddr_;
    uint64_t spValue_;
};