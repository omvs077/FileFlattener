#include "CallGraphView.h"
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsTextItem>
#include <QWheelEvent>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <cmath>

namespace {
constexpr qreal kNodeH = 26.0;
constexpr qreal kMinNodeW = 110.0;
constexpr qreal kMaxNodeW = 320.0;
constexpr qreal kLabelPadding = 28.0;
constexpr qreal kRepulsion = 42000.0;
constexpr qreal kSpringStrength = 0.014;
constexpr qreal kSpringGap = 90.0;
constexpr qreal kDamping = 0.85;
constexpr qreal kMaxSpeed = 14.0;
constexpr int kIterations = 320;
}

CallGraphView::CallGraphView(QWidget* parent) : QGraphicsView(parent) {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
}

void CallGraphView::wheelEvent(QWheelEvent* event) {
    double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    scale(factor, factor);
}

void CallGraphView::zoomToFit() {
    if (!m_scene) return;
    QRectF r = m_scene->itemsBoundingRect();
    if (r.isEmpty()) return;
    fitInView(r, Qt::KeepAspectRatio);
}

void CallGraphView::searchNodes(const QString& query) {
    QString q = query.trimmed();
    QGraphicsItem* firstMatch = nullptr;

    for (auto& kv : m_searchableItems) {
        auto* item = kv.second;
        bool matches = !q.isEmpty() && kv.first.contains(q, Qt::CaseInsensitive);

        QPen basePen;
        if (item->data(0).isValid()) {
            basePen = item->data(0).value<QPen>();
        } else if (auto* ell = qgraphicsitem_cast<QGraphicsEllipseItem*>(item)) {
            basePen = ell->pen();
            item->setData(0, QVariant::fromValue(basePen));
        }

        QPen newPen = basePen;
        if (matches) {
            newPen.setColor(QColor(220, 30, 30));
            newPen.setWidth(3);
        }
        if (auto* ell = qgraphicsitem_cast<QGraphicsEllipseItem*>(item)) {
            ell->setPen(newPen);
        }

        if (matches && !firstMatch) firstMatch = item;
    }

    if (firstMatch) centerOn(firstMatch);
}

void CallGraphView::setCallGraph(const CodeGraph& graph) {
    m_graph = graph;
    layoutAndRender();
}

void CallGraphView::layoutAndRender() {
    m_scene->clear();
    m_searchableItems.clear();

    std::unordered_set<int> participating;
    for (const auto& e : m_graph.edges) {
        if (e.type != CodeEdgeType::Calls) continue;
        participating.insert(e.fromId);
        participating.insert(e.toId);
    }

    std::vector<int> nodeIds;
    for (const auto& n : m_graph.nodes) {
        if (participating.count(n.id)) nodeIds.push_back(n.id);
        if (static_cast<int>(nodeIds.size()) >= kMaxRenderedNodes) break;
    }

    std::unordered_set<int> included(nodeIds.begin(), nodeIds.end());
    std::vector<std::pair<int,int>> edgePairs;
    for (const auto& e : m_graph.edges) {
        if (e.type != CodeEdgeType::Calls) continue;
        if (included.count(e.fromId) && included.count(e.toId)) {
            edgePairs.push_back({e.fromId, e.toId});
        }
    }

    if (nodeIds.empty()) {
        m_scene->setSceneRect(0, 0, 400, 200);
        return;
    }

    std::unordered_map<int, const CodeNode*> nodeById;
    for (const auto& n2 : m_graph.nodes) nodeById[n2.id] = &n2;

    QFont font;
    QFontMetrics fm(font);
    std::unordered_map<int, qreal> nodeWidth;
    for (int id : nodeIds) {
        const CodeNode* node = nodeById[id];
        QString label = node ? QString::fromStdString(node->name) : QString();
        qreal w = fm.horizontalAdvance(label) + kLabelPadding;
        w = std::max(kMinNodeW, std::min(kMaxNodeW, w));
        nodeWidth[id] = w;
    }

    std::unordered_map<int, QPointF> pos;
    std::unordered_map<int, QPointF> velocity;
    std::unordered_map<int, int> indexOf;
    const int n = static_cast<int>(nodeIds.size());
    qreal radius = std::max<qreal>(260.0, n * 22.0);
    for (int i = 0; i < n; ++i) {
        qreal angle = (2.0 * M_PI * i) / n;
        pos[nodeIds[i]] = QPointF(radius * std::cos(angle), radius * std::sin(angle));
        velocity[nodeIds[i]] = QPointF(0, 0);
        indexOf[nodeIds[i]] = i;
    }

    for (int iter = 0; iter < kIterations; ++iter) {
        std::vector<QPointF> forces(n, QPointF(0, 0));
        for (int i = 0; i < n; ++i) {
            QPointF pi = pos[nodeIds[i]];
            for (int j = i + 1; j < n; ++j) {
                QPointF d = pi - pos[nodeIds[j]];
                qreal dsq = std::max(d.x()*d.x() + d.y()*d.y(), 1.0);
                QPointF f = (d / std::sqrt(dsq)) * (kRepulsion / dsq);
                forces[i] += f;
                forces[j] -= f;
            }
        }
        for (const auto& [a, b] : edgePairs) {
            qreal springLen = (nodeWidth[a] + nodeWidth[b]) / 2.0 + kSpringGap;
            QPointF d = pos[b] - pos[a];
            qreal dist = std::max(std::sqrt(d.x()*d.x() + d.y()*d.y()), 1.0);
            QPointF f = (d / dist) * ((dist - springLen) * kSpringStrength);
            forces[indexOf[a]] += f;
            forces[indexOf[b]] -= f;
        }
        for (int i = 0; i < n; ++i) {
            int id = nodeIds[i];
            velocity[id] = (velocity[id] + forces[i]) * kDamping;
            qreal spd = std::sqrt(velocity[id].x()*velocity[id].x() + velocity[id].y()*velocity[id].y());
            if (spd > kMaxSpeed) velocity[id] *= (kMaxSpeed / spd);
            pos[id] += velocity[id];
        }
    }

    for (int id : nodeIds) {
        const CodeNode* node = nodeById[id];
        if (!node) continue;
        QPointF center = pos[id];
        qreal w = nodeWidth[id];

        auto* ell = new QGraphicsEllipseItem(center.x() - w/2, center.y() - kNodeH/2, w, kNodeH);
        ell->setBrush(QBrush(QColor(255, 235, 200)));
        ell->setPen(QPen(QColor(180, 130, 60)));
        ell->setFlag(QGraphicsItem::ItemIsSelectable, true);

        QString label = QString::fromStdString(node->name);
        QString tip = QString("%1\n%2").arg(label).arg(QString::fromStdString(node->filePath));
        ell->setToolTip(tip);

        auto* text = new QGraphicsTextItem(label, ell);
        QRectF br = ell->boundingRect();
        QRectF tb = text->boundingRect();
        text->setPos(br.left() + (br.width() - tb.width()) / 2,
                     br.top() + (br.height() - tb.height()) / 2);

        m_scene->addItem(ell);
        m_searchableItems.push_back({label, ell});
    }

    for (const auto& [a, b] : edgePairs) {
        QPointF pa = pos[a];
        QPointF pb = pos[b];
        QPointF d = pb - pa;
        qreal len = std::max(std::sqrt(d.x()*d.x() + d.y()*d.y()), 1.0);
        QPointF unit = d / len;
        qreal aHalf = nodeWidth[a] / 2.0 * 0.6;
        qreal bHalf = nodeWidth[b] / 2.0 * 0.6;
        QPointF start = pa + unit * aHalf;
        QPointF end   = pb - unit * bHalf;

        auto* line = new QGraphicsLineItem(start.x(), start.y(), end.x(), end.y());
        line->setPen(QPen(QColor(140, 165, 205), 1.1));
        line->setZValue(-1);
        m_scene->addItem(line);

        qreal arrowSize = 8.0;
        QPointF perp(-unit.y(), unit.x());
        QPolygonF arrow;
        arrow << end
              << (end - unit * arrowSize + perp * (arrowSize * 0.5))
              << (end - unit * arrowSize - perp * (arrowSize * 0.5));
        auto* arrowItem = new QGraphicsPolygonItem(arrow);
        arrowItem->setBrush(QBrush(QColor(140, 165, 205)));
        arrowItem->setPen(Qt::NoPen);
        arrowItem->setZValue(-1);
        m_scene->addItem(arrowItem);
    }

    m_scene->setSceneRect(m_scene->itemsBoundingRect().adjusted(-60, -60, 60, 60));
}
