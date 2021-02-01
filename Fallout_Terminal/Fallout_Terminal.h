#ifndef FALLOUT_TERMINAL_H
#define FALLOUT_TERMINAL_H

#include <QWidget>
#include "../Common/QTextTableWidget.h"
#include <QSet>
#include <QList>//deque

struct Word {
    QString word;
    int start_index;
    bool is_pressed, is_answer;
    //bool is_pressed = false;
    //bool is_answer = false;
};

struct Hint {
    int start_index, end_index;
    bool is_pressed;
};

struct SelectedCells {
    int start_index, end_index;
    bool is_selected;
};

class Fallout_Terminal : public QWidget
{
    Q_OBJECT

public:
    Fallout_Terminal(QWidget *parent = nullptr);
    ~Fallout_Terminal();
private:
    QVector<QString> getWordsFromDictionary();
    QVector<int> generateWordsStartIndexes();
    void findHints(QString & hackingSymbols);
    QString generateHackingSymbols();
    QVector<QVector<QString>> generateHackingIndexes();
    void tuneTextTableWidget(); //Первоначальная настройка таблицы
    void newGame();             //Заполнение таблицы, подсказок (и др.) и сброс попыток
    void tuneAnotherTableWidget(bool isTopWidget);//top, right widget
    std::pair<int,int> numV(int numInString);
    int numS(int row, int column);
    int isWord(int numInString);//returns wordNum or -1
    int isHint(int numInString);//returns hintNum or -1

    int sameCharsAsAnswer(QString s);

    void wordPressed(int index, bool callFromHint);
    void hintPressed(int index);
    void setAttemptsCount(int attempts);
    void addStringToRightRows(QString s);
    void changeSelectedStringInRightRows(QString s);
    void clearRightRows();

    int rowsCount = 17;
    int columnsCount = 2;
    int symbolsInRow = 12;
    int wordsCount = 4*4;
    int wordsSize = 6;
    int attemptsLeft, maxAttempts = 4;
    int rightRowSize = 16;//С учётом '>'; Количество символов в строке правого ряда
    int topRowsCount = 5;//Количество рядов в верхнем заголовке
    int symbolsInIndex = 0;//Длина строки вида 0x1234. Устанавливается при генерации
    QTextTableWidget * textTableWidget;
    QTextTableWidget * topTextTableWidget;
    QTextTableWidget * rightTextTableWidget;
    QVector<Word> words;
    QVector<Hint> hints;
    //SelectedCells selectedCells;
    QList<QString> rightRows;//Или же std::deque
    /*
    //https://fallout.fandom.com/ru/wiki/Взлом_терминала
    //Уровень защиты Терминала
    //Требуется навык Наука
    //Количество букв в пароле
    Очень простой       15 	4
    Простой             25 	6
    Средний             50 	8
    Сложный             75 	10
    Очень сложный       100 12 */
private slots:
    //void tempTest();
    //void cellActivated(int row, int column);
    void cellClicked(int row, int column);
    //void cellDoubleClicked(int row, int column);//?
    void cellEntered(int row, int column);
    void cellPressed(int row, int column);
    //setSelected on QTableWidgetItem
    void itemSelectionChanged();
    void currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
};
#endif // FALLOUT_TERMINAL_H
