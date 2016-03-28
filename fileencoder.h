#ifndef FILEENCODER_H
#define FILEENCODER_H

#include <QObject>
#include <QHash>
#include <QByteArray>
#define NFS_MAGIC_HEADER 0x4e465321

class NFSFileSystem : public QObject
{
    Q_OBJECT
public:
    explicit NFSFileSystem(QObject * parent);
    ~NFSFileSystem(){;}
    void buildFileSystem(QString dir, QString destFile);
    void rebaseFileSystem(QString srcFile, QString dir);
    void load(QString srcFile);
    void loadHeader(QString srcFile);
    void load(QString srcFile, QString name);
    void load(QString srcFile, QStringList nameList);
    QByteArray * get(QString srcFile, QString name);
signals:
    void progress(double percent, QString filename);
private:
    QHash<QString, QByteArray*> cache;
    QHash<QString, int> header;
};

class NFSFileEncoder : public QObject
{
    Q_OBJECT
public:
    explicit NFSFileEncoder(QObject *parent = 0);
    void build(QString srcFile, QString destFile);
    QByteArray read(QString srcFile, int offset, QString filename, int *skipSize = 0);
    int readHeader(QString srcFile, int offset);
    QByteArray generateLFSRData(int size, QString pass);
signals:
    void fileFound(QString);
public slots:
};

#endif // FILEENCODER_H
