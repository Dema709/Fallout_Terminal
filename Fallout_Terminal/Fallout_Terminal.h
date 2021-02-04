#ifndef FALLOUT_TERMINAL_H
#define FALLOUT_TERMINAL_H

#include <QWidget>
#include "../Common/QTextTableWidget.h"
#include <QSet>
#include <QList>//deque
#include <QTimer>

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

enum TapDirection{
    center,
    top,
    bottom,
    left,
    right
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
    std::pair<int,int> numV(int numInString);
    int numS(int row, int column);
    int isWord(int numInString);//returns wordNum or -1
    int isHint(int numInString);//returns hintNum or -1

    int sameCharsAsAnswer(QString s);
    void showWarning(bool warningEnabled);//Включено ли мигающее предупреждение

    void wordPressed(int index, bool callFromHint);
    void hintPressed(int index);
    void setAttemptsCount(int attempts);
    void addStringToRightRows(QString s);
    void changeSelectedStringInRightRows(QString s);
    void clearRightRows();

    TapDirection getTapDirection(int rowTapped, int columnTapped);

    int rowsCount = 17;
    int columnsCount = 2;
    int symbolsInRow = 12;
    int wordsCount = 18;
    int wordsSize = 7;
    int attemptsLeft, maxAttempts = 4;
    int rightRowSize = 16;//С учётом '>'; Количество символов в строке правого ряда
    int topRowsCount = 5;//Количество рядов в верхнем заголовке
    int symbolsInIndex = 0;//Длина строки вида 0x1234. Устанавливается при генерации
    QTextTableWidget * textTableWidget;
    QVector<Word> words;
    QVector<Hint> hints;
    QList<QString> rightRows;//Или же std::deque
    QTimer * warningTimer;
    bool warningShown;
    QTimer * flashingTimer;//Для выбранной ячейки (мигание)
    /*
    //https://fallout.fandom.com/ru/wiki/Взлом_терминала
    //Уровень защиты Терминала
    //Требуется навык Наука
    //Количество букв в пароле - недостоверная информация из вики!
    Очень простой       15 	4
    Простой             25 	6
    Средний             50 	8
    Сложный             75 	10
    Очень сложный       100 12 */
    int currentRow=0, currentColumn=0;
private slots:
    void changeWarningState();//Предупреждение мигает
    void flashSelectedSymbol();//Для выбранной ячейки (мигание)

    void cellClicked(int row, int column);
    void cellEntered(int row, int column);

    void cellPressed(int row, int column);
    void cellDoubleClicked(int row, int column);
    void itemSelectionChanged();
    void currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

};
#endif // FALLOUT_TERMINAL_H
