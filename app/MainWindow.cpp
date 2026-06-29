#include "MainWindow.h"

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
#include <QMap>
#include <QRegularExpression>

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

    // --- Split view: tree (left) + analytics table (right) ---
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

    setCentralWidget(central);

    connect(m_browseRootBtn, &QPushButton::clicked, this, &MainWindow::onBrowseRootFolder);
    connect(m_browseSaveBtn, &QPushButton::clicked, this, &MainWindow::onBrowseSaveTarget);
    connect(m_scanBtn, &QPushButton::clicked, this, &MainWindow::onScanClicked);
}

void MainWindow::onBrowseRootFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Root Folder");
    if (!dir.isEmpty()) {
        m_rootFolderEdit->setText(dir);
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

            QTreeWidgetItem* folderItem = new QTreeWidgetItem();
            folderItem->setText(0, seg);

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

        QTreeWidgetItem* fileItem = new QTreeWidgetItem();
        fileItem->setText(0, fileName);
        fileItem->setText(1, QString::number(file.sizeBytes));

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

    m_analyticsTable->setRowCount(exts.size());
    int row = 0;
    for (const QString& ext : exts) {
        const ExtStats& s = statsByExt[ext];
        m_analyticsTable->setItem(row, 0, new QTableWidgetItem(ext));
        m_analyticsTable->setItem(row, 1, new QTableWidgetItem(QString::number(s.count)));
        m_analyticsTable->setItem(row, 2, new QTableWidgetItem(QString::number(s.totalSize)));
        row++;
    }
}
