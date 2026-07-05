#pragma once
#include <QGraphicsView>
#include "CodeLexer.h"
#include <unordered_map>

class QGraphicsScene;

class CodeGraphView : public QGraphicsView {
    Q_OBJECT

public:
    explicit CodeGraphView(QWidget* parent = nullptr);
    void setCodeGraph(const CodeGraph& graph);

protected:
    void wheelEvent(QWheelEvent* event) override;

private:
    void layoutAndRender();

    QGraphicsScene* m_scene = nullptr;
    CodeGraph m_graph;
};
