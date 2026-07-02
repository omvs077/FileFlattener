#pragma once
#include <QGraphicsView>
#include "GraphModel.h"
#include <vector>
#include <utility>
#include <unordered_set>

class QGraphicsScene;
class QTimer;
class QGraphicsLineItem;
class NodeItem;

class GraphView : public QGraphicsView {
    Q_OBJECT

public:
    explicit GraphView(QWidget* parent = nullptr);
    void setGraphModel(const GraphModel& model);
    void searchNodes(const QString& query);

public slots:
    void expandAll();
    void collapseAll();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onPhysicsTick();

private:
    void layoutAndRender();
    void computeSubtreeLayout(int nodeId, qreal& xCursor, int depth,
                               std::unordered_map<int,QPointF>& out);
    void setSubtreeVisible(int nodeId, bool visible);
    void rebuildEdges();
    void updateToggleLabel(int nodeId);

    QGraphicsScene* m_scene  = nullptr;
    QTimer*         m_timer  = nullptr;
    GraphModel      m_model;

    std::vector<NodeItem*>           m_nodeItems;
    std::vector<QGraphicsLineItem*>  m_edgeItems;
    std::vector<std::pair<int,int>>  m_edgePairs;
    std::unordered_set<int>          m_collapsed;

    bool   m_panning   = false;
    QPoint m_panStart;
    int    m_tickCount = 0;

    static constexpr int kPhysicsNodeCap = 600;
};
