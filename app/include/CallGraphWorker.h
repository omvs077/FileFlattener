#pragma once
#include <QObject>
#include <QMetaType>
#include <filesystem>
#include <vector>
#include "CallGraphAnalyzer.h"

Q_DECLARE_METATYPE(CallGraphResult)

// Runs both CodeLexer::analyze() and CallGraphAnalyzer::analyze() on a
// background thread, so the GUI never blocks even on large C++ projects.
class CallGraphWorker : public QObject {
    Q_OBJECT
public:
    CallGraphWorker(std::filesystem::path root,
                     std::vector<std::filesystem::path> sourceFiles,
                     QObject* parent = nullptr);

public slots:
    void run();

signals:
    void finished(CallGraphResult result);

private:
    std::filesystem::path m_root;
    std::vector<std::filesystem::path> m_sourceFiles;
};
