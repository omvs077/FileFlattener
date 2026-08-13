#include "CodeAnalysisWorker.h"

CodeAnalysisWorker::CodeAnalysisWorker(std::filesystem::path root,
                                       std::vector<std::filesystem::path> sourceFiles,
                                       QObject* parent)
    : QObject(parent)
    , m_root(std::move(root))
    , m_sourceFiles(std::move(sourceFiles)) {
    qRegisterMetaType<CodeGraph>("CodeGraph");
}

void CodeAnalysisWorker::run() {
    CodeGraph graph;
    try {
        graph = CodeLexer::analyze(m_root, m_sourceFiles);
    } catch (const std::exception&) {
        // Fall through with an empty graph rather than crashing the worker thread.
    }
    emit finished(graph);
}

