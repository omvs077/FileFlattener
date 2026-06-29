#pragma once
#include <QDialog>
#include "FileScanner.h"
#include "Deduplicator.h"
#include "Renamer.h"

class QLabel;

class PreviewDialog : public QDialog {
    Q_OBJECT

public:
    PreviewDialog(
        const ScanResult& scan,
        const DedupResult& dedup,
        const std::vector<FlattenedFile>& flattened,
        const QString& targetZipPath,
        QWidget* parent = nullptr
    );
};
