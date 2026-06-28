#include "MainWindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QFileDialog>
#include <QRadioButton>
#include <QButtonGroup>

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

    // --- Root Folder row ---
    QHBoxLayout* rootRow = new QHBoxLayout();
    rootRow->addWidget(new QLabel("Root Folder:"));
    m_rootFolderEdit = new QLineEdit();
    m_rootFolderEdit->setPlaceholderText("Drag a folder here or click Browse...");
    rootRow->addWidget(m_rootFolderEdit);
    m_browseRootBtn = new QPushButton("Browse");
    rootRow->addWidget(m_browseRootBtn);
    mainLayout->addLayout(rootRow);

    // --- Save Target row ---
    QHBoxLayout* saveRow = new QHBoxLayout();
    saveRow->addWidget(new QLabel("Save Target:"));
    m_saveTargetEdit = new QLineEdit();
    m_saveTargetEdit->setPlaceholderText("Output .zip path...");
    saveRow->addWidget(m_saveTargetEdit);
    m_browseSaveBtn = new QPushButton("Browse");
    saveRow->addWidget(m_browseSaveBtn);
    mainLayout->addLayout(saveRow);

    // --- Preset + Filter row ---
    QHBoxLayout* presetRow = new QHBoxLayout();
    presetRow->addWidget(new QLabel("Preset Type:"));
    m_presetCombo = new QComboBox();
    m_presetCombo->addItem("Standard Backup");
    m_presetCombo->addItem("AI-Ready Source Only");
    m_presetCombo->addItem("Media Only");
    m_presetCombo->addItem("Documents Only");
    presetRow->addWidget(m_presetCombo);
    mainLayout->addLayout(presetRow);

    QHBoxLayout* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Filter Rules (exclude):"));
    m_filterRulesEdit = new QLineEdit(".git, node_modules, build, *.log");
    filterRow->addWidget(m_filterRulesEdit);
    mainLayout->addLayout(filterRow);

    mainLayout->addStretch();

    setCentralWidget(central);

    connect(m_browseRootBtn, &QPushButton::clicked, this, &MainWindow::onBrowseRootFolder);
    connect(m_browseSaveBtn, &QPushButton::clicked, this, &MainWindow::onBrowseSaveTarget);
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
