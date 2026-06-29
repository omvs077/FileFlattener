#include "PreviewDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>

PreviewDialog::PreviewDialog(
    const ScanResult& scan,
    const DedupResult& dedup,
    const std::vector<FlattenedFile>& flattened,
    const QString& targetZipPath,
    QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Export Preview Summary");
    setMinimumWidth(480);

    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* title = new QLabel("<b>EXPORT PREVIEW SUMMARY</b>");
    layout->addWidget(title);

    QLabel* targetLabel = new QLabel(QString("Target ZIP: %1").arg(targetZipPath));
    targetLabel->setWordWrap(true);
    layout->addWidget(targetLabel);

    QFrame* line1 = new QFrame();
    line1->setFrameShape(QFrame::HLine);
    layout->addWidget(line1);

    layout->addWidget(new QLabel("<b>Scan Diagnostics:</b>"));
    layout->addWidget(new QLabel(QString("  Total Eligible Files Found: %1").arg(flattened.size())));
    layout->addWidget(new QLabel(QString("  Estimated Archive Size: %1 bytes").arg(scan.totalSizeBytes)));
    layout->addWidget(new QLabel(QString("  Skipped Symlinks: %1").arg(scan.skippedSymlinks)));

    QFrame* line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    layout->addWidget(line2);

    layout->addWidget(new QLabel("<b>Warnings & Anomalies:</b>"));

    if (!dedup.nameCollisions.empty()) {
        layout->addWidget(new QLabel(QString("  Name Collisions Detected: %1 duplicate filename group(s)")
            .arg(dedup.nameCollisions.size())));
        layout->addWidget(new QLabel("  (Will resolve using Path-Based Renaming schema)"));
    } else {
        layout->addWidget(new QLabel("  Name Collisions Detected: none"));
    }

    if (!dedup.contentDuplicates.empty()) {
        layout->addWidget(new QLabel(QString("  Content Duplicates: %1 identical payload group(s)")
            .arg(dedup.contentDuplicates.size())));
    } else {
        layout->addWidget(new QLabel("  Content Duplicates: none"));
    }

    QFrame* line3 = new QFrame();
    line3->setFrameShape(QFrame::HLine);
    layout->addWidget(line3);

    QHBoxLayout* buttonRow = new QHBoxLayout();
    QPushButton* cancelBtn = new QPushButton("Cancel");
    QPushButton* confirmBtn = new QPushButton("Confirm && Export");
    confirmBtn->setDefault(true);

    buttonRow->addWidget(cancelBtn);
    buttonRow->addStretch();
    buttonRow->addWidget(confirmBtn);
    layout->addLayout(buttonRow);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(confirmBtn, &QPushButton::clicked, this, &QDialog::accept);
}
