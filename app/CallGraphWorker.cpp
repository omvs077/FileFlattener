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
    CodeGraph baseGraph = CodeLexer::analyze(m_root, m_sourceFiles);
    CallGraphResult result = CallGraphAnalyzer::analyze(m_root, m_sourceFiles, baseGraph);
    emit finished(result);
}
