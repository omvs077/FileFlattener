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
    resize(1100, 700);
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

    m_treeWidget = new QTreeWidget();
    m_treeWidget->setHeaderLabels({ "Name", "Size (bytes)" });
    m_treeWidget->setColumnWidth(0, 400);
    mainLayout->addWidget(m_treeWidget, 1);

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

    m_statusLabel->setText(QString("Scan complete. %1 files, %2 directories, %3 bytes total.")
        .arg(result.files.size())
        .arg(result.totalDirectories)
        .arg(result.totalSizeBytes));
}

// Normalize a filesystem-style relative path string ("a\\b" or "a/b") into
// a list of clean path segments, regardless of platform separator.
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

    // Key: normalized "/"-joined segment path -> the QTreeWidgetItem for that folder.
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
