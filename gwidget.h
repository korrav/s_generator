#ifndef GWIDGET_H
#define GWIDGET_H

#include <QWidget>
#include <QString>
#include <QFile>
#include <array>
#include <QDataStream>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QMessageBox>
#include <algorithm>
#include <QEvent>
#include "samples.h"

class FocusFilter : public QObject {
    Q_OBJECT
protected:
    virtual bool eventFilter(QObject* pobj, QEvent* pe) {
        if(pe->type() == QEvent::FocusIn) {
            emit selectFocus(pobj);
        }
        return false;
    }

public:
    FocusFilter(QObject* pobj = 0): QObject(pobj){}
    ~FocusFilter(){}
signals:
    void selectFocus(QObject*);
};


class GWidget : public QWidget
{
    Q_OBJECT
    const QString DEFAULT_NAME_FIFO = "/tmp/pdata";
    const qint16 DEFAULT_VALUE = 0; //исходное значение величины отсчётов каналов
    const qint16 DEFAULT_SAMPLE_PERIOD = 0; //исходное значение периода выборки отсчётов
    qint64 atime_; //абсолютное время
    qint64 dtime_; //относительное время (мс)
    int idTimer_ = 0;   //идентификатор запущенного таймера
    std::shared_ptr<Samples> pgsampl_;   //генератор отсчётов
    QFile fifo_;   //файл именованного канала
    QDataStream stream_; //двоичный поток, управляющий записью/чтением данных из именованного буфера

    /*Элементы создания файла именованного канала*/
    QGroupBox gNameFifo_;
    QLineEdit edNameFifo_;   //указано имя файла именованного канала
    QPushButton butSelectFifo_;  //кнопка выбора файла именнованного канала
    QPushButton butEnterNameFifo_;  //кнопка инициирования создания именнованного канала, указанного в редакторе ввода

    /*Элементы установки значения периода выборки*/
    QGroupBox gSelectDtime_;
    QLabel labSelectDtime_;
    QLineEdit edSelectDtime_;
    QPushButton butEnterDtime_;

    /*Элементы выбора режима генерирования выборок*/
    QGroupBox gModeGenerateSamples_;
    QRadioButton butFixSamples_;
    QRadioButton butRandomSamples_;
    QLineEdit edFixSamples_[4];
    QPushButton butEnterFixSamples_;
    QLabel labMeanRandomSamples_;
    QLabel labSigmaRandomSamples_;
    QLineEdit edMeanRandomSamples_;
    QLineEdit edSigmaRandomSamples_;
    QPushButton butEnterRandomSamples_;

    /*Элементы управления инициирования внеочередного отсчёта*/
    QGroupBox gSnapSamples_;
    QLineEdit edSnapSamples_[4];
    QPushButton butEnterSnapSamples_;

    /*Элементы управления работой генератора отсчётов*/
    QGroupBox gRunning_;
    QButtonGroup bgRunning_;
    QPushButton butRun_;
    QPushButton butStop_;

private:
    void writeUnit(void);   //записать очередной блок данных
    qint64 getAtime(void);    //получить atime
    qint64 getDtime(void);    //получить dtime

    void createControlsSelectDtime(void);   //Создание элементов установки значения периода выборки
    void createControlsFifo(void);  //Создание элементов управления файлом именованного канала
    void createControlsModeGenerateSamples(void);   //Создание элементов управления режимом генерации выборок
    void createControlsSnapSamples(void);   //Создание элементов управления инициированим внеочередного отсчёта
    void createControlsRunning(void);   //Создание элементов управления работой генератора отсчётов

private slots:
    void createFifo(QString name);   //создать новый канал
    void changeNameFifoInEditor(void);  //реагировать на измененение содержимого редактора имени именованного канала
    void selectFullNameFifo(void);  //выбрать в диалоговом окне полное имя файла именнованного канала
    void setSnapUnit(Samples::SnapData ps); //установить внеочередные данные
    void setDtime(qint64 t);    //установить dtime
    void setAtime(qint64 a);    //установить atime
    void setGeneratorSamples(std::shared_ptr<Samples> pg); //установить генератор отсчётов
    void stopGeneratorSamples(void);   //остановить генератор отсчётов
    void activateControls(bool status);    //активировать элементы управления
    void runGeneratorSamples(bool status);  //запустить/остановить генератор отсчётов
    void selectEditorFixSamplesModes(void);   //выбрать редактор режима фиксированных выборок
    void setIntervalRandomParameter(QObject* pobj); //установка допустимого интервала значений параметра произвольных выборок
    void selectEditorRandomSamplesModes(void);   //выбрать редактор режима произвольных выборок
signals:
    void statusCompleteCreateFifo(bool);    //завершение открытия файла именнованного канала (true - завершено успешно)
protected:
    void timerEvent(QTimerEvent *event);
public:
    GWidget(QWidget *parent = 0);
    ~GWidget();
};

#endif // GWIDGET_H
