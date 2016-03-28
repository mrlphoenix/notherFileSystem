#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "fileencoder.h"

#define NFS_DIR "c:/Qt/Static/5.5.1/"
#define NFS_PACK "qt_static_551.nfs"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    NFSFileSystem fs(this);
    connect(&fs, SIGNAL(progress(double,QString)),this,SLOT(onProgress(double,QString)));
    fs.buildFileSystem(NFS_DIR,NFS_PACK);

    ui->label->setText("DONE");
}

void MainWindow::onProgress(double percent, QString filename)
{
    ui->progressBar->setValue(percent*ui->progressBar->maximum());
    ui->label->setText(filename);
}

void MainWindow::on_pushButton_2_clicked()
{
    NFSFileSystem fs(this);
    connect(&fs, SIGNAL(progress(double,QString)),this,SLOT(onProgress(double,QString)));
    fs.rebaseFileSystem(NFS_PACK,"C:/GDTycoon/");
    ui->label->setText("DONE");
}

void MainWindow::on_pushButton_3_clicked()
{
    NFSFileSystem * fs = new NFSFileSystem(this);
    connect(fs, SIGNAL(progress(double,QString)),this,SLOT(onProgress(double,QString)));
    fs->load(NFS_PACK);
    ui->label->setText("DONE");
}
