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

    warningTimer = new QTimer(this);
    connect(warningTimer, SIGNAL(timeout()), this, SLOT(changeWarningState()));

    QGridLayout * mainLayout = new QGridLayout(this);
    textTableWidget = new QTextTableWidget();
    tuneTextTableWidget();
    newGame();
    mainLayout->addWidget(textTableWidget, 1, 0);

    connect(textTableWidget, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
    connect(textTableWidget, SIGNAL(currentCellChanged(int, int, int, int)), this, SLOT(currentCellChanged(int, int, int, int)));
    connect(textTableWidget, SIGNAL(cellClicked(int, int)), this, SLOT(cellClicked(int, int)));
    connect(textTableWidget, SIGNAL(cellEntered(int, int)), this, SLOT(cellEntered(int, int)));
    connect(textTableWidget, SIGNAL(cellPressed(int, int)), this, SLOT(cellPressed(int, int)));

    /*
    topTextTableWidget = new QTextTableWidget();
    mainLayout->addWidget(topTextTableWidget, 0, 0, 1, 2);

    rightTextTableWidget = new QTextTableWidget();
    mainLayout->addWidget(rightTextTableWidget, 1, 1);

    //Заглушка для верхней и правой полос
    tuneAnotherTableWidget(false);//right
    tuneAnotherTableWidget(true);//top
    */

}

Fallout_Terminal::~Fallout_Terminal(){
}

QVector<QString> Fallout_Terminal::getWordsFromDictionary(){

    //Переделывать под базу данных с запросами вроде
    //select * from table where id>5 order by rand() limit 5;
    //не самый хороший вариант. Читает и так быстро, а вот
    //запись данных в базу очень долгая (несколько минут
    //на один словарь)

    QFile file("litw-win.txt");
    if (!file.open(QIODevice::ReadOnly)){
        QString errorMessage = "Fallout_Terminal::getWordsFromDictionary() cannot open file";
        QMessageBox::critical(this, "Error", errorMessage);
        throw std::runtime_error(errorMessage.toStdString());
    }

    QTextStream fin(&file);
    fin.setCodec("UTF-8");//Для корректного отображения кириллицы
    QList<QString> wordsList;//С подходящей длиной
    QString newWord;
    int freq;//temp; dictionary style
    while (!fin.atEnd()){
        fin>>freq>>newWord;
        if (newWord.size()==wordsSize && freq!=1){
            wordsList.push_back(newWord);//.toUpper()
        }
    }

    //Вместо random_shuffle
    QVector<QString> wordsVector;
    for (int i=0; i<wordsCount; i++){
        int n = effolkronium::random_static::get<int>(0, wordsList.size()-1);
        wordsVector.push_back(wordsList[n].toUpper());
        wordsList.removeAt(n);
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
    //qDebug()<<"Answer is"<<words[answerNumber].word;

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
            //QString indString = "0x"+QString("%1").arg(start_index, 4, 16, QChar('0')).toUpper();
            //indString += ' ';
            //if (col!=0) indString = ' ' + indString;
            //total_column.push_back(indString);
            total_column.push_back("0x"+QString("%1").arg(start_index, 4, 16, QChar('0')).toUpper());
            start_index += symbolsInRow;
        }
        answer.push_back(std::move(total_column));
    }
    //qDebug()<<"0x"+QString::number(start_index, 16).toUpper();//to smth like "0x0CB8"
    //qDebug()<<QString("0x%1").arg(start_index, 4, 16, QChar('0'));//to smth like "0x0cb8"

    if (!answer.empty() && !answer[0].empty())
        symbolsInIndex = answer[0][0].size();

    return answer;
}

void Fallout_Terminal::tuneTextTableWidget(){
    textTableWidget->setBackground(QColor(  8,  34,  21));
    textTableWidget->setForeground(QColor( 55, 255, 137));

    QFont font("Consolas", 20);
    //QFont font("Consolas", 10, QFont::Monospace);
    font.setBold(true);
    textTableWidget->setFont(font);


    //font.setPixelSize(20);//Для работы с разными разрешениями?
    //QDesktopWidget *d = QApplication::desktop();
    //qDebug()<<d->width()<<d->height();//Разрешение экрана


    //Qt::ItemFlag flag = (Qt::ItemFlag)((uint32_t)Qt::ItemIsEditable | (uint32_t)Qt::ItemIsEnabled);
    textTableWidget->setFlag(Qt::ItemIsEditable);
    textTableWidget->setAlignment(Qt::AlignCenter);

    auto hackingIndexes = generateHackingIndexes();

    textTableWidget->horizontalHeader()->setMaximumSectionSize(15);
    textTableWidget->verticalHeader()  ->setMaximumSectionSize(28);
    //textTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);//Сильно замедляет заполнение
    //textTableWidget->verticalHeader()  ->setSectionResizeMode(QHeaderView::ResizeToContents);//Без этого не уменьшалось

    for (int row=0; row<rowsCount+5; row++){
        for (int col=0; col<rightRowSize + columnsCount*(symbolsInRow+hackingIndexes[0][0].size()+2); col++){
            textTableWidget->setText(row, col, " ");
            //textTableWidget->horizontalHeader()->setDefaultSectionSize(1);
            //textTableWidget->item(row, col);
        }
    }

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

void Fallout_Terminal::newGame(){
    //top
    {
        QString topString = "ROBCO INDUSTRIES (TM) TERMLINK PROTOCOL";
        int row = 0;
        for (int column=0; column<topString.size(); column++){
            textTableWidget->setText(row, column, QString(topString[column]));
        }
    }
    /*{
        QString topString = "ВВЕДИТЕ ПАРОЛЬ";
        int row = 1;
        int column;
        for (column=0; column<topString.size(); column++){
            textTableWidget->setText(row, column, QString(topString[column]));
        }
        for (; column<textTableWidget->columnCount(); column++){
            textTableWidget->setText(row, column, " ");
        }
    }*/
    //Всё перенесено в showWarning
    //Мигающее сообщение на второй строке обновляется функцией showWarning(bool)
    //Надпись "ВВЕДИТЕ ПАРОЛЬ" убирается после первой попытки ввода (?) - нет
    //"# ПОПЫТОК ОСТАЛОСЬ: ▮▮▮▮" - в функции setAttemptsCount(int)

    //right
    clearRightRows();

    auto hackingIndexes = generateHackingIndexes();

    for (int i=0; i<hackingIndexes.size(); i++){//section
        for (int j=0; j<hackingIndexes[0].size(); j++){//row in section
            for (int k=0; k<hackingIndexes[0][0].size(); k++){//num in index
                textTableWidget->setText(j+topRowsCount, i*(symbolsInRow+symbolsInIndex+2)+k, QString(hackingIndexes[i][j][k]));
            }
            //textTableWidget->setText(j,i*(1+symbolsInRow),ans[i][j]);
        }
    }
    //qDebug()<<"newGame 3";
    QString hackingSymbols = generateHackingSymbols();
    for (int i=0; i<hackingSymbols.size(); i++){
        auto pair = numV(i);
        textTableWidget->setText(pair.first, pair.second, QString(hackingSymbols[i]));
    }

    this->setAttemptsCount(maxAttempts);
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
    int row = numInSection / symbolsInRow + topRowsCount;
    int column = (section+1) * (symbolsInIndex+2) - 1 +//+2 за счёт пробела
            (section)*symbolsInRow +
            numInSection % symbolsInRow;
    return {row, column};
}

int Fallout_Terminal::numS(int row, int column){
    int answer = 0;
    int section = column/(symbolsInRow+symbolsInIndex+2);
    answer += section*symbolsInRow*rowsCount;
    answer += (row - topRowsCount)*symbolsInRow;
    answer += column - section*(symbolsInRow+symbolsInIndex+2) - 1 - symbolsInIndex;
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

int Fallout_Terminal::sameCharsAsAnswer(QString s){
    //Глупый перебор, ну да ладно
    //Слов не так много
    int sameCharsCount = 0;
    for (auto & w : words){
        if (w.is_answer){
            for (int i=0; i<std::min(s.size(), w.word.size()); i++){
                if (s[i]==w.word[i]){
                    sameCharsCount++;
                }
            }
            return sameCharsCount;
        }
    }
    return -1;//Сюда попасть не должны
    //Только если вдруг секретного слова-ответа не будет
}

void Fallout_Terminal::showWarning(bool warningEnabled){
    int row = 1;
    if (!warningEnabled){
        warningTimer->stop();
        QString topString = "ВВЕДИТЕ ПАРОЛЬ";
        int column;
        for (column=0; column<topString.size(); column++){
            textTableWidget->setText(row, column, QString(topString[column]));
        }
        for (; column<textTableWidget->columnCount(); column++){
            textTableWidget->setText(row, column, " ");
        }
    } else {
        warningShown = true;
        changeWarningState();
        //warningTimer->start(0);
    }
}
void Fallout_Terminal::changeWarningState(){
    int row = 1;
    if (warningShown) {
        QString topString = "!!! ПРЕДУПРЕЖДЕНИЕ: ТЕРМИНАЛ МОЖЕТ БЫТЬ ЗАБЛОКИРОВАН !!!";
        for (int column=0; column<textTableWidget->columnCount(); column++){
            textTableWidget->setText(row, column, QString(topString[column]));
        }
    } else {
        for (int column=0; column<textTableWidget->columnCount(); column++){
            textTableWidget->setText(row, column, " ");
        }
    }
    warningShown = !warningShown;
    qDebug()<<"warningShown"<<warningShown;

    warningTimer->start(1000);
}

void Fallout_Terminal::wordPressed(int index, bool callFromHint = false){
    //При неправильно выбранном слове отнимается 1 попытка
    //Если слово убирается не от нажатия подсказки
    words[index].is_pressed = true;

    if (words[index].is_answer){
        qDebug()<<"Answer found";

        addStringToRightRows(words[index].word);///////////////////////////////////////////////////////////////
        addStringToRightRows("Точно!");///////////////////////////////////////////////////////////////
        addStringToRightRows("Пожалуйста,");///////////////////////////////////////////////////////////////
        addStringToRightRows("подождите");///////////////////////////////////////////////////////////////
        addStringToRightRows("входа в систему");///////////////////////////////////////////////////////////////

        QMessageBox::information(this, "Hacked ;)", "Answer found");
        newGame();//tuneTextTableWidget();

        return;
    }

    for (int i=0; i<wordsSize; i++){
        auto pair = numV(words[index].start_index+i);
        textTableWidget->item(pair.first, pair.second)->setText(".");
    }


    if (!callFromHint){
        setAttemptsCount(attemptsLeft-1);

        addStringToRightRows(words[index].word);///////////////////////////////////////////////////////////////
        addStringToRightRows("Отказ в доступе");///////////////////////////////////////////////////////////////
        if (attemptsLeft<=0){
            addStringToRightRows("Блокировка");///////////////////////////////////////////////////////////////
            addStringToRightRows("начата.");///////////////////////////////////////////////////////////////

            qDebug()<<"You lose";
            QMessageBox::information(this, "Oh no!", "Terminal blocked");
            //this->close();
            newGame();//tuneTextTableWidget();

        } else {
        addStringToRightRows(QString::number(sameCharsAsAnswer(words[index].word)) +
                             "/" + QString::number(wordsSize) + " правильно.");///////////////////////////////////////////////////////////////
        }

    } //else "Заглушка убрана" - в hintPressed
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

        QString tempString;
        for (int i=hints[index].start_index; i<=hints[index].end_index; i++){
            auto pair = numV(i);
            tempString += textTableWidget->item(pair.first, pair.second)->text();
        }
        addStringToRightRows(tempString);///////////////////////////////////////////////////////////////
        addStringToRightRows("Пользование");///////////////////////////////////////////////////////////////
        addStringToRightRows("вновь разрешено");///////////////////////////////////////////////////////////////

        setAttemptsCount(maxAttempts);

    } else {
        //Восстановление попыток или убирание слова

        if (effolkronium::random_static::get<float>(0, 1) < 0.2){

            QString tempString;
            for (int i=hints[index].start_index; i<=hints[index].end_index; i++){
                auto pair = numV(i);
                tempString += textTableWidget->item(pair.first, pair.second)->text();
            }
            addStringToRightRows(tempString);///////////////////////////////////////////////////////////////
            addStringToRightRows("Пользование");///////////////////////////////////////////////////////////////
            addStringToRightRows("вновь разрешено");///////////////////////////////////////////////////////////////

            setAttemptsCount(maxAttempts);
        } else {

            QString tempString;
            for (int i=hints[index].start_index; i<=hints[index].end_index; i++){
                auto pair = numV(i);
                tempString += textTableWidget->item(pair.first, pair.second)->text();
            }
            addStringToRightRows(tempString);///////////////////////////////////////////////////////////////
            addStringToRightRows("Заглушка убрана");///////////////////////////////////////////////////////////////

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

    {
        QString topString;
        topString.setNum(attemptsLeft);
        topString += " ПОПЫТКИ ОСТАЛОСЬ: ";
        for (int i=0; i<attemptsLeft; i++){
            topString += "▮";
        }
        int row = 3;
        for (int column=0; column<rightRowSize + columnsCount*(symbolsInRow+symbolsInIndex+2); column++){
            if (column<topString.size())
                textTableWidget->setText(row, column, QString(topString[column]));
            else
                textTableWidget->setText(row, column, " ");
        }
    }

    if (attemptsLeft == 1) showWarning(1);
    if (attemptsLeft == maxAttempts) showWarning(0);

    //Как вариант - изменять падежи слова "ПОПЫТКИ". В оригинале - не меняется

    /*
    //temp
    QString titleString = "Терминал Fallout ";
    for (int i=0; i<attemptsLeft; i++){
        titleString += "▮";
    }
    setWindowTitle(titleString);//Название окна
    */
}

void Fallout_Terminal::addStringToRightRows(QString s){
    changeSelectedStringInRightRows("");//Очистка выбранной строчки, так функция вызывается при нажатии

    int addedRowsCount = ceil(s.size() / (rightRowSize-1.));
    //qDebug()<<"addStringToRightRows"<<rightRowSize<<s.size()<<addedRowsCount;
    for (int i=0; i<addedRowsCount; i++){
        rightRows.push_back('>'+s.mid(i*(rightRowSize-1), (rightRowSize-1.)));
    }
    while (rightRows.size()>rowsCount-2){
        rightRows.pop_front();//removeFirst() или removeAt(0) - тоже вариант
    }
    //qDebug()<<rightRows;

    //Заполнение ячеек
    for (int i=0; i<rightRows.size(); i++){
        //Строка rightRows[rightRows.size()-1-i] занимает строку rowsCount-3-i
        int j;
        for (j=0; j<rightRows[rightRows.size()-1-i].size(); j++){
            textTableWidget->setText(topRowsCount + rowsCount-3-i, j+columnsCount*(symbolsInRow+symbolsInIndex+2),
                                     QString(rightRows[rightRows.size()-1-i][j]));
        }
        for (; j<rightRowSize; j++){
            textTableWidget->setText(topRowsCount + rowsCount-3-i, j+columnsCount*(symbolsInRow+symbolsInIndex+2),
                                     " ");
        }
    }
}

void Fallout_Terminal::changeSelectedStringInRightRows(QString s){
    //Выбранная сейчас строчка под курсором, а именно та её часть, что влезает в одну строку
    textTableWidget->setText(topRowsCount + rowsCount-1, columnsCount*(symbolsInRow+symbolsInIndex+2),
                             ">");
    int i;
    for (i=0; i<std::min(s.size(), rightRowSize-1); i++){
        textTableWidget->setText(topRowsCount + rowsCount-1, 1+i+columnsCount*(symbolsInRow+symbolsInIndex+2),
                                 QString(s[i]));
    }
    for (; i<rightRowSize-1; i++){
        textTableWidget->setText(topRowsCount + rowsCount-1, 1+i+columnsCount*(symbolsInRow+symbolsInIndex+2),
                                 " ");
    }
}

void Fallout_Terminal::clearRightRows(){
    rightRows.clear();
    for (int i=0; i<rowsCount; i++){//row
        for (int j=0; j<rightRowSize; j++){//column
            textTableWidget->setText(i+topRowsCount, j+columnsCount*(symbolsInRow+symbolsInIndex+2), " ");
        }
    }
}

void Fallout_Terminal::cellClicked(int row, int column){
    //qDebug()<<"cellClicked"<<row<<column;

    //Не обрабатываем клетки с индексами, верхнюю и правую части
    if (row<topRowsCount ||
            column>(symbolsInRow+symbolsInIndex+2)*columnsCount-2 ||
            //Если попали на клетку с индексом - не выделяем
            column%(symbolsInRow+symbolsInIndex+2)<symbolsInIndex+1 ||
            column%(symbolsInRow+symbolsInIndex+2)==symbolsInRow+symbolsInIndex+1){
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
    addStringToRightRows(textTableWidget->item(row, column)->text());///////////////////////////////////////////////////////////////
    addStringToRightRows("Отказ в доступе");///////////////////////////////////////////////////////////////
    if (attemptsLeft<=0){
        addStringToRightRows("Блокировка");///////////////////////////////////////////////////////////////
        addStringToRightRows("начата.");///////////////////////////////////////////////////////////////

        qDebug()<<"You lose";
        QMessageBox::information(this, "Oh no!", "Terminal blocked");
        //this->close();
        newGame();//tuneTextTableWidget();

    } else {
        addStringToRightRows("0/" + QString::number(wordsSize) + " правильно.");///////////////////////////////////////////////////////////////
    }
}
void Fallout_Terminal::cellEntered(int row, int column){
    //Убираем выделение
    for (int i=0; i<textTableWidget->columnCount(); i++){
        for (int j=0; j<textTableWidget->rowCount(); j++){
            textTableWidget->item(j, i)->setSelected(false);
        }
    }

    //Не выделяем клетки с индексами, верхнюю и правую части
    if (row<topRowsCount ||
            column>(symbolsInRow+symbolsInIndex+2)*columnsCount-2 ||
            //Если попали на клетку с индексом - не выделяем
            column%(symbolsInRow+symbolsInIndex+2)<symbolsInIndex+1 ||
            column%(symbolsInRow+symbolsInIndex+2)==symbolsInRow+symbolsInIndex+1){
        changeSelectedStringInRightRows("");
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
        changeSelectedStringInRightRows(words[wordIndex].word);
        return;
    }

    //Проверка на подсказки () {} [] <>
    int hintIndex = isHint(numS(row, column));
    if (hintIndex != -1){
        QString hint;
        for (int i=hints[hintIndex].start_index; i<=hints[hintIndex].end_index; i++){
            auto pair = numV(i);
            textTableWidget->item(pair.first, pair.second)->setSelected(true);
            hint += textTableWidget->item(pair.first, pair.second)->text();
        }
        changeSelectedStringInRightRows(hint);
        return;
    }

    //if (column%(symbolsInRow+1)!=0){
        textTableWidget->item(row, column)->setSelected(true);
        changeSelectedStringInRightRows(QString(textTableWidget->item(row,column)->text()));
    //}
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
