#pragma once
#include <QObject>
#include <QMetaType>
#include <filesystem>
#include <vector>
#include "CodeLexer.h"

Q_DECLARE_METATYPE(CodeGraph)

class CodeAnalysisWorker : public QObject {
    Q_OBJECT
public:
    CodeAnalysisWorker(std::filesystem::path root,
                        std::vector<std::filesystem::path> sourceFiles,
                        QObject* parent = nullptr);

public slots:
    void run();

signals:
    void finished(CodeGraph graph);

private:
    std::filesystem::path m_root;
    std::vector<std::filesystem::path> m_sourceFiles;
};
