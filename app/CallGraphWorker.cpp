#include "CallGraphWorker.h"

CallGraphWorker::CallGraphWorker(std::filesystem::path root,
                                  std::vector<std::filesystem::path> sourceFiles,
                                  CodeGraph baseGraph,
                                  QObject* parent)
    : QObject(parent)
    , m_root(std::move(root))
    , m_sourceFiles(std::move(sourceFiles))
    , m_baseGraph(std::move(baseGraph)) {
    qRegisterMetaType<CallGraphResult>("CallGraphResult");
}

void CallGraphWorker::run() {
    CallGraphResult result = CallGraphAnalyzer::analyze(m_root, m_sourceFiles, m_baseGraph);
    emit finished(result);
}
