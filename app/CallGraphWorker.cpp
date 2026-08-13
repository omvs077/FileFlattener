#include "CallGraphWorker.h"
#include "CodeLexer.h"

CallGraphWorker::CallGraphWorker(std::filesystem::path root,
                                  std::vector<std::filesystem::path> sourceFiles,
                                  QObject* parent)
    : QObject(parent)
    , m_root(std::move(root))
    , m_sourceFiles(std::move(sourceFiles)) {
    qRegisterMetaType<CallGraphResult>("CallGraphResult");
}

void CallGraphWorker::run() {
    CallGraphResult result;
    try {
        CodeGraph baseGraph = CodeLexer::analyze(m_root, m_sourceFiles);
        result = CallGraphAnalyzer::analyze(m_root, m_sourceFiles, baseGraph);
    } catch (const std::exception&) {
        // Fall through with an empty result rather than crashing the worker thread.
    }
    emit finished(result);
}
