#include "MainWindow.h"
#include "Deduplicator.h"
#include "Renamer.h"
#include "ZipWriter.h"
#include "PreviewDialog.h"
#include "StructureExporter.h"
#include "FlattenWorker.h"
#include "GraphView.h"
#include <QTabWidget>

#include <QWidget>
#include <QFontMetrics>
#include <QSettings>
#include <QToolButton>
#include <QMenu>
#include <QStatusBar>
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
#include <QMap>
#include <QRegularExpression>
#include <QMessageBox>
#include <QFileIconProvider>
#include <QDir>
#include <QThread>
#include <QProgressDialog>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("FileFlattener (Desktop v1.0)");
    resize(1200, 700);
    setAcceptDrops(true);
    setWindowIcon(QIcon(QCoreApplication::applicationDirPath() + "/app_icon.ico"));
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
    m_recentBtn = new QToolButton();
    m_recentBtn->setText("Recent \xe2\x96\xbe");
    m_recentBtn->setPopupMode(QToolButton::InstantPopup);
    rootRow->addWidget(m_recentBtn);
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
    connect(m_presetCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onPresetChanged);
    m_scanBtn = new QPushButton("Scan");
    presetRow->addWidget(m_scanBtn);
    mainLayout->addLayout(presetRow);

    QHBoxLayout* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Filter Rules (exclude):"));
    m_filterRulesEdit = new QLineEdit(".git, node_modules, build, *.log");
    filterRow->addWidget(m_filterRulesEdit);
    mainLayout->addLayout(filterRow);

    QHBoxLayout* treeToolbarRow = new QHBoxLayout();
    m_expandAllBtn = new QPushButton("Expand All");
    m_collapseAllBtn = new QPushButton("Collapse All");
    treeToolbarRow->addWidget(m_expandAllBtn);
    treeToolbarRow->addWidget(m_collapseAllBtn);

    m_treeSearchEdit = new QLineEdit();
    m_treeSearchEdit->setPlaceholderText("Search files...");
    m_treeSearchEdit->setMaximumWidth(220);
    treeToolbarRow->addWidget(m_treeSearchEdit);

    m_treeSortCombo = new QComboBox();
    m_treeSortCombo->addItem("Name (A-Z)");
    m_treeSortCombo->addItem("Name (Z-A)");
    m_treeSortCombo->addItem("Size (Largest first)");
    m_treeSortCombo->addItem("Size (Smallest first)");
    treeToolbarRow->addWidget(m_treeSortCombo);

    treeToolbarRow->addStretch();
    mainLayout->addLayout(treeToolbarRow);

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

    QTabWidget* tabs = new QTabWidget();
    tabs->addTab(splitter, "Files");
    m_graphView = new GraphView();
    tabs->addTab(m_graphView, "Folder Graph");
    mainLayout->addWidget(tabs, 1);

    m_statusLabel = new QLabel("Ready. Select a folder and click Scan.");
    mainLayout->addWidget(m_statusLabel);

    QHBoxLayout* actionRow = new QHBoxLayout();
    actionRow->addStretch();
    m_flattenZipBtn = new QPushButton("FLATTEN && ZIP");
    m_flattenZipBtn->setMinimumHeight(36);
    m_flattenZipBtn->setMinimumWidth(180);
    m_flattenZipBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #2e7d32;"
        "  color: white;"
        "  font-weight: bold;"
        "  border-radius: 4px;"
        "  border: none;"
        "}"
        "QPushButton:hover { background-color: #388e3c; }"
        "QPushButton:pressed { background-color: #1b5e20; }"
        "QPushButton:disabled { background-color: #9e9e9e; }"
    );
    QPushButton* m_exportStructureBtn = new QPushButton("Export Folder Structure");
    m_exportStructureBtn->setMinimumHeight(36);
    m_exportStructureBtn->setMinimumWidth(180);
    m_exportStructureBtn->setStyleSheet(
        "QPushButton { background-color: #1565c0; color: white; font-weight: bold; border-radius: 4px; border: none; }"
        "QPushButton:hover { background-color: #1976d2; }"
        "QPushButton:pressed { background-color: #0d47a1; }"
        "QPushButton:disabled { background-color: #9e9e9e; }"
    );
    actionRow->addWidget(m_exportStructureBtn);
    connect(m_exportStructureBtn, &QPushButton::clicked, this, &MainWindow::onExportStructureClicked);
    actionRow->addWidget(m_flattenZipBtn);
    mainLayout->addLayout(actionRow);

    setCentralWidget(central);

    rebuildRecentMenu();

    QLabel* creditLabel = new QLabel(
        "Crafted by <b>Dvvyom</b> &nbsp;|&nbsp; "
        "<a href=\"https://github.com/omvs077\" style=\"color:#2f6fed; font-weight:bold; text-decoration:underline;\">GitHub</a> &nbsp;|&nbsp; "
        "<a href=\"mailto:omvs077@gmail.com\" style=\"color:#2f6fed; font-weight:bold; text-decoration:underline;\">omvs077@gmail.com</a> "
        "&nbsp;&mdash;&nbsp; suggestions/feedback welcome"
    );
    creditLabel->setOpenExternalLinks(true);
    creditLabel->setStyleSheet("color: #444; font-size: 11px;");
    statusBar()->setStyleSheet("border-top: 1px solid #ccc; padding: 2px 4px;");
    statusBar()->addPermanentWidget(creditLabel);
    statusBar()->setSizeGripEnabled(false);
    connect(m_browseRootBtn, &QPushButton::clicked, this, &MainWindow::onBrowseRootFolder);
    connect(m_browseSaveBtn, &QPushButton::clicked, this, &MainWindow::onBrowseSaveTarget);
    connect(m_scanBtn, &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(m_flattenZipBtn, &QPushButton::clicked, this, &MainWindow::onFlattenZipClicked);
    connect(m_expandAllBtn, &QPushButton::clicked, this, &MainWindow::onExpandAllClicked);
    connect(m_collapseAllBtn, &QPushButton::clicked, this, &MainWindow::onCollapseAllClicked);
    connect(m_treeSearchEdit, &QLineEdit::textChanged, this, &MainWindow::onTreeSearchTextChanged);
    connect(m_treeSortCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onTreeSortChanged);
}

void MainWindow::onBrowseRootFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Root Folder");
    if (!dir.isEmpty()) {
        m_rootFolderEdit->setText(dir);

        QString folderName = QFileInfo(dir).fileName();
        if (folderName.isEmpty()) folderName = "export";
        QString suggestedZip = QDir::cleanPath(QDir(dir).filePath("../" + folderName + "_export.zip"));
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

void MainWindow::onExpandAllClicked() {
    m_treeWidget->expandAll();
    if (m_graphView) m_graphView->expandAll();
}

void MainWindow::onCollapseAllClicked() {
    m_treeWidget->collapseAll();
    if (m_graphView) m_graphView->collapseAll();
}

static bool filterTreeItemRecursive(QTreeWidgetItem* item, const QString& searchText) {
    bool anyChildVisible = false;
    for (int i = 0; i < item->childCount(); ++i) {
        if (filterTreeItemRecursive(item->child(i), searchText)) {
            anyChildVisible = true;
        }
    }

    bool selfMatches = searchText.isEmpty() ||
        item->text(0).contains(searchText, Qt::CaseInsensitive);

    bool isLeaf = (item->childCount() == 0);
    bool visible = anyChildVisible || (isLeaf && selfMatches) || (!isLeaf && selfMatches);

    item->setHidden(!visible);
    return visible;
}

void MainWindow::onTreeSearchTextChanged(const QString& text) {
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        filterTreeItemRecursive(m_treeWidget->topLevelItem(i), text);
    }
    if (!text.isEmpty()) {
        m_treeWidget->expandAll();
    }
}

void MainWindow::onTreeSortChanged(int index) {
    Q_UNUSED(index);
    populateTree();
    onTreeSearchTextChanged(m_treeSearchEdit->text());
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

    PresetType preset = PresetType::StandardBackup;
    switch (m_presetCombo->currentIndex()) {
        case 0: preset = PresetType::StandardBackup; break;
        case 1: preset = PresetType::AiReadySourceOnly; break;
        case 2: preset = PresetType::MediaOnly; break;
        case 3: preset = PresetType::DocumentsOnly; break;
    }

    FilterEngine filter(m_filterRulesEdit->text().toStdString(), preset);

    ScanResult result;
    if (!scanner.scan(root, result, err, &filter)) {
        QString qerr = QString::fromStdString(err);
        if (qerr.contains("exceeds limit", Qt::CaseInsensitive)) {
            QMessageBox box(this);
            box.setWindowTitle("Folder Too Large");
            box.setIcon(QMessageBox::Warning);
            if (!m_smartFilterAttempted) {
                box.setText("This folder exceeds the 20GB scan limit.");
                box.setInformativeText(
                    "Common heavy folders like node_modules, build, .venv, and target "
                    "are often safe to exclude. Add them to Filter Rules and re-scan?"
                );
                QPushButton* addBtn = box.addButton("Add Excludes && Re-scan", QMessageBox::AcceptRole);
                box.addButton("Cancel", QMessageBox::RejectRole);
                box.exec();
                if (box.clickedButton() == addBtn) {
                    m_smartFilterAttempted = true;
                    QString current = m_filterRulesEdit->text().trimmed();
                    QStringList toAdd = { "node_modules", "build", ".venv", "target" };
                    QStringList existing = current.split(",", Qt::SkipEmptyParts);
                    for (QString& s : existing) s = s.trimmed();
                    for (const QString& rule : toAdd) {
                        if (!existing.contains(rule, Qt::CaseInsensitive)) existing << rule;
                    }
                    m_filterRulesEdit->setText(existing.join(", "));
                    onScanClicked();
                    return;
                }
            } else {
                box.setText("This folder still exceeds the 20GB scan limit.");
                box.setInformativeText(
                    "The common excludes (node_modules, build, .venv, target) did not reduce it enough. "
                    "Try manually adding more excludes in Filter Rules (e.g. large media subfolders) and scan again."
                );
                box.addButton("OK", QMessageBox::AcceptRole);
                box.exec();
                m_smartFilterAttempted = false;
            }
        }
        m_statusLabel->setText(QString("Scan failed: %1").arg(qerr));
        return;
    }

    m_smartFilterAttempted = false;
    m_lastScanResult = result;
    if (m_graphView) m_graphView->setGraphModel(GraphBuilder::build(result));
    saveRecentProject(QString::fromStdString(root.string()));
    populateTree();
    populateAnalyticsTable();

    m_statusLabel->setText(QString("Scan complete. %1 files, %2 directories, %3 bytes total. (%4 excluded by filters)")
        .arg(result.files.size())
        .arg(result.totalDirectories)
        .arg(result.totalSizeBytes)
        .arg(result.filteredOutCount));
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

    QString projectName = QFileInfo(m_rootFolderEdit->text()).fileName();
    if (projectName.isEmpty()) projectName = "Export";

    // Lightweight pre-check: filename collisions only (no hashing yet --
    // content duplicate detection happens inside the worker thread).
    DedupResult quickDedupPreview;
    for (size_t i = 0; i < m_lastScanResult.files.size(); ++i) {
        std::string fname = m_lastScanResult.files[i].relativePath.filename().string();
        quickDedupPreview.nameCollisions[fname].push_back(i);
    }
    for (auto it = quickDedupPreview.nameCollisions.begin(); it != quickDedupPreview.nameCollisions.end(); ) {
        if (it->second.size() < 2) it = quickDedupPreview.nameCollisions.erase(it);
        else ++it;
    }

    Renamer previewRenamer;
    auto previewFlattened = previewRenamer.resolve(m_lastScanResult, quickDedupPreview);

    PreviewDialog previewDialog(m_lastScanResult, quickDedupPreview, previewFlattened, targetZip, this);
    if (previewDialog.exec() != QDialog::Accepted) {
        m_statusLabel->setText("Export cancelled.");
        return;
    }

    m_flattenZipBtn->setEnabled(false);
    m_scanBtn->setEnabled(false);

    m_progressDialog = new QProgressDialog("Preparing...", "Cancel", 0, 100, this);
    m_progressDialog->setWindowTitle("Exporting");
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setMinimumDuration(0);
    m_progressDialog->setMinimumWidth(500);
    m_progressDialog->setFixedSize(m_progressDialog->sizeHint().expandedTo(QSize(500, 0)));
    m_progressDialog->setValue(0);

    m_workerThread = new QThread(this);
    m_worker = new FlattenWorker(m_lastScanResult, targetZip, projectName);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &FlattenWorker::run);
    connect(m_worker, &FlattenWorker::currentFile, this, [this](const QString& fname) {
        if (m_progressDialog) {
            QFontMetrics fm(m_progressDialog->font());
            QString elided = fm.elidedText("Zipping: " + fname, Qt::ElideMiddle, 460);
            m_progressDialog->setLabelText(elided);
        }
    });

    connect(m_worker, &FlattenWorker::stageChanged, this, &MainWindow::onWorkerStageChanged);
    connect(m_worker, &FlattenWorker::progressChanged, this, &MainWindow::onWorkerProgressChanged);
    connect(m_worker, &FlattenWorker::finishedSuccess, this, &MainWindow::onWorkerFinishedSuccess);
    connect(m_worker, &FlattenWorker::finishedError, this, &MainWindow::onWorkerFinishedError);

    connect(m_worker, &FlattenWorker::finishedSuccess, m_workerThread, &QThread::quit);
    connect(m_worker, &FlattenWorker::finishedError, m_workerThread, &QThread::quit);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);

    m_workerThread->start();
    m_progressDialog->show();
}

void MainWindow::onPresetChanged(int index) {
    QString defaults;
    switch (index) {
        case 0: // Standard Backup
            defaults = ".git, node_modules, build, *.log";
            break;
        case 1: // AI-Ready Source Only
            defaults = ".git, node_modules, build, target, .venv, dist, *.exe, *.dll, *.bin, *.png, *.jpg, *.jpeg, *.mp4, *.zip";
            break;
        case 2: // Media Only
            defaults = ".git, node_modules, build, *.txt, *.md, *.json, *.py, *.cpp, *.h, *.cs, *.js, *.ts";
            break;
        case 3: // Documents Only
            defaults = ".git, node_modules, build, *.exe, *.dll, *.bin, *.mp4, *.mp3, *.png, *.jpg, *.jpeg, *.zip";
            break;
        default:
            return;
    }
    if (m_filterRulesEdit) m_filterRulesEdit->setText(defaults);
}

void MainWindow::onWorkerStageChanged(QString stage) {
    if (m_progressDialog) m_progressDialog->setLabelText(stage);
}

void MainWindow::onWorkerProgressChanged(int percent) {
    if (m_progressDialog) m_progressDialog->setValue(percent);
}

void MainWindow::onWorkerFinishedSuccess(QString zipPath) {
    if (m_progressDialog) {
        m_progressDialog->close();
        m_progressDialog->deleteLater();
        m_progressDialog = nullptr;
    }
    m_flattenZipBtn->setEnabled(true);
    m_scanBtn->setEnabled(true);

    m_statusLabel->setText(QString("Export complete: %1").arg(zipPath));
    QMessageBox::information(this, "Export Complete", QString("ZIP created successfully at:\n%1").arg(zipPath));
}

void MainWindow::onWorkerFinishedError(QString errorMessage) {
    if (m_progressDialog) {
        m_progressDialog->close();
        m_progressDialog->deleteLater();
        m_progressDialog = nullptr;
    }
    m_flattenZipBtn->setEnabled(true);
    m_scanBtn->setEnabled(true);

    m_statusLabel->setText("Export failed.");
    QMessageBox::critical(this, "Export Failed", errorMessage);
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
    static QFileIconProvider iconProvider;

    std::vector<ScannedFile> sortedFiles = m_lastScanResult.files;
    int sortMode = m_treeSortCombo ? m_treeSortCombo->currentIndex() : 0;
    switch (sortMode) {
        case 0: // Name A-Z
            std::sort(sortedFiles.begin(), sortedFiles.end(), [](const ScannedFile& a, const ScannedFile& b) {
                return a.relativePath.filename().string() < b.relativePath.filename().string();
            });
            break;
        case 1: // Name Z-A
            std::sort(sortedFiles.begin(), sortedFiles.end(), [](const ScannedFile& a, const ScannedFile& b) {
                return a.relativePath.filename().string() > b.relativePath.filename().string();
            });
            break;
        case 2: // Size largest first
            std::sort(sortedFiles.begin(), sortedFiles.end(), [](const ScannedFile& a, const ScannedFile& b) {
                return a.sizeBytes > b.sizeBytes;
            });
            break;
        case 3: // Size smallest first
            std::sort(sortedFiles.begin(), sortedFiles.end(), [](const ScannedFile& a, const ScannedFile& b) {
                return a.sizeBytes < b.sizeBytes;
            });
            break;
    }

    auto getOrCreateFolder = [&](const QStringList& segments) -> QTreeWidgetItem* {
        QTreeWidgetItem* current = nullptr;
        QString accumulated;

        for (const QString& seg : segments) {
            accumulated = accumulated.isEmpty() ? seg : accumulated + "/" + seg;

            if (folderItems.contains(accumulated)) {
                current = folderItems[accumulated];
                continue;
            }

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

    for (const auto& file : sortedFiles) {
        QString relPathStr = QString::fromStdString(file.relativePath.string());
        QStringList allSegments = splitPathSegments(relPathStr);

        QString fileName = allSegments.isEmpty() ? relPathStr : allSegments.last();
        QStringList folderSegments = allSegments;
        if (!folderSegments.isEmpty()) folderSegments.removeLast();

        QTreeWidgetItem* parentItem = nullptr;
        if (!folderSegments.isEmpty()) {
            parentItem = getOrCreateFolder(folderSegments);
        }

        QTreeWidgetItem* fileItem = new QTreeWidgetItem();
        fileItem->setText(0, fileName);
        fileItem->setText(1, QString::number(file.sizeBytes));

        QIcon fileIcon = iconProvider.icon(QFileInfo(fileName));
        if (fileIcon.isNull()) {
            fileIcon = iconProvider.icon(QFileIconProvider::File);
        }
        fileItem->setIcon(0, fileIcon);

        if (parentItem) {
            parentItem->addChild(fileItem);
        } else {
            m_treeWidget->addTopLevelItem(fileItem);
        }
    }

    m_treeWidget->expandAll();
    if (m_graphView) m_graphView->expandAll();
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
        QString sampleName = (ext == "(no extension)") ? QStringLiteral("file") : ("x" + ext);
        QIcon icon = tableIconProvider.icon(QFileInfo(sampleName));
        if (icon.isNull()) {
            icon = tableIconProvider.icon(QFileIconProvider::File);
        }
        extItem->setIcon(icon);

        m_analyticsTable->setItem(row, 0, extItem);
        m_analyticsTable->setItem(row, 1, new QTableWidgetItem(QString::number(s.count)));
        m_analyticsTable->setItem(row, 2, new QTableWidgetItem(QString::number(s.totalSize)));
        row++;
    }
}



































QStringList MainWindow::loadRecentProjects() {
    QSettings settings("FileFlattener", "FileFlattenerApp");
    return settings.value("recentProjects").toStringList();
}

void MainWindow::saveRecentProject(const QString& path) {
    if (path.trimmed().isEmpty()) return;
    QSettings settings("FileFlattener", "FileFlattenerApp");
    QStringList recents = settings.value("recentProjects").toStringList();
    recents.removeAll(path);
    recents.prepend(path);
    while (recents.size() > 5) recents.removeLast();
    settings.setValue("recentProjects", recents);
    rebuildRecentMenu();
}

void MainWindow::rebuildRecentMenu() {
    if (!m_recentBtn) return;
    QStringList recents = loadRecentProjects();
    QMenu* menu = new QMenu(m_recentBtn);
    if (recents.isEmpty()) {
        QAction* none = menu->addAction("No recent projects");
        none->setEnabled(false);
    } else {
        for (const QString& path : recents) {
            QAction* action = menu->addAction(path);
            connect(action, &QAction::triggered, this, [this, path]() {
                m_rootFolderEdit->setText(path);
            });
        }
    }
    m_recentBtn->setMenu(menu);
}








void MainWindow::onExportStructureClicked() {
    if (m_lastScanResult.files.empty()) {
        QMessageBox::warning(this, "No Scan Data", "Please scan a folder first.");
        return;
    }
    QString savePath = QFileDialog::getSaveFileName(
        this, "Export Folder Structure", "folder_structure.txt",
        "Text Files (*.txt);;All Files (*)");
    if (savePath.isEmpty()) return;

    std::string projectName = m_rootFolderEdit->text().toStdString();
    std::string text = StructureExporter::buildStructureText(m_lastScanResult, projectName);

    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Could not write file: " + savePath);
        return;
    }
    file.write(QByteArray::fromStdString(text));
    file.close();
    m_statusLabel->setText("Folder structure exported to: " + savePath);
}
