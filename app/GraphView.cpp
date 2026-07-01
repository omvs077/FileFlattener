#include "GraphView.h"
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsLineItem>
#include <QGraphicsSceneMouseEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QTimer>
#include <QFontMetrics>
#include <QScrollBar>
#include <QtMath>
#include <map>
#include <unordered_map>

namespace {
constexpr qreal kNodeW = 120.0;
constexpr qreal kNodeH = 36.0;
constexpr qreal kRepulsion = 12000.0;
constexpr qreal kSpringLength = 140.0;
constexpr qreal kSpringStrength = 0.02;
constexpr qreal kDamping = 0.85;
constexpr qreal kMaxSpeed = 12.0;
}

class NodeItem : public QGraphicsRectItem {
public:
    NodeItem(int nodeId, QGraphicsItem* parent = nullptr)
        : QGraphicsRectItem(0, 0, kNodeW, kNodeH, parent), m_nodeId(nodeId) {
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
        setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
        setAcceptedMouseButtons(Qt::LeftButton);
    }

    int nodeId() const { return m_nodeId; }
    bool isPinned() const { return m_pinned; }
    QPointF velocity = QPointF(0, 0);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        m_pinned = true;
        velocity = QPointF(0, 0);
        QGraphicsRectItem::mousePressEvent(event);
    }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override {
        m_pinned = false;
        QGraphicsRectItem::mouseReleaseEvent(event);
    }

private:
    int m_nodeId = -1;
    bool m_pinned = false;
};

GraphView::GraphView(QWidget* parent)
    : QGraphicsView(parent) {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &GraphView::onPhysicsTick);
}

void GraphView::setGraphModel(const GraphModel& model) {
    m_model = model;
    layoutAndRender();
}

void GraphView::wheelEvent(QWheelEvent* event) {
    const qreal factor = (event->angleDelta().y() > 0) ? 1.15 : (1.0 / 1.15);
    scale(factor, factor);
    event->accept();
}

void GraphView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        m_panning = true;
        m_panStart = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void GraphView::mouseMoveEvent(QMouseEvent* event) {
    if (m_panning) {
        QPoint delta = event->pos() - m_panStart;
        m_panStart = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void GraphView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton && m_panning) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void GraphView::layoutAndRender() {
    m_scene->clear();
    m_nodeItems.clear();
    m_edgeItems.clear();
    m_edgePairs.clear();
    m_timer->stop();
    if (m_model.nodes.empty()) return;

    std::map<int, std::vector<int>> byDepth;
    for (size_t i = 0; i < m_model.nodes.size(); ++i)
        byDepth[m_model.nodes[i].depth].push_back(static_cast<int>(i));

    std::unordered_map<int, QPointF> seedPos;
    for (auto& entry : byDepth) {
        int depth = entry.first;
        qreal x = 0.0;
        for (int id : entry.second) {
            seedPos[id] = QPointF(x, depth * 90.0);
            x += kNodeW + 20.0;
        }
    }

    QFont labelFont;
    QFontMetrics fm(labelFont);

    m_nodeItems.assign(m_model.nodes.size(), nullptr);
    for (const auto& n : m_model.nodes) {
        NodeItem* item = new NodeItem(n.id);
        QColor fill = n.isDirectory ? QColor(100, 149, 237) : QColor(144, 238, 144);
        item->setBrush(QBrush(fill));
        item->setPen(QPen(Qt::black));
        item->setPos(seedPos[n.id]);
        m_scene->addItem(item);
        m_nodeItems[n.id] = item;

        QString fullName = QString::fromStdString(n.name.empty() ? "(root)" : n.name);
        QString elided = fm.elidedText(fullName, Qt::ElideRight, static_cast<int>(kNodeW - 10));
        QGraphicsSimpleTextItem* text = new QGraphicsSimpleTextItem(elided, item);
        text->setFont(labelFont);
        text->setBrush(Qt::black);
        QRectF tr = text->boundingRect();
        text->setPos((kNodeW - tr.width()) / 2.0, (kNodeH - tr.height()) / 2.0);
        item->setToolTip(fullName);
    }

    QPen edgePen(QColor(150, 150, 150));
    for (const auto& n : m_model.nodes) {
        if (n.parentId >= 0) {
            QGraphicsLineItem* line = m_scene->addLine(QLineF(), edgePen);
            line->setZValue(-1);
            m_edgeItems.push_back(line);
            m_edgePairs.emplace_back(n.parentId, n.id);
        }
    }

    // Draw initial edges
    for (size_t i = 0; i < m_edgePairs.size(); ++i) {
        int pid = m_edgePairs[i].first;
        int cid = m_edgePairs[i].second;
        QPointF p1 = m_nodeItems[pid]->pos() + QPointF(kNodeW / 2, kNodeH / 2);
        QPointF p2 = m_nodeItems[cid]->pos() + QPointF(kNodeW / 2, kNodeH / 2);
        m_edgeItems[i]->setLine(QLineF(p1, p2));
    }

    m_scene->setSceneRect(m_scene->itemsBoundingRect().adjusted(-200, -200, 200, 200));
    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    m_timer->start(16);
}

void GraphView::onPhysicsTick() {
    const int n = static_cast<int>(m_nodeItems.size());
    if (n == 0) return;

    if (n <= kPhysicsNodeCap) {
        std::vector<QPointF> forces(n, QPointF(0, 0));

        for (int i = 0; i < n; ++i) {
            QPointF pi = m_nodeItems[i]->pos() + QPointF(kNodeW / 2, kNodeH / 2);
            for (int j = i + 1; j < n; ++j) {
                QPointF pj = m_nodeItems[j]->pos() + QPointF(kNodeW / 2, kNodeH / 2);
                QPointF delta = pi - pj;
                qreal distSq = delta.x() * delta.x() + delta.y() * delta.y();
                if (distSq < 1.0) distSq = 1.0;
                qreal dist = qSqrt(distSq);
                QPointF dir = delta / dist;
                qreal f = kRepulsion / distSq;
                forces[i] += dir * f;
                forces[j] -= dir * f;
            }
        }

        for (const auto& pair : m_edgePairs) {
            int a = pair.first;
            int b = pair.second;
            QPointF pa = m_nodeItems[a]->pos() + QPointF(kNodeW / 2, kNodeH / 2);
            QPointF pb = m_nodeItems[b]->pos() + QPointF(kNodeW / 2, kNodeH / 2);
            QPointF delta = pb - pa;
            qreal dist = qSqrt(delta.x() * delta.x() + delta.y() * delta.y());
            if (dist < 1.0) dist = 1.0;
            QPointF dir = delta / dist;
            QPointF f = dir * ((dist - kSpringLength) * kSpringStrength);
            forces[a] += f;
            forces[b] -= f;
        }

        for (int i = 0; i < n; ++i) {
            NodeItem* item = m_nodeItems[i];
            if (item->isPinned()) { item->velocity = QPointF(0, 0); continue; }
            item->velocity = (item->velocity + forces[i]) * kDamping;
            qreal speed = qSqrt(item->velocity.x() * item->velocity.x() + item->velocity.y() * item->velocity.y());
            if (speed > kMaxSpeed) item->velocity *= (kMaxSpeed / speed);
            item->setPos(item->pos() + item->velocity);
        }
    }

    // Always sync edges (covers drag-follow on large graphs too)
    for (size_t i = 0; i < m_edgePairs.size(); ++i) {
        int pid = m_edgePairs[i].first;
        int cid = m_edgePairs[i].second;
        QPointF p1 = m_nodeItems[pid]->pos() + QPointF(kNodeW / 2, kNodeH / 2);
        QPointF p2 = m_nodeItems[cid]->pos() + QPointF(kNodeW / 2, kNodeH / 2);
        m_edgeItems[i]->setLine(QLineF(p1, p2));
    }
}
