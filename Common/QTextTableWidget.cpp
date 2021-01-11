#include "QTextTableWidget.h"

#include <QDebug>

QTextTableWidget::QTextTableWidget(QWidget * parent) : QTableWidget(parent){
}
QTextTableWidget::~QTextTableWidget(){
    //qDebug()<<"~QTextTableWidget";
}

void QTextTableWidget::setText(const int row, const int column, const QString &text){
    //В стандартном QTableWidget почему-то нет такой проверки
    if (row<0 || column<0){
        qWarning().nospace()<<"Warning in QTextTableWidget::setText("<<row<<", "<<column<<") "<<
                              (row<0 ? " row<0" : "")<<
                              (column<0 ? " column<0" : "");
        return;
    }

    //Увеличение размера таблицы при выходе за границы
    if (row >= this->rowCount())
        this->setRowCount(row + 1);
    if (column >= this->columnCount())
        this->setColumnCount(column + 1);

    QTableWidgetItem * cell = this->item(row, column);
    if (cell == nullptr){
        //Если QTableWidgetItem не задан для ячейки, создаём его
        cell = new QTableWidgetItem();
        this->setItem(row, column, cell);

        cell->setFlags(cell->flags() ^ flag);
    }
    cell->setText(text);

    if (alignmentIsSet)
        cell->setTextAlignment(alignment);
    if (fontIsSet)
        cell->setFont(font);
}

void QTextTableWidget::setAlignment(uint16_t alignment){
    this->alignment = alignment;
    alignmentIsSet = true;
}

void QTextTableWidget::setFont(QFont font){
    this->font = font;
    fontIsSet = true;
}

void QTextTableWidget::setFlag(Qt::ItemFlag flag){
    //int old_flag = this->flag;    int add_flag = flag;
    //this->flag = static_cast<Qt::ItemFlag>(old_flag ^ add_flag);
    this->flag = flag;
}
