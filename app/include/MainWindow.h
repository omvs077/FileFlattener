#pragma once
#include <QMainWindow>
class QToolButton;
#include "FileScanner.h"

class QLineEdit;
class QPushButton;
class QComboBox;
class QTreeWidget;
class QTableWidget;
class QLabel;
class QThread;
class QProgressDialog;
class FlattenWorker;
class GraphView;

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
    void onExpandAllClicked();
    void onCollapseAllClicked();
    void onTreeSearchTextChanged(const QString& text);
    void onTreeSortChanged(int index);
    void onFlattenZipClicked();

    void onWorkerStageChanged(QString stage);
    void onWorkerProgressChanged(int percent);
    void onWorkerFinishedSuccess(QString zipPath);
    void onWorkerFinishedError(QString errorMessage);
    void onPresetChanged(int index);

private:
    void setupUi();
    void runScan(const QString& path);
    void populateTree();
    void populateAnalyticsTable();

    QLineEdit* m_rootFolderEdit = nullptr;
    QPushButton* m_browseRootBtn = nullptr;

    QLineEdit* m_saveTargetEdit = nullptr;
    QPushButton* m_browseSaveBtn = nullptr;

    QComboBox* m_presetCombo = nullptr;
    QLineEdit* m_filterRulesEdit = nullptr;
    bool m_smartFilterAttempted = false;
    QToolButton* m_recentBtn = nullptr;
    QStringList loadRecentProjects();
    void saveRecentProject(const QString& path);
    void rebuildRecentMenu();
    QPushButton* m_scanBtn = nullptr;
    QPushButton* m_expandAllBtn = nullptr;
    QPushButton* m_collapseAllBtn = nullptr;
    QLineEdit* m_treeSearchEdit = nullptr;
    QComboBox* m_treeSortCombo = nullptr;
    QPushButton* m_flattenZipBtn = nullptr;

    QTreeWidget* m_treeWidget = nullptr;
    QTableWidget* m_analyticsTable = nullptr;
    QLabel* m_statusLabel = nullptr;

    ScanResult m_lastScanResult;

    QThread* m_workerThread = nullptr;
    FlattenWorker* m_worker = nullptr;
    QProgressDialog* m_progressDialog = nullptr;
    GraphView* m_graphView = nullptr;
};










