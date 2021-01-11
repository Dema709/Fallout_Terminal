#ifndef QTEXTTABLEWIDGET_H
#define QTEXTTABLEWIDGET_H

#include <QTableWidget>

//Виджет для простого вывода текста в виде таблицы

class QTextTableWidget : public QTableWidget
{
public:
    QTextTableWidget(QWidget * parent = nullptr);
    virtual ~QTextTableWidget();

    void setText(const int row, const int column, const QString &text);
    void setAlignment(uint16_t alignment);//Для заполняемых ячеек (не для всех)
    void setFont(QFont font);//Для заполняемых ячеек (не для всех)
    void setFlag(Qt::ItemFlag flag);//Для новых ячеек; отключает этот флаг
private:
    uint16_t alignment;
    bool alignmentIsSet = false;
    QFont font;
    bool fontIsSet = false;
    Qt::ItemFlag flag = Qt::NoItemFlags;//Все выбраны (?)
};

#endif // QTEXTTABLEWIDGET_H
