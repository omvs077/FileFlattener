#pragma once
#include <QMainWindow>

class QLineEdit;
class QPushButton;
class QComboBox;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onBrowseRootFolder();
    void onBrowseSaveTarget();

private:
    void setupUi();

    QLineEdit* m_rootFolderEdit = nullptr;
    QPushButton* m_browseRootBtn = nullptr;

    QLineEdit* m_saveTargetEdit = nullptr;
    QPushButton* m_browseSaveBtn = nullptr;

    QComboBox* m_presetCombo = nullptr;
    QLineEdit* m_filterRulesEdit = nullptr;
};
