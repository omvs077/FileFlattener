#pragma once
#include <QGraphicsView>
#include "CodeLexer.h"
#include <unordered_map>
#include <vector>
#include <utility>

class QGraphicsScene;
class QGraphicsItem;

class CodeGraphView : public QGraphicsView {
    Q_OBJECT

public:
    explicit CodeGraphView(QWidget* parent = nullptr);
    void setCodeGraph(const CodeGraph& graph);

public slots:
    void zoomToFit();
    void searchNodes(const QString& query);

protected:
    void wheelEvent(QWheelEvent* event) override;

private:
    void layoutAndRender();

    QGraphicsScene* m_scene = nullptr;
    CodeGraph m_graph;
    std::vector<std::pair<QString, QGraphicsItem*>> m_searchableItems;
};
