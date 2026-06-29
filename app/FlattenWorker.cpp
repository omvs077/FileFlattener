#include "FlattenWorker.h"
#include "ZipWriter.h"
#include "StructureExporter.h"

FlattenWorker::FlattenWorker(ScanResult scan, QString targetZipPath, QString projectName, QObject* parent)
    : QObject(parent)
    , m_scan(std::move(scan))
    , m_targetZipPath(std::move(targetZipPath))
    , m_projectName(std::move(projectName))
{
}

void FlattenWorker::run() {
    std::string err;

    emit stageChanged("Checking for duplicates...");
    emit progressChanged(0);

    Deduplicator dedup;
    DedupResult dedupResult;
    bool dedupOk = dedup.process(m_scan, dedupResult, err, [this](size_t done, size_t total) {
        if (total > 0) {
            int pct = static_cast<int>((static_cast<double>(done) / total) * 40.0); // dedup = 0-40%
            emit progressChanged(pct);
        }
    });

    if (!dedupOk) {
        emit finishedError(QString::fromStdString(err));
        return;
    }

    emit stageChanged("Resolving file names...");
    emit progressChanged(40);

    Renamer renamer;
    auto flattened = renamer.resolve(m_scan, dedupResult);

    std::string structureText = StructureExporter::buildStructureText(m_scan, m_projectName.toStdString());
    std::string manifestJson = StructureExporter::buildManifestJson(m_scan, flattened, m_projectName.toStdString());

    emit stageChanged("Writing ZIP archive...");
    emit progressChanged(45);

    ZipWriter writer;
    bool zipOk = writer.writeZip(
        m_scan, flattened, m_targetZipPath.toStdString(), err,
        structureText, manifestJson,
        [this](uint64_t written, uint64_t total) {
            if (total > 0) {
                int pct = 45 + static_cast<int>((static_cast<double>(written) / total) * 55.0); // zip = 45-100%
                emit progressChanged(pct);
            }
        }
    );

    if (!zipOk) {
        emit finishedError(QString::fromStdString(err));
        return;
    }

    emit progressChanged(100);
    emit finishedSuccess(m_targetZipPath);
}
