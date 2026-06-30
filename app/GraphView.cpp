#include "GraphView.h"
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <map>
#include <unordered_map>

GraphView::GraphView(QWidget* parent)
    : QGraphicsView(parent) {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
}

void GraphView::setGraphModel(const GraphModel& model) {
    m_model = model;
    layoutAndRender();
}

void GraphView::layoutAndRender() {
    m_scene->clear();
    if (m_model.nodes.empty()) return;

    const qreal nodeW = 120.0;
    const qreal nodeH = 36.0;
    const qreal hSpacing = 20.0;
    const qreal vSpacing = 80.0;

    // Group node ids by depth (preserves build order within a depth level).
    std::map<int, std::vector<int>> byDepth;
    for (size_t i = 0; i < m_model.nodes.size(); ++i) {
        byDepth[m_model.nodes[i].depth].push_back(static_cast<int>(i));
    }

    std::unordered_map<int, QPointF> pos;
    for (auto& entry : byDepth) {
        int depth = entry.first;
        qreal x = 0.0;
        for (int id : entry.second) {
            pos[id] = QPointF(x, depth * vSpacing);
            x += nodeW + hSpacing;
        }
    }

    QPen edgePen(QColor(150, 150, 150));
    for (const auto& n : m_model.nodes) {
        if (n.parentId >= 0) {
            QPointF p1 = pos[n.parentId] + QPointF(nodeW / 2, nodeH);
            QPointF p2 = pos[n.id] + QPointF(nodeW / 2, 0);
            m_scene->addLine(QLineF(p1, p2), edgePen);
        }
    }

    for (const auto& n : m_model.nodes) {
        QPointF p = pos[n.id];
        QColor fill = n.isDirectory ? QColor(100, 149, 237) : QColor(144, 238, 144);
        m_scene->addRect(p.x(), p.y(), nodeW, nodeH, QPen(Qt::black), QBrush(fill));

        QString label = QString::fromStdString(n.name.empty() ? "(root)" : n.name);
        QGraphicsTextItem* text = m_scene->addText(label);
        text->setPos(p.x() + 4, p.y() + 8);
        text->setDefaultTextColor(Qt::black);
        text->setTextWidth(nodeW - 8);
    }

    m_scene->setSceneRect(m_scene->itemsBoundingRect().adjusted(-40, -40, 40, 40));
}
