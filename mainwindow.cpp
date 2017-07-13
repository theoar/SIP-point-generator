#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initTable();

    onPrzesuniecieChange();
    ui->ProgresBar->hide();

}

MainWindow::~MainWindow()
{
    clearAllPoints();
    delete ui;
}

void MainWindow::onButtonSave()
{
    QString FilePath = QFileDialog::getSaveFileName(this, "Zapisz pliki");

    if(FilePath.isNull() || FilePath.isEmpty())
        return;

    QFile File(FilePath);

    if(!File.open(QFile::WriteOnly))
    {
        QMessageBox::critical(this, "Błąd", QString("Nie można otworzyć pliku %1").arg(File.fileName()));
        return;
    }

    ui->ProgresBar->show();
    ui->ProgresBar->setRange(0, PunktyZrobione.length());

    QTextStream Stream(&File);
    Stream.setRealNumberPrecision(24);
    Stream.setRealNumberNotation(QTextStream::ScientificNotation);

    for(Point & X : PunktyZrobione)
    {
        Stream << X.X  << ' ';
        Stream << X.Y << ' ';
        Stream << X.Wys << ' ';
        Stream << X.Kat << ' ';
        Stream << X.Flaga << ' ';

        if(!qIsNaN(X.XOdn))
            Stream << X.XOdn << ' ';
        if(!qIsNaN(X.YOdn))
            Stream << X.YOdn << ' ';

        Stream << '"';

        if(!qIsNaN(X.HUp))
            Stream << QString::number(X.HUp, 'f', 2);
        if(!qIsNaN(X.HDown))
            Stream << '|' <<QString::number(X.HDown, 'f', 2);

        Stream << '"' << ' ';
        Stream << X.Kerg;
        Stream << "\r\n";

        ui->ProgresBar->setValue(ui->ProgresBar->value()+1);
    }

    clearAllPoints();

    ui->statusBar->showMessage(QString("Zapisano pliki: %1" ).arg(
                                   File.fileName()));
    ui->ProgresBar->hide();

    File.close();

    ui->ButtonConvert->setEnabled(false);
    ui->ButtonSave->setEnabled(false);

    this->setWindowTitle("Konwerter");
}

void MainWindow::onButtonOpen()
{    
    QString FilePath = QFileDialog::getOpenFileName(this, "Owtórz plik");

    if(FilePath.isNull() || FilePath.isEmpty())
        return;

    QFile File(FilePath);
    if(!File.open(QFile::ReadOnly))
    {
        QMessageBox::critical(this, "Błąd", QString("Nie można otworzyć pliku %1").arg(FilePath));
        return;
    }

    QTextStream Stream(&File);
    Stream.setRealNumberPrecision(24);
    Stream.setRealNumberNotation(QTextStream::ScientificNotation);

    clearAllPoints();

    ui->ProgresBar->show();
    ui->ProgresBar->setRange(0, File.size());
    ui->ProgresBar->setValue(0);

    ui->statusBar->showMessage("Rozpoczęto przetwarzanie..");
    QString Line;
    while(!Stream.atEnd())
    {
        ui->ProgresBar->setValue(ui->ProgresBar->value()+Line.toLocal8Bit().size());

        Line = Stream.readLine();
        QString Temp;

        QRegExp Cleaner("\"(.+)\"");
        Cleaner.indexIn(Line);
        if(Cleaner.capturedTexts().length()>1)
            Line.replace(Cleaner, "\"" + Cleaner.capturedTexts()[1].trimmed() + "\"");

        QStringList Lista = Line.split(' ', QString::SkipEmptyParts);

        if(Lista.length()<8)
            continue;

        QTextStream LineStream(&Line);
        LineStream.setRealNumberNotation(QTextStream::RealNumberNotation::ScientificNotation);
        LineStream.setRealNumberPrecision(24);

        Point *NewPoint = new Point();

        LineStream >> Temp;
        LineStream >> NewPoint->X;
        LineStream >> NewPoint->Y;
        LineStream >> NewPoint->Wys;
        LineStream >> NewPoint->Kat;
        LineStream >> NewPoint->Flaga;

        if(Lista.length()>9)
        {
            LineStream >> NewPoint->XOdn;
            LineStream >> NewPoint->YOdn;
        }

        LineStream >> Temp;
        QRegExp Wys("\\d+\\.");
        if(Temp[0]!= '"' || Temp[Temp.length()-1]!='"' || !Temp.contains(Wys))
        {
            delete NewPoint;
            continue;
        }

        QStringList UpDown = Temp.remove('"').split('|');
        QVector<double> Vector;
        bool Ok;

        for(int x = 0; x<2 && x<UpDown.length(); ++x)
        {
            double Wysokosc = 0;

            Temp=UpDown.at(x);
            Temp=Temp.trimmed();
            Wys.setPattern("(\\d+\\.\\d+)");
            Wys.indexIn(Temp);
            if(Wys.capturedTexts().length()>1)
            {
                double H = Wys.capturedTexts().at(1).toDouble(&Ok);
                if(Ok)
                    Wysokosc = H;
                else
                    Wysokosc = NAN; chuj
            }

            Vector.push_back(Wysokosc);
        }

        NewPoint->HUp = Vector.length()>0 ? Vector[0]  : 0;
        NewPoint->HDown = Vector.length()>1 ? Vector[1] : NAN;

        LineStream >> NewPoint->Kerg;

        Points.push_back(QPair<Point*, bool>(NewPoint, false));
        insertItem(NewPoint);
    }

    ui->ButtonConvert->setEnabled(Points.length()>0);
    ui->statusBar->showMessage(QString("Liczba wczytanych punków: %1").arg(QString::number(Points.length())));
    ui->ProgresBar->hide();

    this->setWindowTitle(File.fileName());
    QApplication::beep();

    File.close();

}

void MainWindow::onButtonConvert()
{
    ui->statusBar->showMessage("Konwertowanie punktów");
    ui->ProgresBar->show();
    ui->ProgresBar->setRange(0, Points.length());
    ui->ProgresBar->setValue(0);

    double MaxLenght = distance(ui->SpinBoxDeltaX->value(), ui->SpinBoxDeltaY->value());
    double MaxKatDiff = ui->SpinBoxKatDiff->value();
    double MaxKat = ui->SpinBoxKat->value();
    double MaxDy = ui->SpinBoxDeltaY->value();
    double MaxDx = ui->SpinBoxDeltaX->value();

    int LicznikPojedynczych = 0;
    int LicznikDodwojnych = 0;
    int LicznikZlych = 0;
    int Wykorzystane = 0;

    PunktyZrobione.clear();

    for(QPair<Point*, bool> & P : Points)
        P.second=false;

    Point* DopGora;
    Point* DopDol;

    double Distance;
    double DistanceBest;

    QPair<Point*, bool> *P1;
    QPair<Point*, bool> *P2;

    for(QPair<Point*, bool> & P : Points)
    {
        Point* X = P.first;

        if(katDiff(X)>MaxKat)
        {
            PunktyZrobione.push_back(*X);
            LicznikZlych++;
            P.second=true;
            ui->ProgresBar->setValue(ui->ProgresBar->value()+1);
            continue;
        }

        if( !qIsNaN(X->HUp) && !qIsNaN(X->HDown) )
        {
            PunktyZrobione.push_back(*X);
            LicznikDodwojnych++;
            P.second=true;
            ui->ProgresBar->setValue(ui->ProgresBar->value()+1);
            continue;
        }

        DistanceBest = MaxLenght+1;
        DopGora = nullptr;
        DopDol = nullptr;

        if(P.second)
            continue;

        for(QPair<Point*, bool> & PP : Points)
        {
            Point* Y = PP.first;

            Point* Gora = nullptr;
            Point* Dol = nullptr;

            if(PP.second)
                continue;

            if(Y==X)
                continue;


            Distance = distance(X, Y);
            if((Distance>MaxLenght) ||
                    (Distance>DistanceBest) ||
                    ( przyrost(X->X, Y->X) > MaxDx ) ||
                    ( przyrost(X->Y, Y->Y) > MaxDy ) ||
                    ( (katDiff(X)+katDiff(Y))>MaxKatDiff ) )
                continue;

            DistanceBest = Distance;

            if(X->X>Y->X)
            {
                Gora = X;
                Dol = Y;
            }
            else
            {
                Gora = Y;
                Dol = X;
            }

            if( !(Gora->Flaga & PODKRESLENIE) && !(Dol->Flaga & NADKRESLENIE) )
            {
                DistanceBest = MaxLenght+1;
                continue;
            }

            if(Gora->HUp>Dol->HUp)
            {
                DopGora=Gora;
                DopDol=Dol;
                P2=&PP;
            }

        }

        P1=&P;

        if(DopGora==nullptr || DopDol==nullptr)
        {
            PunktyZrobione.push_back(*X);
            LicznikPojedynczych++;
            ui->ProgresBar->setValue(ui->ProgresBar->value()+1);
            P1->second=true;
        }
        else
        {

            Point Punkt = *DopGora;

            if(DopGora->Kerg != DopDol->Kerg)
                Punkt.Kerg = fitKerg(DopGora, DopDol);

            Punkt.HDown=DopDol->HUp;
            Punkt.Flaga|=PODKRESLENIE;

            PunktyZrobione.push_back(Punkt);
            if(qIsNaN(DopGora->XOdn) &&
                    qIsNaN(DopGora->YOdn) &&
                    !qIsNaN(DopDol->XOdn) &&
                    !qIsNaN(DopDol->YOdn))
            {
                Punkt.XOdn = DopDol->XOdn;
                Punkt.YOdn = DopDol->YOdn;
            }

            P1->second=true;
            P2->second=true;

            int Flagi = ((DopGora->Flaga & 0b1111) + (DopDol->Flaga & 0b1111))/2;
            Punkt.Flaga = DopGora->Flaga & (~0b1111);
            Punkt.Flaga = DopGora->Flaga | Flagi;

            LicznikDodwojnych++;

            ui->ProgresBar->setValue(ui->ProgresBar->value()+2);
        }

    }

    Wykorzystane=0;
    for(auto & X : Points)
        if(X.second==true)
            Wykorzystane++;


    ui->statusBar->showMessage(QString("Liczba punktow Górna-Dolna: %1 <> Pojedynczych: %2 <> Uznanych za wysokości osi: %3 <> Użytych: %4 <>  Wczytanych %5").arg(
                                   QString::number(LicznikDodwojnych),
                                   QString::number(LicznikPojedynczych),
                                   QString::number(LicznikZlych),
                                   QString::number(Wykorzystane),
                                   QString::number(Points.length()))
                               );
    ui->ProgresBar->hide();
    ui->ButtonSave->setEnabled(true);
    ui->ButtonSave->setEnabled(PunktyZrobione.length()>0);

    QApplication::beep();

}

void MainWindow::onPrzesuniecieChange()
{
    ui->SpinBoxOdl->setValue(qSqrt(qPow(ui->SpinBoxDeltaX->value(),2)+qPow(ui->SpinBoxDeltaY->value(),2)));
}

void MainWindow::initTable()
{
    ui->Table->setColumnCount(9);
    ui->Table->setHorizontalHeaderLabels(Headers);
    ui->Table->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::insertItem(MainWindow::Point *Item)
{
    int row = ui->Table->rowCount();
    ui->Table->insertRow(row);

    ui->Table->setItem(row, 0, new QTableWidgetItem( QString::number(Item->X)  ));
    ui->Table->setItem(row, 1, new QTableWidgetItem( QString::number(Item->Y) ));
    ui->Table->setItem(row, 2, new QTableWidgetItem(QString::number(Item->Wys) ));
    ui->Table->setItem(row, 3, new QTableWidgetItem(QString::number(Item->Kat) ));
    ui->Table->setItem(row, 4, new QTableWidgetItem(QString::number(Item->Flaga) ));
    ui->Table->setItem(row, 5, new QTableWidgetItem(QString::number(Item->XOdn) ));
    ui->Table->setItem(row, 6, new QTableWidgetItem(QString::number(Item->YOdn) ));
    ui->Table->setItem(row, 7, new QTableWidgetItem(QString::number(Item->HUp) + "|" + QString::number(Item->HDown) ));
    ui->Table->setItem(row, 8, new QTableWidgetItem(Item->Kerg));
}

void MainWindow::clearAllPoints()
{
    for(auto & X : Points)
        delete X.first;

    Points.clear();
    PunktyZrobione.clear();

    ui->Table->setRowCount(0);
}

double MainWindow::distance(MainWindow::Point *A, MainWindow::Point *B)
{
    return qSqrt(qPow(B->X-A->X, 2)+qPow(B->Y-A->Y, 2));
}

double MainWindow::distance(double Dy, double Dx)
{
    return qSqrt(qPow(Dy,2)+qPow(Dx,2));
}

double MainWindow::katDiff(MainWindow::Point *A)
{
    if(A->Kat>180)
        return 360-A->Kat;
    else
        return A->Kat;
}

double MainWindow::przyrost(double Wsp1, double Wsp2)
{
    return qAbs(Wsp1-Wsp2);
}

QString MainWindow::fitKerg(Point *A, Point *B)
{

    int RokA, RokB;
    int NumA, NumB;
    int IndexNumer, IndexRok;

    QRegExp Reg("#P\\.\\d+\\.(\\d+)\\.(\\d+)");

    if(A->Kerg=="_")
        return B->Kerg;
    if(B->Kerg=="_")
        return A->Kerg;

    if(A->Kerg.contains(Reg) && !B->Kerg.contains(Reg))
        return A->Kerg;
    if(B->Kerg.contains(Reg) && !A->Kerg.contains(Reg))
        return B->Kerg;

    if(A->Kerg.contains(Reg) && B->Kerg.contains(Reg))
    {
        IndexNumer = 2;
        IndexRok = 1;
    }
    else
    {
        Reg.setPattern("(\\d+)\\/(\\d+)");
        IndexNumer = 1;
        IndexRok = 2;
    }

    Reg.indexIn(A->Kerg);
    QString Rok = Reg.cap(IndexRok);
    RokA = Reg.cap(IndexRok).toInt();
    NumA = Reg.cap(IndexNumer).toInt();

    if(Rok[0]=='0')
        RokA+=2000;
    else
        if(Rok.length()<3)
            RokA+=1900;

    Reg.indexIn(B->Kerg);
    Rok = Reg.cap(IndexRok);
    RokB = Reg.cap(IndexRok).toInt();
    NumB = Reg.cap(IndexNumer).toInt();

    if(Rok[0]=='0')
        RokB+=2000;
    else
        if(Rok.length()<3)
            RokB+=1900;

    if(RokA>RokB)
        return A->Kerg;
    if(RokB>RokA)
        return B->Kerg;
    if(NumA>NumB)
        return A->Kerg;
    if(NumB>NumA)
        return B->Kerg;

    return A->Kerg;


}


