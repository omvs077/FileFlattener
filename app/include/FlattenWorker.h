#pragma once
#include <QObject>
#include <QAtomicInt>
#include "FileScanner.h"
#include "Deduplicator.h"
#include "Renamer.h"

class FlattenWorker : public QObject {
    Q_OBJECT
public:
    FlattenWorker(
        ScanResult scan,
        QString targetZipPath,
        QString projectName,
        QObject* parent = nullptr
    );

public slots:
    void run();
    void requestCancel();

signals:
    void stageChanged(QString stageLabel);
    void progressChanged(int percent);
    void currentFile(QString filename);
    void finishedSuccess(QString zipPath);
    void finishedError(QString errorMessage);
    void finishedCancelled();

private:
    ScanResult m_scan;
    QString m_targetZipPath;
    QString m_projectName;
    QAtomicInt m_cancelRequested { 0 };
};
