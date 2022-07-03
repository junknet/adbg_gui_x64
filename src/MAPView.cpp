#include "MAPView.h"
#include "log.h"
#include <QClipboard>
#include <qapplication.h>
#include <qnamespace.h>

MapView::MapView(QWidget *parent) : QListWidget()
{
    auto font = QFont("FiraCode", 8);
    setFont(font);
    connect(this, SIGNAL(itemClicked(QListWidgetItem *)), this, SLOT(onListMailItemClicked(QListWidgetItem *)));
}

void MapView::setMap(QString &map)
{
    mapStr_ = map;
    mapList_.clear();
    clear();
    auto lines = mapStr_.split("\n", Qt::SkipEmptyParts);
    for (auto line : lines)
    {
        QString data;
        auto infos = line.split(" ", Qt::SkipEmptyParts);

        if (infos.length() == 6)
        {
            auto name = infos[5];
            if (name.endsWith("so") || name.endsWith(".dex") || name.endsWith(".apk"))
            {
                data += infos[0];
                data += "\t" + infos[1];
                data += "\t" + infos[infos.length() - 1];
                mapList_.push_back(data);
                addItem(data);
            }
        }
    }
}

void MapView::filterMap(QString &str)
{
    clear();
    for (auto item : mapList_)
    {
        if (item.contains(str))
        {
            addItem(item);
        }
    }
}

uint32_t MapView::findSoBase(QString so_name)
{
    for (auto &item : mapList_)
    {
        if (item.endsWith(so_name))
        {
            auto addr_str = item.split("-", Qt::SkipEmptyParts)[0];
            return addr_str.toUInt(nullptr, 16);
        }
    }
    logd("{} find base addr erro!", so_name.toStdString());
    return 0;
}

void MapView::onListMailItemClicked(QListWidgetItem *item)
{
    auto clipboard = QApplication::clipboard();
    auto addr = item->text().split("-", Qt::SkipEmptyParts)[0];
    clipboard->setText("0x" + addr);
}
