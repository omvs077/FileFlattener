#pragma once
#include <QGraphicsView>
#include "GraphModel.h"
#include <vector>
#include <utility>

class QGraphicsScene;
class QTimer;
class QGraphicsLineItem;
class NodeItem;

class GraphView : public QGraphicsView {
    Q_OBJECT

public:
    explicit GraphView(QWidget* parent = nullptr);
    void setGraphModel(const GraphModel& model);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onPhysicsTick();

private:
    void layoutAndRender();

    QGraphicsScene* m_scene = nullptr;
    QTimer* m_timer = nullptr;
    GraphModel m_model;

    std::vector<NodeItem*> m_nodeItems;
    std::vector<QGraphicsLineItem*> m_edgeItems;
    std::vector<std::pair<int,int>> m_edgePairs;

    // Pan state
    bool m_panning = false;
    QPoint m_panStart;

    static constexpr int kPhysicsNodeCap = 800;
};
