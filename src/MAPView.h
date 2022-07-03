#pragma once

#include <QList>
#include <QListWidget>
#include <cstdint>
#include <qtmetamacros.h>

class MapView : public QListWidget
{
    Q_OBJECT
  public:
    explicit MapView(QWidget *parent = nullptr);
    ~MapView() override = default;

    void setMap(QString &map);
    void filterMap(QString &str);

    uint32_t findSoBase(QString so_name);
  public slots:
    void onListMailItemClicked(QListWidgetItem *item);

  private:
    QString mapStr_;
    QList<QString> mapList_;
};