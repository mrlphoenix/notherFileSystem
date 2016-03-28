#include <QCryptographicHash>
#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QDebug>
#include <QApplication>
#include "fileencoder.h"

NFSFileEncoder::NFSFileEncoder(QObject *parent) : QObject(parent)
{

}

void NFSFileEncoder::build(QString srcFile, QString destFile)
{
    QFile file(srcFile);
    QFileInfo fileInfo(srcFile);
    file.open(QFile::ReadOnly);
    QByteArray data = qCompress(file.readAll(),9);
    qApp->processEvents();
    QByteArray gamma = generateLFSRData(data.size(),fileInfo.baseName());
    qApp->processEvents();
    for(int i=0; i<data.size(); ++i)
        data[i] = data[i] ^ gamma[i];
    qApp->processEvents();

    QByteArray header;
    QDataStream ds(&header, QIODevice::WriteOnly);
    ds << int(NFS_MAGIC_HEADER);
    ds << data.size();

    QFile outFile(destFile);
    outFile.open(QIODevice::Append);
    outFile.write(header);
    outFile.write(data);
    outFile.flush();
    outFile.close();
    file.close();
}

QByteArray NFSFileEncoder::read(QString srcFile, int offset, QString filename, int * skipSize)
{
    QFile file(srcFile);
    file.open(QFile::ReadOnly);
    file.seek(offset);
    QByteArray header = file.read(sizeof(int)*2);
    QDataStream headerDS(header);
    int magic, dataSize;
    headerDS >> magic;
    headerDS >> dataSize;
    if (magic == NFS_MAGIC_HEADER)
    {
        if (skipSize)
            (*skipSize) = dataSize + sizeof(int)*2;
        QByteArray data = file.read(dataSize);
        qApp->processEvents();
        QFileInfo fileInfo(filename);
        QByteArray gamma = generateLFSRData(data.size(),fileInfo.baseName());
        qApp->processEvents();
        for (int i = 0; i< data.size(); ++i)
            data[i] = data[i] ^ gamma[i];
        qApp->processEvents();
        QByteArray uncompressedData = qUncompress(data);
        file.close();
        return uncompressedData;
    }
    return QByteArray();
}

int NFSFileEncoder::readHeader(QString srcFile, int offset)
{
    QFile file(srcFile);
    file.open(QFile::ReadOnly);
    file.seek(offset);
    QByteArray header = file.read(sizeof(int)*2);
    QDataStream headerDS(header);
    int magic, dataSize;
    headerDS >> magic;
    headerDS >> dataSize;
    file.close();
    if (magic == NFS_MAGIC_HEADER)
        return dataSize + sizeof(int) * 2;
    else return sizeof(int) * 2;
}

QByteArray NFSFileEncoder::generateLFSRData(int size, QString pass)
{
    QByteArray result;
    QByteArray hash = QCryptographicHash::hash(pass.toLocal8Bit(),QCryptographicHash::Md5);
    hash = hash.mid(0,4);
    QDataStream ds(hash);
    int seed;
    ds >> seed;
    for (int i=0; i<size; ++i)
    {
        char b = 0;
        for (int bIndex = 0; bIndex < 8; ++bIndex)
        {
            if (seed & 0x00000001)
            {
                seed = (seed ^ 0x80000057 >> 1) | 0x80000000;
                b |= (1 << bIndex);
            }
            else
                seed >>= 1;
        }
        result.append(b);
    }
    return result;
}


NFSFileSystem::NFSFileSystem(QObject *parent) : QObject(parent)
{

}

void NFSFileSystem::buildFileSystem(QString dir, QString destFile)
{
    QDir currentDir(dir);
    QStringList dirs, files;
    QDirIterator dirIt(dir,QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden,QDirIterator::Subdirectories);
    while(dirIt.hasNext())
        dirs.append(currentDir.relativeFilePath(dirIt.next()));
    QDirIterator filesIt(dir,QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden,QDirIterator::Subdirectories);
    while(filesIt.hasNext())
        files.append(currentDir.relativeFilePath(filesIt.next()));
    QByteArray fsHeader;
    QDataStream ds(&fsHeader, QIODevice::WriteOnly);
    ds << NFS_MAGIC_HEADER;
    ds << dirs.count();
    ds << files.count();
    foreach (const QString &s, dirs)
        ds << s;
    foreach (const QString &s, files)
        ds << s;
    ds << fsHeader.size() + 4;
    QFile outFile(destFile);
    outFile.open(QFile::WriteOnly);
    outFile.write(fsHeader);
    outFile.flush();
    outFile.close();

    NFSFileEncoder encoder(this);
    double i = 0;
    foreach (const QString &s, files)
    {
        encoder.build(dir + s, destFile);
        emit progress(i/double(files.count()), s);
        i+=1;
        qApp->processEvents();
    }
}

void NFSFileSystem::rebaseFileSystem(QString srcFile, QString dir)
{
    QFile fs(srcFile);
    fs.open(QFile::ReadOnly);
    QDataStream ds(&fs);
    int magic, dirCount, fileCount;
    ds >> magic;
    if (magic == NFS_MAGIC_HEADER)
    {
        ds >> dirCount;
        ds >> fileCount;
        QStringList dirs, files;
        for (int i = 0; i < dirCount; ++i)
        {
            QString currentDir;
            ds >> currentDir;
            dirs.append(currentDir);
        }
        for (int i = 0; i < fileCount; ++i)
        {
            QString currentFile;
            ds >> currentFile;
            files.append(currentFile);
        }
        foreach (const QString &d, dirs)
            QDir().mkdir(dir + d);
        int offset;
        ds >> offset;
        NFSFileEncoder encoder(this);
        double i = 0;
        foreach (const QString &f, files)
        {
            emit progress(i/double(files.count()),f);
            int skipSize = 0;
            QByteArray data = encoder.read(srcFile,offset,f,&skipSize);
            qApp->processEvents();
            QFile out(dir + f);
            out.open(QFile::WriteOnly);
            out.write(data);
            out.flush();
            out.close();
            offset += skipSize;
            i += 1;
        }
    }
}

void NFSFileSystem::load(QString srcFile)
{
    QFile fs(srcFile);
    fs.open(QFile::ReadOnly);
    QDataStream ds(&fs);
    int magic, dirCount, fileCount;
    ds >> magic;
    if (magic == NFS_MAGIC_HEADER)
    {
        ds >> dirCount;
        ds >> fileCount;
        QStringList dirs, files;
        for (int i = 0; i < dirCount; ++i)
        {
            QString currentDir;
            ds >> currentDir;
            dirs.append(currentDir);
        }
        for (int i = 0; i < fileCount; ++i)
        {
            QString currentFile;
            ds >> currentFile;
            files.append(currentFile);
        }
        int offset;
        ds >> offset;
        NFSFileEncoder encoder(this);
        double i = 0;
        foreach (const QString &f, files)
        {
            emit progress(i/double(files.count()),f);
            int skipSize = 0;
            QByteArray * data = new QByteArray();
            data->append(encoder.read(srcFile,offset,f,&skipSize));
            qApp->processEvents();
            cache[f] = data;
            offset += skipSize;
            i += 1;
        }
    }
}

void NFSFileSystem::loadHeader(QString srcFile)
{
    QFile fs(srcFile);
    fs.open(QFile::ReadOnly);
    QDataStream ds(&fs);
    int magic, dirCount, fileCount;
    ds >> magic;
    if (magic == NFS_MAGIC_HEADER)
    {
        ds >> dirCount;
        ds >> fileCount;
        QStringList dirs, files;
        for (int i = 0; i < dirCount; ++i)
        {
            QString currentDir;
            ds >> currentDir;
            dirs.append(currentDir);
        }
        for (int i = 0; i < fileCount; ++i)
        {
            QString currentFile;
            ds >> currentFile;
            files.append(currentFile);
        }
        int offset;
        ds >> offset;
        NFSFileEncoder encoder(this);
        double i = 0;
        foreach (const QString &f, files)
        {
            emit progress(i/double(files.count()),f);
            int skipSize = encoder.readHeader(srcFile,offset);
            header[f] = offset;
            offset += skipSize;
            i += 1;
        }
    }
}

void NFSFileSystem::load(QString srcFile, QString name)
{
    if (!cache.contains(name))
    {
        if (header.contains(name))
        {
            NFSFileEncoder encoder;
            QByteArray * data = new QByteArray();
            data->append(encoder.read(srcFile,header[name],name));
            qApp->processEvents();
            cache[name] = data;
        }
        else
        {
            QFile newFile(name);
            if (newFile.exists())
            {
                newFile.open(QFile::ReadOnly);
                cache[name] = new QByteArray(newFile.readAll());
            }
            newFile.close();
        }
    }
}

void NFSFileSystem::load(QString srcFile, QStringList nameList)
{
    foreach (const QString &s, nameList)
        load(srcFile,s);
}

QByteArray *NFSFileSystem::get(QString srcFile, QString name)
{
    load(srcFile,name);
    if (cache.contains(name))
        return cache[name];
    else return 0;
}
