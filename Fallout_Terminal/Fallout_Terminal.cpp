#include "Fallout_Terminal.h"
#include "../Common/random.hpp"

#include <QGridLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QFile>
#include <QMessageBox>
#include <algorithm>

#include <QDebug>

Fallout_Terminal::Fallout_Terminal(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("Терминал Fallout");//Название окна

    QGridLayout * mainLayout = new QGridLayout(this);
    textTableWidget = new QTextTableWidget();
    tuneTextTableWidget();
    mainLayout->addWidget(textTableWidget, 1, 0);

    connect(textTableWidget, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
    connect(textTableWidget, SIGNAL(currentCellChanged(int, int, int, int)), this, SLOT(currentCellChanged(int, int, int, int)));
    connect(textTableWidget, SIGNAL(cellClicked(int, int)), this, SLOT(cellClicked(int, int)));
    connect(textTableWidget, SIGNAL(cellEntered(int, int)), this, SLOT(cellEntered(int, int)));
    connect(textTableWidget, SIGNAL(cellPressed(int, int)), this, SLOT(cellPressed(int, int)));

    topTextTableWidget = new QTextTableWidget();
    mainLayout->addWidget(topTextTableWidget, 0, 0, 1, 2);

    rightTextTableWidget = new QTextTableWidget();
    mainLayout->addWidget(rightTextTableWidget, 1, 1);

    //Заглушка для верхней и правой полос
    tuneAnotherTableWidget(false);//right
    tuneAnotherTableWidget(true);//top

}

Fallout_Terminal::~Fallout_Terminal(){
}

QVector<QString> Fallout_Terminal::getWordsFromDictionary(){
    //temp
    //Переделать под базу данных с запросами вроде
    //select * from table where id>5 order by rand() limit 5;

    QFile file("litw-win.txt");
    if (!file.open(QIODevice::ReadOnly)){
        QString errorMessage = "Fallout_Terminal::getWordsFromDictionary() cannot open file";
        QMessageBox::critical(this, "Error", errorMessage);
        throw std::runtime_error(errorMessage.toStdString());
    }

    QTextStream fin(&file);
    fin.setCodec("UTF-8");//Для корректного отображения кириллицы
    QVector<QString> wordsVector;
    QString newWord;
    int freq;//temp; dictionary style
    int curWordsCount = 0;
    while (!fin.atEnd()){
        fin>>freq>>newWord;
        if (newWord.size()==wordsSize){
            wordsVector.push_back(newWord);
            qDebug()<<newWord;
            curWordsCount++;
            if (curWordsCount==wordsCount)
                break;
        }
    }
    return wordsVector;
}

QVector<int> Fallout_Terminal::generateWordsStartIndexes(){
    if (wordsCount*wordsSize + (wordsCount-1) > columnsCount*rowsCount*symbolsInRow)
        return {};//Нет возможности впихнуть столько слов этой длины

    //Вариант реализации алгоритма:
    //1. Создать вектор из слов, в которые входят случайные символы
    //и слова с добавленным в конец 1 символом. Таким образом
    //решится проблема следования слов друг за другом впритык
    //2. Случайно перемешать вектор
    //3. Удалить последний символ
    //{start_index, end_index} -> start_index, т.к. длина слов фиксирована

    //Алгоритм генерации только индексов:
    //Создаём вектор с длинами
    QVector<int> sizesVector;
    for (int i=0; i<wordsCount; i++){
        sizesVector.push_back(wordsSize+1);
    }
    int sizeLeft;//Оставшееся свободное место (с учётом последующего удаления последнего символа)
    sizeLeft = columnsCount*rowsCount*symbolsInRow - wordsCount*(wordsSize+1) + 1;
    for (int i=0; i<sizeLeft; i++){
        sizesVector.push_back(1);
    }

    //Перемешиваем вектор
    uint seed = effolkronium::random_static::get<uint>();
    srand(seed);//Инициализация генератора случайных чисел
    //Или же srand(time(NULL));
    std::random_shuffle(sizesVector.begin(), sizesVector.end());
    //Формируем массив начальных индексов у слов
    QVector<int> startIndexes;
    int indexSum = 0;
    for (auto i : sizesVector){
        if (i == wordsSize + 1){
            startIndexes.push_back(indexSum);
        }
        indexSum += i;
    }
    return startIndexes;
}

void Fallout_Terminal::findHints(QString & hackingSymbols){
    //Special symbols for hints: () {} [] <>
    QString hintChars = "(){}[]<>";
    QVector<int> lastIndexes(hintChars.size()/2, -1);//Последние индексы закрывающих скобок
    int curIndex = hackingSymbols.size()-1;

    while (curIndex >= 0){
        int wordIndex = isWord(curIndex);
        if (wordIndex != -1){
            lastIndexes.fill(-1);
            curIndex = words[wordIndex].start_index - 1;//Пропускаем слово
        } else {
            for (int i=0; i<hintChars.size()/2; i++){
                if (hackingSymbols[curIndex] == hintChars[i*2]){
                    //Открывающая скобка
                    if (lastIndexes[i] != -1){
                        hints.push_back({curIndex, lastIndexes[i], false});
                    }
                    break;
                } else if (hackingSymbols[curIndex] == hintChars[i*2+1]) {
                    //Закрывающая скобка
                    lastIndexes[i] = curIndex;
                    break;
                }
            }
            curIndex--;
        }

    }
}

QString Fallout_Terminal::generateHackingSymbols(){
    QVector<int> startIndexes = generateWordsStartIndexes();
    QVector<QString> wordsVector = getWordsFromDictionary();

    if (startIndexes.empty() || wordsVector.empty()){
        QString errorMessage = "Fallout_Terminal::";
        if (startIndexes.empty()) errorMessage += "generateWordsStartIndexes() ";
        if (wordsVector.empty()) errorMessage += "getWordsFromDictionary() ";
        errorMessage += "returned empty vector";
        QMessageBox::critical(this, "Error", errorMessage);
        throw std::runtime_error(errorMessage.toStdString());
    }

    words.clear();
    for (int i=0; i<startIndexes.size(); i++){
        words.push_back({wordsVector[i], startIndexes[i], false, false});
    }
    int answerNumber = effolkronium::random_static::get<int>(0, wordsCount-1);
    words[answerNumber].is_answer = true;

    //Special symbols for hints: () {} [] <>
    QVector<QChar> symbols = {'!',
                              '?',
                              '@',
                              '"',
                             '\'',
                              '#',
                              '$',
                              ';',
                              ':',
                              ',',
                              '.',
                              '%',
                              '^',
                              '*',
                              '-',
                              '_',
                              '|',
                              '/',
                             '\\',
                              '(',
                              ')',
                              '{',
                              '}',
                              '[',
                              ']',
                              '<',
                              '>'
                             };

    QString hackingSymbols;

    for (int i=0; i<rowsCount*columnsCount*symbolsInRow; i++){
        hackingSymbols += symbols[effolkronium::random_static::get<int>(0, symbols.size()-1)];
    }
    qDebug()<<hackingSymbols;//Without words

    for (int i=0; i<startIndexes.size(); i++){
        for (int j=0; j<wordsSize; j++){
            hackingSymbols[startIndexes[i]+j] = wordsVector[i][j];
        }
    }
    qDebug()<<hackingSymbols;//With words

    //Заполнение вектора с подсказками
    hints.clear();
    findHints(hackingSymbols);
    /*//Вывод подсказок
    for (auto & h : hints){
        QString hintText;
        for (int i=h.start_index; i<=h.end_index; i++){
            hintText.push_back(hackingSymbols[i]);
        }
        qDebug()<<hintText;
    }*/

    return hackingSymbols;
}

QVector<QVector<QString>> Fallout_Terminal::generateHackingIndexes(){
    //Возможные ограничения:
    //initial index F***
    //Последний символ - чётный

    QVector<QVector<QString>> answer; answer.reserve(columnsCount);
    uint16_t start_index = effolkronium::random_static::get<uint16_t>(0, (UINT16_MAX - rowsCount*columnsCount*symbolsInRow)/2);
    start_index *= 2;//Последний символ - чётный

    for (int col=0; col<columnsCount; col++){
        QVector<QString> total_column; total_column.reserve(rowsCount);
        for (int row=0; row<rowsCount; row++){
            QString indString = "0x"+QString("%1").arg(start_index, 4, 16, QChar('0')).toUpper();
            indString += ' ';
            if (col!=0) indString = ' ' + indString;
            total_column.push_back(indString);
            start_index += symbolsInRow;
        }
        answer.push_back(std::move(total_column));
    }
    //qDebug()<<"0x"+QString::number(start_index, 16).toUpper();//to smth like "0x0CB8"
    //qDebug()<<QString("0x%1").arg(start_index, 4, 16, QChar('0'));//to smth like "0x0cb8"

    return answer;
}

void Fallout_Terminal::tuneTextTableWidget(){
    QFont font("Consolas", 20);
    //Agenda Medium Ultra Condensed
    //QFont font("Consolas", 10, QFont::Monospace);
    font.setBold(true);
    textTableWidget->setFont(font);
    //Qt::ItemFlag flag = (Qt::ItemFlag)((uint32_t)Qt::ItemIsEditable | (uint32_t)Qt::ItemIsEnabled);
    textTableWidget->setFlag(Qt::ItemIsEditable);
    textTableWidget->setAlignment(Qt::AlignCenter);

    auto ans = generateHackingIndexes();

    for (int i=0; i<ans.size(); i++){
        for (int j=0; j<ans[0].size(); j++){
            textTableWidget->setText(j,i*(1+symbolsInRow),ans[i][j]);
        }
    }

    auto ans2 = generateHackingSymbols();

    for (int i=0; i<columnsCount; i++){
        for (int j=0; j<rowsCount; j++){
            for (int k=0; k<symbolsInRow; k++){
                textTableWidget->setText(j,i*(1+symbolsInRow)+k+1,
                                         QString(ans2[i*symbolsInRow*rowsCount +
                                                      j*symbolsInRow +
                                                      k]));
            }
        }
    }

    this->setAttemptsCount(maxAttempts);

    textTableWidget->horizontalHeader()->hide();
    textTableWidget->verticalHeader()->hide();
    textTableWidget->setShowGrid(false);

    textTableWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    textTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textTableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textTableWidget->resizeColumnsToContents();//textTableWidget->resizeRowsToContents();
    int delta = 2;//Запас (в пискелях)
    textTableWidget->setFixedSize(//textTableWidget->verticalHeader()->width() +
                                  textTableWidget->horizontalHeader()->length() + delta,
                                  //textTableWidget->horizontalHeader()->height() +
                                  textTableWidget->verticalHeader()->length() + delta);

    textTableWidget->setMouseTracking(true);//[signal] void QTableWidget::cellEntered(int row, int column)
}

void Fallout_Terminal::tuneAnotherTableWidget(bool isTopWidget){
    QTextTableWidget * textTableWidget = (isTopWidget ? topTextTableWidget : rightTextTableWidget);//temp (некрасивый ход)

    QFont font("Consolas", 20);
    font.setBold(true);
    textTableWidget->setFont(font);
    textTableWidget->setFlag(Qt::ItemIsEditable);
    textTableWidget->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);//(Qt::AlignCenter);

    if (isTopWidget){
        textTableWidget->setText(0,0, "ROBCO INDUSTRIES (TM) TERMLINK PROTOCOL");
        textTableWidget->setText(1,0, "!!! ПРЕДУПРЕЖДЕНИЕ: ТЕРМИНАЛ МОЖЕТ БЫТЬ ЗАБЛОКИРОВАН !!!");
        textTableWidget->setText(2,0, "");
        textTableWidget->setText(3,0, "# ПОПЫТОК ОСТАЛОСЬ: ▮▮▮▮");
        //textTableWidget->setText(4,0, "");
    } else {
        textTableWidget->setText(rowsCount-1, 0, "> fuck =_=");
    }

    textTableWidget->horizontalHeader()->hide();
    textTableWidget->verticalHeader()->hide();
    textTableWidget->setShowGrid(false);

    textTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textTableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textTableWidget->resizeColumnsToContents();
    textTableWidget->resizeRowsToContents();

    if (isTopWidget){
        textTableWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        textTableWidget->setFixedSize(//textTableWidget->verticalHeader()->width() +
                                      textTableWidget->horizontalHeader()->length(),
                                      //textTableWidget->horizontalHeader()->height() +
                                      textTableWidget->verticalHeader()->length());
    }
    //textTableWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
/*
    textTableWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    int delta = 2;//Запас (в пискелях)
    textTableWidget->setFixedSize(//textTableWidget->verticalHeader()->width() +
                                  textTableWidget->horizontalHeader()->length() + delta,
                                  //textTableWidget->horizontalHeader()->height() +
                                  textTableWidget->verticalHeader()->length() + delta);

    textTableWidget->setMouseTracking(true);//[signal] void QTableWidget::cellEntered(int row, int column)*/
}

std::pair<int,int> Fallout_Terminal::numV(int numInString){
    int section = numInString / (rowsCount * symbolsInRow);
    int numInSection = numInString % (rowsCount * symbolsInRow);
    int row = numInSection / symbolsInRow;
    int column = section * (symbolsInRow+1) + numInSection % symbolsInRow + 1;
    return {row, column};
}

int Fallout_Terminal::numS(int row, int column){
    int answer = 0;
    int section = column/(symbolsInRow+1);
    answer += section*symbolsInRow*rowsCount;
    answer += row*symbolsInRow;
    answer += column - section*(symbolsInRow+1)-1;
    return answer;
}

int Fallout_Terminal::isWord(int numInString){
    for (int i=0; i<words.size(); i++){
        if (numInString-words[i].start_index>=0 && numInString-words[i].start_index<wordsSize){
            if (!words[i].is_pressed){
                return i;
            } else {
                return -1;
            }
        }
    }
    return -1;
}

int Fallout_Terminal::isHint(int numInString){
    for (int i=0; i<hints.size(); i++){
        if (hints[i].start_index == numInString){
            if (!hints[i].is_pressed){
                return i;
            } else {
                return -1;
            }
        }
    }
    return -1;
}

void Fallout_Terminal::wordPressed(int index, bool callFromHint = false){
    //При неправильно выбранном слове отнимается 1 попытка
    //Если слово убирается не от нажатия подсказки
    words[index].is_pressed = true;

    if (words[index].is_answer){
        qDebug()<<"Answer found";

        QMessageBox::information(this, "Hacked ;)", "Answer found");
        tuneTextTableWidget();

        return;
    }

    for (int i=0; i<wordsSize; i++){
        auto pair = numV(words[index].start_index+i);
        textTableWidget->item(pair.first, pair.second)->setText(".");
    }

    if (!callFromHint){
        setAttemptsCount(attemptsLeft-1);
    }

}

void Fallout_Terminal::hintPressed(int index){
    //Убирание лишних слов и восстановление попыток
    //Возможно, с нотками рандома

    hints[index].is_pressed = true;

    int wordsLeft = 0;//Количество оставшиъся ненажатых слов
    for (auto & word : words){
        if (!word.is_answer && !word.is_pressed){
            wordsLeft++;
        }
    }

    if (wordsLeft == 1){
        //Восстановление количества попыток (и только)
        //qDebug()<<"todo Restore";
        setAttemptsCount(maxAttempts);
    } else {
        //Восстановление попыток или убирание слова

        if (effolkronium::random_static::get<float>(0, 1) < 0.2){
            setAttemptsCount(maxAttempts);
        } else {
            int ind = effolkronium::random_static::get<int>(0, wordsLeft-1);//Т.к. нельзя удалять ответ
            //qDebug()<<"todo randRestore"<<ind<<wordsLeft;

            for (int i=0; i<words.size(); i++){
                if (!words[i].is_answer && !words[i].is_pressed){
                    wordsLeft--;
                    if (wordsLeft == ind){
                        wordPressed(i, true);
                    }
                }
            }
        }
    }
}

void Fallout_Terminal::setAttemptsCount(int attempts){
    qDebug()<<"changeAttemptsCount"<<attemptsLeft<<"->"<<attempts;
    attemptsLeft = attempts;

    //temp
    QString titleString = "Терминал Fallout ";
    for (int i=0; i<attemptsLeft; i++){
        titleString += "▮";
    }
    setWindowTitle(titleString);//Название окна

    if (attemptsLeft <= 0){
        qDebug()<<"You lose";
        QMessageBox::information(this, "Oh no!", "Terminal blocked");
        //this->close();
        tuneTextTableWidget();
    }
}

void Fallout_Terminal::cellClicked(int row, int column){
    qDebug()<<"cellClicked"<<row<<column;

    //Если попали на клетку с индексом - ничего не делаем
    if (column%(symbolsInRow+1)==0){
        return;
    }

    this->cellEntered(row, column);//Выделение
    //Без повторного выделения вручную выделится только одна ячейка
    //(больше при растягивании)

    //Определяем индекс слова (если выбрано)
    int wordIndex = isWord(numS(row, column));
    if (wordIndex != -1){
        wordPressed(wordIndex);
        return;
    }

    //Проверка на подсказки () {} [] <>
    int hintIndex = isHint(numS(row, column));
    if (hintIndex != -1){
        hintPressed(hintIndex);
        return;
    }

    setAttemptsCount(attemptsLeft-1);
}
void Fallout_Terminal::cellEntered(int row, int column){
    //Убираем выделение
    for (int i=0; i<columnsCount*(symbolsInRow+1); i++){
        for (int j=0; j<rowsCount; j++){
            textTableWidget->item(j, i)->setSelected(false);
        }
    }

    //Если попали на клетку с индексом - не выделяем
    if (column%(symbolsInRow+1)==0){
        return;
    }

    //qDebug()<<"cellEntered"<<row<<column;

    //Определяем индекс слова (если выбрано)
    int wordIndex = isWord(numS(row, column));
    if (wordIndex != -1){
        for (int i=0; i<wordsSize; i++){
            auto pair = numV(words[wordIndex].start_index+i);
            textTableWidget->item(pair.first, pair.second)->setSelected(true);
        }
        //qDebug()<<row<<column<<words[wordIndex].word;
        return;
    }

    //Проверка на подсказки () {} [] <>
    int hintIndex = isHint(numS(row, column));
    if (hintIndex != -1){
        for (int i=hints[hintIndex].start_index; i<=hints[hintIndex].end_index; i++){
            auto pair = numV(i);
            textTableWidget->item(pair.first, pair.second)->setSelected(true);
        }
        return;
    }

    if (column%(symbolsInRow+1)!=0){
        textTableWidget->item(row,column)->setSelected(true);
    }
}
void Fallout_Terminal::cellPressed(int row, int column){
    //qDebug()<<"cellPressed"<<row<<column;
}
void Fallout_Terminal::itemSelectionChanged(){
    //Убираем выделение
    /*for (int i=0; i<columnsCount*(symbolsInRow+1); i++){
        for (int j=0; j<rowsCount; j++){
            textTableWidget->item(j, i)->setSelected(false);
        }
    }*/
    //qDebug()<<"itemSelectionChanged";
    //!this
}
void Fallout_Terminal::currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn){
    //qDebug()<<"currentCellChanged"<<currentRow<<currentColumn<<previousRow<<previousColumn;
    //this->cellEntered(currentRow, currentColumn);
    //Убираем выделение
    /*for (int i=0; i<columnsCount*(symbolsInRow+1); i++){
        for (int j=0; j<rowsCount; j++){
            textTableWidget->item(j, i)->setSelected(false);
        }
    }*/
}