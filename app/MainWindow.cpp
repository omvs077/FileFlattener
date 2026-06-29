#include "MainWindow.h"
#include "Deduplicator.h"
#include "Renamer.h"
#include "ZipWriter.h"
#include "PreviewDialog.h"
#include "StructureExporter.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QFileDialog>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QHeaderView>
#include <QSplitter>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QMap>
#include <QRegularExpression>
#include <QFileIconProvider>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("FileFlattener (Desktop v1.0)");
    resize(1200, 700);
    setAcceptDrops(true);
    setupUi();
}

void MainWindow::setupUi() {
    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    QHBoxLayout* rootRow = new QHBoxLayout();
    rootRow->addWidget(new QLabel("Root Folder:"));
    m_rootFolderEdit = new QLineEdit();
    m_rootFolderEdit->setPlaceholderText("Drag a folder here or click Browse...");
    rootRow->addWidget(m_rootFolderEdit);
    m_browseRootBtn = new QPushButton("Browse");
    rootRow->addWidget(m_browseRootBtn);
    mainLayout->addLayout(rootRow);

    QHBoxLayout* saveRow = new QHBoxLayout();
    saveRow->addWidget(new QLabel("Save Target:"));
    m_saveTargetEdit = new QLineEdit();
    m_saveTargetEdit->setPlaceholderText("Output .zip path...");
    saveRow->addWidget(m_saveTargetEdit);
    m_browseSaveBtn = new QPushButton("Browse");
    saveRow->addWidget(m_browseSaveBtn);
    mainLayout->addLayout(saveRow);

    QHBoxLayout* presetRow = new QHBoxLayout();
    presetRow->addWidget(new QLabel("Preset Type:"));
    m_presetCombo = new QComboBox();
    m_presetCombo->addItem("Standard Backup");
    m_presetCombo->addItem("AI-Ready Source Only");
    m_presetCombo->addItem("Media Only");
    m_presetCombo->addItem("Documents Only");
    presetRow->addWidget(m_presetCombo);
    m_scanBtn = new QPushButton("Scan");
    presetRow->addWidget(m_scanBtn);
    mainLayout->addLayout(presetRow);

    QHBoxLayout* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Filter Rules (exclude):"));
    m_filterRulesEdit = new QLineEdit(".git, node_modules, build, *.log");
    filterRow->addWidget(m_filterRulesEdit);
    mainLayout->addLayout(filterRow);

    QSplitter* splitter = new QSplitter(Qt::Horizontal);

    m_treeWidget = new QTreeWidget();
    m_treeWidget->setHeaderLabels({ "Name", "Size (bytes)" });
    m_treeWidget->setColumnWidth(0, 350);
    splitter->addWidget(m_treeWidget);

    m_analyticsTable = new QTableWidget();
    m_analyticsTable->setColumnCount(3);
    m_analyticsTable->setHorizontalHeaderLabels({ "Extension", "Count", "Total Size (bytes)" });
    m_analyticsTable->horizontalHeader()->setStretchLastSection(true);
    m_analyticsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    splitter->addWidget(m_analyticsTable);

    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter, 1);

    m_statusLabel = new QLabel("Ready. Select a folder and click Scan.");
    mainLayout->addWidget(m_statusLabel);

    QHBoxLayout* actionRow = new QHBoxLayout();
    actionRow->addStretch();
    m_flattenZipBtn = new QPushButton("FLATTEN && ZIP");
    m_flattenZipBtn->setMinimumHeight(36);
    actionRow->addWidget(m_flattenZipBtn);
    mainLayout->addLayout(actionRow);

    setCentralWidget(central);

    connect(m_browseRootBtn, &QPushButton::clicked, this, &MainWindow::onBrowseRootFolder);
    connect(m_browseSaveBtn, &QPushButton::clicked, this, &MainWindow::onBrowseSaveTarget);
    connect(m_scanBtn, &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(m_flattenZipBtn, &QPushButton::clicked, this, &MainWindow::onFlattenZipClicked);
}

void MainWindow::onBrowseRootFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Root Folder");
    if (!dir.isEmpty()) {
        m_rootFolderEdit->setText(dir);

        QString folderName = QFileInfo(dir).fileName();
        if (folderName.isEmpty()) folderName = "export";
        QString suggestedZip = QDir(dir).filePath("../" + folderName + "_export.zip");
        suggestedZip = QDir::cleanPath(suggestedZip);
        m_saveTargetEdit->setText(suggestedZip);
    }
}

void MainWindow::onBrowseSaveTarget() {
    QString file = QFileDialog::getSaveFileName(this, "Select Output ZIP", "", "ZIP Archives (*.zip)");
    if (!file.isEmpty()) {
        m_saveTargetEdit->setText(file);
    }
}

void MainWindow::onScanClicked() {
    runScan(m_rootFolderEdit->text());
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    const QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        QString path = urls.first().toLocalFile();
        if (QFileInfo(path).isDir()) {
            m_rootFolderEdit->setText(path);

            QString folderName = QFileInfo(path).fileName();
            if (folderName.isEmpty()) folderName = "export";
            QString suggestedZip = QDir::cleanPath(QDir(path).filePath("../" + folderName + "_export.zip"));
            m_saveTargetEdit->setText(suggestedZip);

            runScan(path);
        }
    }
}

void MainWindow::runScan(const QString& path) {
    if (path.isEmpty()) {
        m_statusLabel->setText("Please select a folder first.");
        return;
    }

    FileScanner scanner;
    std::string err;
    std::filesystem::path root = path.toStdString();

    if (!scanner.validateRoot(root, err)) {
        m_statusLabel->setText(QString("Validation failed: %1").arg(QString::fromStdString(err)));
        return;
    }

    ScanResult result;
    if (!scanner.scan(root, result, err)) {
        m_statusLabel->setText(QString("Scan failed: %1").arg(QString::fromStdString(err)));
        return;
    }

    m_lastScanResult = result;
    populateTree();
    populateAnalyticsTable();

    m_statusLabel->setText(QString("Scan complete. %1 files, %2 directories, %3 bytes total.")
        .arg(result.files.size())
        .arg(result.totalDirectories)
        .arg(result.totalSizeBytes));
}

void MainWindow::onFlattenZipClicked() {
    if (m_lastScanResult.files.empty()) {
        QMessageBox::warning(this, "No Scan Data", "Please scan a folder first before exporting.");
        return;
    }

    QString targetZip = m_saveTargetEdit->text();
    if (targetZip.isEmpty()) {
        QMessageBox::warning(this, "No Save Target", "Please choose a save target ZIP path first.");
        return;
    }

    // --- Targeted Pass 2: run dedup (size-group + hash) only now ---
    Deduplicator dedup;
    DedupResult dedupResult;
    std::string err;
    if (!dedup.process(m_lastScanResult, dedupResult, err)) {
        QMessageBox::critical(this, "Deduplication Failed", QString::fromStdString(err));
        return;
    }

    Renamer renamer;
    auto flattened = renamer.resolve(m_lastScanResult, dedupResult);

    // --- Show the mandatory Pre-Export Preview Modal ---
    PreviewDialog dialog(m_lastScanResult, dedupResult, flattened, targetZip, this);
    if (dialog.exec() != QDialog::Accepted) {
        m_statusLabel->setText("Export cancelled.");
        return;
    }

    // --- Confirmed: stream to ZIP ---
    QString projectName = QFileInfo(m_rootFolderEdit->text()).fileName();
    if (projectName.isEmpty()) projectName = "Export";

    std::string structureText = StructureExporter::buildStructureText(m_lastScanResult, projectName.toStdString());
    std::string manifestJson = StructureExporter::buildManifestJson(m_lastScanResult, flattened, projectName.toStdString());

    ZipWriter writer;
    if (!writer.writeZip(m_lastScanResult, flattened, targetZip.toStdString(), err, structureText, manifestJson)) {
        QMessageBox::critical(this, "Export Failed", QString::fromStdString(err));
        return;
    }

    m_statusLabel->setText(QString("Export complete: %1").arg(targetZip));
    QMessageBox::information(this, "Export Complete", QString("ZIP created successfully at:\n%1").arg(targetZip));
}

static QStringList splitPathSegments(const QString& path) {
    QStringList parts = path.split(QRegularExpression("[\\\\/]"));
    QStringList clean;
    for (const QString& p : parts) {
        if (!p.isEmpty()) clean << p;
    }
    return clean;
}

void MainWindow::populateTree() {
    m_treeWidget->clear();

    QMap<QString, QTreeWidgetItem*> folderItems;

    auto getOrCreateFolder = [&](const QStringList& segments) -> QTreeWidgetItem* {
        QTreeWidgetItem* current = nullptr;
        QString accumulated;

        for (const QString& seg : segments) {
            accumulated = accumulated.isEmpty() ? seg : accumulated + "/" + seg;

            if (folderItems.contains(accumulated)) {
                current = folderItems[accumulated];
                continue;
            }

            static QFileIconProvider iconProvider;
            QTreeWidgetItem* folderItem = new QTreeWidgetItem();
            folderItem->setText(0, seg);
            folderItem->setIcon(0, iconProvider.icon(QFileIconProvider::Folder));

            if (current) {
                current->addChild(folderItem);
            } else {
                m_treeWidget->addTopLevelItem(folderItem);
            }

            folderItems[accumulated] = folderItem;
            current = folderItem;
        }

        return current;
    };

    for (const auto& file : m_lastScanResult.files) {
        QString relPathStr = QString::fromStdString(file.relativePath.string());
        QStringList allSegments = splitPathSegments(relPathStr);

        QString fileName = allSegments.isEmpty() ? relPathStr : allSegments.last();
        QStringList folderSegments = allSegments;
        if (!folderSegments.isEmpty()) folderSegments.removeLast();

        QTreeWidgetItem* parentItem = nullptr;
        if (!folderSegments.isEmpty()) {
            parentItem = getOrCreateFolder(folderSegments);
        }

        static QFileIconProvider fileIconProvider;
        QTreeWidgetItem* fileItem = new QTreeWidgetItem();
        fileItem->setText(0, fileName);
        fileItem->setText(1, QString::number(file.sizeBytes));
        fileItem->setIcon(0, fileIconProvider.icon(QFileIconProvider::File));

        if (parentItem) {
            parentItem->addChild(fileItem);
        } else {
            m_treeWidget->addTopLevelItem(fileItem);
        }
    }

    m_treeWidget->expandAll();
}

void MainWindow::populateAnalyticsTable() {
    struct ExtStats {
        int count = 0;
        uint64_t totalSize = 0;
    };

    QMap<QString, ExtStats> statsByExt;

    for (const auto& file : m_lastScanResult.files) {
        QString ext = QString::fromStdString(file.relativePath.extension().string());
        if (ext.isEmpty()) ext = "(no extension)";
        ext = ext.toLower();

        ExtStats& s = statsByExt[ext];
        s.count++;
        s.totalSize += file.sizeBytes;
    }

    QList<QString> exts = statsByExt.keys();
    std::sort(exts.begin(), exts.end(), [&](const QString& a, const QString& b) {
        return statsByExt[a].totalSize > statsByExt[b].totalSize;
    });

    static QFileIconProvider tableIconProvider;

    m_analyticsTable->setRowCount(exts.size());
    int row = 0;
    for (const QString& ext : exts) {
        const ExtStats& s = statsByExt[ext];

        QTableWidgetItem* extItem = new QTableWidgetItem(ext);

        // Use a representative dummy filename with this extension to get a
        // realistic shell icon (e.g. "x.txt", "x.png").
        QString sampleName = (ext == "(no extension)") ? QStringLiteral("file") : ("x" + ext);
        QFileIconProvider::IconType fallbackType = QFileIconProvider::File;
        QIcon icon = tableIconProvider.icon(QFileInfo(sampleName));
        if (icon.isNull()) {
            icon = tableIconProvider.icon(fallbackType);
        }
        extItem->setIcon(icon);

        m_analyticsTable->setItem(row, 0, extItem);
        m_analyticsTable->setItem(row, 1, new QTableWidgetItem(QString::number(s.count)));
        m_analyticsTable->setItem(row, 2, new QTableWidgetItem(QString::number(s.totalSize)));
        row++;
    }
}









