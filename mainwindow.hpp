#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QStringList>
#include <QString>
#include <QtMath>
#include <QTextStream>
#include <QDebug>
#include <QList>
#include <QRegExp>

#define PODKRESLENIE 0b10000
#define NADKRESLENIE 0b1000000

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    struct Point{
        double X = NAN;
        double Y = NAN;
        double Wys = NAN;
        double Kat = NAN;
        int Flaga = -1;
        double HUp = NAN;
        double HDown = NAN;
        double XOdn = NAN;
        double YOdn = NAN ;
        QString Kerg;
    };

public slots:
    void onButtonSave();
    void onButtonOpen();
    void onButtonConvert();
    void onPrzesuniecieChange();

public:
    void initTable();
    void insertItem(Point * Item);
    void clearAllPoints();
    double distance(Point* A, Point* B);
    double distance(double Dy, double Dx);
    double katDiff(Point *A);
    double przyrost(double Wsp1, double Wsp2);
    QString fitKerg(Point *A, Point *B);
private:
    Ui::MainWindow *ui;

    QList<QPair<Point*, bool>> Points;   
    QList<Point> PunktyZrobione;
    QStringList Headers = { "X", "Y", "Wys", "Kąt", "Flaga", "XOdn", "YDon", "H", "Kerg" };

};

#endif // MAINWINDOW_HPP
