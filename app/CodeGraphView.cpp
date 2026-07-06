#include "CodeGraphView.h"
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QWheelEvent>
#include <QBrush>
#include <QPen>
#include <functional>
#include <unordered_set>
#include <cmath>

CodeGraphView::CodeGraphView(QWidget* parent) : QGraphicsView(parent) {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
}

void CodeGraphView::wheelEvent(QWheelEvent* event) {
    double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    scale(factor, factor);
}

void CodeGraphView::zoomToFit() {
    if (!m_scene) return;
    QRectF r = m_scene->itemsBoundingRect();
    if (r.isEmpty()) return;
    fitInView(r, Qt::KeepAspectRatio);
}

void CodeGraphView::searchNodes(const QString& query) {
    QString q = query.trimmed();
    QGraphicsItem* firstMatch = nullptr;

    for (auto& kv : m_searchableItems) {
        auto* item = kv.second;
        bool matches = !q.isEmpty() && kv.first.contains(q, Qt::CaseInsensitive);

        if (auto* rect = qgraphicsitem_cast<QGraphicsRectItem*>(item)) {
            QPen pen = rect->pen();
            pen.setColor(matches ? QColor(220, 30, 30) : pen.color());
            rect->setPen(pen);
        }

        QPen basePen;
        if (item->data(0).isValid()) {
            basePen = item->data(0).value<QPen>();
        } else {
            basePen = (qgraphicsitem_cast<QGraphicsRectItem*>(item))
                ? qgraphicsitem_cast<QGraphicsRectItem*>(item)->pen()
                : qgraphicsitem_cast<QGraphicsEllipseItem*>(item)->pen();
            item->setData(0, QVariant::fromValue(basePen));
        }

        QPen newPen = basePen;
        if (matches) {
            newPen.setColor(QColor(220, 30, 30));
            newPen.setWidth(3);
        }

        if (auto* rectItem = qgraphicsitem_cast<QGraphicsRectItem*>(item)) {
            rectItem->setPen(newPen);
        } else if (auto* ellItem = qgraphicsitem_cast<QGraphicsEllipseItem*>(item)) {
            ellItem->setPen(newPen);
        }

        if (matches && !firstMatch) firstMatch = item;
    }

    if (firstMatch) {
        centerOn(firstMatch);
    }
}

void CodeGraphView::setCodeGraph(const CodeGraph& graph) {
    m_graph = graph;
    layoutAndRender();
}

void CodeGraphView::layoutAndRender() {
    m_scene->clear();
    m_searchableItems.clear();

    const qreal fileW = 180, classW = 150, methodW = 170;
    const qreal nodeH = 24;
    const qreal colW = 260;
    const qreal rowH = 28;
    const qreal columnGap = 60;
    const qreal rootGap = 20;

    std::unordered_map<int, std::vector<int>> children;
    std::unordered_set<int> hasParent;
    for (const auto& e : m_graph.edges) {
        if (e.type == CodeEdgeType::Contains) {
            children[e.fromId].push_back(e.toId);
            hasParent.insert(e.toId);
        }
    }

    std::vector<int> roots;
    for (const auto& n : m_graph.nodes) {
        if (!hasParent.count(n.id)) roots.push_back(n.id);
    }

    std::unordered_map<int, QPointF> centers;

    qreal targetColumnHeight = std::max<qreal>(500.0, std::sqrt(static_cast<qreal>(m_graph.nodes.size())) * rowH * 3.0);

    qreal xOffset = 0;
    qreal yOffset = 0;
    qreal currentColumnWidth = 0;

    for (int rootId : roots) {
        std::unordered_map<int, QPointF> local;
        qreal localRowCounter = 0;
        int maxDepth = 0;

        std::function<qreal(int,int)> layoutNode = [&](int id, int depth) -> qreal {
            maxDepth = std::max(maxDepth, depth);
            auto it = children.find(id);
            qreal y;
            if (it == children.end() || it->second.empty()) {
                y = localRowCounter * rowH;
                localRowCounter += 1;
            } else {
                qreal sum = 0;
                for (int childId : it->second) sum += layoutNode(childId, depth + 1);
                y = sum / static_cast<qreal>(it->second.size());
            }
            local[id] = QPointF(depth * colW, y);
            return y;
        };

        layoutNode(rootId, 0);

        qreal subtreeHeight = std::max<qreal>(localRowCounter * rowH, rowH);
        qreal subtreeWidth = (maxDepth + 1) * colW;

        if (yOffset > 0 && yOffset + subtreeHeight > targetColumnHeight) {
            xOffset += currentColumnWidth + columnGap;
            yOffset = 0;
            currentColumnWidth = 0;
        }

        for (const auto& kv : local) {
            centers[kv.first] = QPointF(kv.second.x() + xOffset, kv.second.y() + yOffset);
        }

        yOffset += subtreeHeight + rootGap;
        currentColumnWidth = std::max(currentColumnWidth, subtreeWidth);
    }

    qreal fallbackY = yOffset;
    for (const auto& n : m_graph.nodes) {
        if (!centers.count(n.id)) {
            centers[n.id] = QPointF(xOffset, fallbackY);
            fallbackY += rowH;
        }
    }

    for (const auto& node : m_graph.nodes) {
        QPointF center = centers[node.id];
        QGraphicsItem* item = nullptr;

        if (node.type == CodeNodeType::File) {
            auto* rect = new QGraphicsRectItem(center.x() - fileW/2, center.y() - nodeH/2, fileW, nodeH);
            rect->setBrush(QBrush(QColor(200, 230, 255)));
            rect->setPen(QPen(QColor(80, 120, 160)));
            item = rect;
        } else if (node.type == CodeNodeType::Class || node.type == CodeNodeType::Struct) {
            auto* rect = new QGraphicsRectItem(center.x() - classW/2, center.y() - nodeH/2, classW, nodeH);
            rect->setBrush(QBrush(QColor(220, 255, 220)));
            rect->setPen(QPen(QColor(90, 160, 90)));
            item = rect;
        } else {
            auto* ell = new QGraphicsEllipseItem(center.x() - methodW/2, center.y() - nodeH/2, methodW, nodeH);
            ell->setBrush(QBrush(QColor(255, 235, 200)));
            ell->setPen(QPen(QColor(180, 130, 60)));
            item = ell;
        }

        QString label = QString::fromStdString(node.name);
        QString tip = QString("%1\n%2:%3")
            .arg(label)
            .arg(QString::fromStdString(node.filePath))
            .arg(node.line);
        item->setToolTip(tip);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);

        auto* text = new QGraphicsTextItem(label, item);
        QRectF br = item->boundingRect();
        QRectF tb = text->boundingRect();
        text->setPos(br.left() + (br.width() - tb.width()) / 2,
                     br.top() + (br.height() - tb.height()) / 2);

        m_scene->addItem(item);
        m_searchableItems.push_back({label, item});
    }

    for (const auto& edge : m_graph.edges) {
        auto itFrom = centers.find(edge.fromId);
        auto itTo = centers.find(edge.toId);
        if (itFrom == centers.end() || itTo == centers.end()) continue;

        QColor color;
        switch (edge.type) {
            case CodeEdgeType::Includes: color = QColor(90, 140, 220); break;
            case CodeEdgeType::Contains: color = QColor(180, 180, 180); break;
            case CodeEdgeType::Inherits: color = QColor(220, 140, 60); break;
        }

        auto* line = new QGraphicsLineItem(itFrom->second.x(), itFrom->second.y(),
                                            itTo->second.x(), itTo->second.y());
        line->setPen(QPen(color, edge.type == CodeEdgeType::Contains ? 1.0 : 1.5));
        line->setZValue(-1);
        m_scene->addItem(line);
    }

    m_scene->setSceneRect(m_scene->itemsBoundingRect().adjusted(-40, -40, 40, 40));
}
