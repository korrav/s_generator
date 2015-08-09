#ifndef SAMPLES_H
#define SAMPLES_H
#include <array>
#include <algorithm>
#include <random>
#include <memory>
#include <map>
#include <QtGlobal>
#include <ctime>
#include <QDebug>
typedef std::array<qint16, 4> Data;

class Samples
{
public:
    typedef std::shared_ptr<std::map<quint8, qint16>> SnapData;
private:
    SnapData psnap_ = nullptr;    //внеочередные данные
protected:
    virtual Data getData(void) = 0;
public:
    Samples(){}
    Data getDataSamples(void) {
        if(psnap_ == nullptr)
            return getData();
        else {
            Data data = getData();
            for(auto el : *psnap_)
                if(el.first < data.size())
                    data[el.first] = el.second;
            psnap_ = nullptr;
            return data;
        }
    }

    void setSnap(SnapData ps) {
        psnap_ = ps;
//        for(auto p : *psnap_)
//        qDebug() << "channel: " << p.first << " value: " << p.second;
    }

};

class FixSamples: public Samples{   //класс возвращает фиксированное значение отсчётов
    Data data_;
public:
    FixSamples(const Data& d): data_(d) {}
    void setData(const Data& d) {
        data_ = d;
    }
    FixSamples() = default;
    FixSamples(const FixSamples&) = default;
    Data getData() {
        return data_;
    }
};

class RandomSamples: public Samples {
    std::default_random_engine dre_;
    std::normal_distribution<qreal> di_;
public:
    RandomSamples(qint16 m, qint16 s): dre_(clock()), di_(m, s) {}
    RandomSamples(): di_(0, 1000) {}
    RandomSamples(const RandomSamples&) = default;
    void setParam(qint16 m, qint16 s){
        di_.param(std::normal_distribution<qreal>::param_type(m, s));
    }
    Data getData() {
        Data data;
        std::generate(data.begin(), data.end(),std::bind(std::ref(di_), std::ref(dre_)));
        return data;
    }
};

#endif // SAMPLES_H
