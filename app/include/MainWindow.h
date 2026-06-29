#pragma once
#include <QMainWindow>
#include "FileScanner.h"

class QLineEdit;
class QPushButton;
class QComboBox;
class QTreeWidget;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onBrowseRootFolder();
    void onBrowseSaveTarget();
    void onScanClicked();

private:
    void setupUi();
    void runScan(const QString& path);
    void populateTree();

    QLineEdit* m_rootFolderEdit = nullptr;
    QPushButton* m_browseRootBtn = nullptr;

    QLineEdit* m_saveTargetEdit = nullptr;
    QPushButton* m_browseSaveBtn = nullptr;

    QComboBox* m_presetCombo = nullptr;
    QLineEdit* m_filterRulesEdit = nullptr;
    QPushButton* m_scanBtn = nullptr;

    QTreeWidget* m_treeWidget = nullptr;
    QLabel* m_statusLabel = nullptr;

    ScanResult m_lastScanResult;
};
