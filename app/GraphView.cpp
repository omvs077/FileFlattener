#include "GraphView.h"
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
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
constexpr qreal kNodeH          = 42.0;
constexpr qreal kHSpacing       = 30.0;
constexpr qreal kVSpacing       = 110.0;
constexpr qreal kToggleR        = 8.0;
constexpr qreal kMinNodeW       = 100.0;
constexpr qreal kPadX           = 22.0;   // horizontal text padding per side
constexpr qreal kRepulsion      = 20000.0;
constexpr qreal kSpringStrength = 0.016;
constexpr qreal kDamping        = 0.85;
constexpr qreal kMaxSpeed       = 12.0;

QString iconForName(const QString& name, bool isDir, bool isRoot) {
    if (isRoot) return u8"\U0001F3E0 ";
    if (isDir)  return u8"\U0001F4C1 ";
    QString ext = name.section('.', -1).toLower();
    if (ext=="cpp"||ext=="c"||ext=="cc"||ext=="cxx") return u8"\U0001F5A5 ";
    if (ext=="h"||ext=="hpp")                         return u8"\U0001F4C4 ";
    if (ext=="py")                                    return u8"\U0001F40D ";
    if (ext=="js"||ext=="ts")                         return u8"\U0001F4DC ";
    if (ext=="json"||ext=="xml"||ext=="yaml"||ext=="yml") return u8"\U0001F4CB ";
    if (ext=="png"||ext=="jpg"||ext=="jpeg"||ext=="gif"||ext=="bmp"||ext=="svg") return u8"\U0001F5BC ";
    if (ext=="mp4"||ext=="mkv"||ext=="avi"||ext=="mov") return u8"\U0001F3AC ";
    if (ext=="mp3"||ext=="wav"||ext=="flac")          return u8"\U0001F3B5 ";
    if (ext=="zip"||ext=="rar"||ext=="7z"||ext=="tar") return u8"\U0001F4E6 ";
    if (ext=="pdf")                                   return u8"\U0001F4D5 ";
    if (ext=="txt"||ext=="md")                        return u8"\U0001F4DD ";
    if (ext=="exe"||ext=="dll")                       return u8"\U00002699 ";
    return u8"\U0001F4C4 ";
}
} // namespace

// ── NodeItem ──────────────────────────────────────────────────────────────────

class NodeItem;
static NodeItem* findNodeItem(QGraphicsItem* item);

class NodeItem : public QGraphicsRectItem {
public:
    NodeItem(int nodeId, bool isDir, bool isRoot, const QString& fullName,
             QGraphicsItem* parent = nullptr)
        : QGraphicsRectItem(parent), m_nodeId(nodeId), m_isDir(isDir)
    {
        QFont font;
        font.setPointSize(8);
        if (isDir || isRoot) font.setBold(true);

        QString icon  = iconForName(fullName, isDir, isRoot);
        QString label = icon + fullName;
        QFontMetrics fm(font);
        qreal textW   = fm.horizontalAdvance(label);
        // Extra room for toggle button on dirs.
        qreal toggleExtra = (isDir && !isRoot) ? kToggleR * 2 + 10 : 0;
        m_nodeW = qMax(kMinNodeW, textW + kPadX * 2 + toggleExtra);

        setRect(0, 0, m_nodeW, kNodeH);
        setFlag(ItemIsMovable, true);
        setFlag(ItemSendsGeometryChanges, true);
        setAcceptedMouseButtons(Qt::LeftButton);

        m_radius = isRoot ? 10.0 : (isDir ? 6.0 : 2.0);

        QColor fill, textColor;
        QPen   border;
        if (isRoot) {
            fill = QColor(255,179,0); border = QPen(QColor(180,110,0),3); textColor = QColor(50,25,0);
        } else if (isDir) {
            fill = QColor(65,125,215); border = QPen(QColor(35,75,155),1.5); textColor = Qt::white;
        } else {
            fill = QColor(115,205,115); border = QPen(QColor(55,135,55),1); textColor = QColor(15,15,15);
        }
        setBrush(QBrush(fill));
        setPen(border);
        m_textColor = textColor;

        // Left-aligned text with icon, 6px left margin.
        auto* text = new QGraphicsSimpleTextItem(label, this);
        text->setFont(font);
        text->setBrush(textColor);
        QRectF tr = text->boundingRect();
        text->setPos(6.0, (kNodeH - tr.height()) / 2.0);
        setToolTip(fullName);
        m_label = label;
        m_font  = font;
    }

    qreal nodeW() const { return m_nodeW; }
    int   nodeId() const { return m_nodeId; }
    bool  isDir()  const { return m_isDir; }
    bool  isPinned() const { return m_pinned; }
    QPointF velocity;

    void setToggle(QGraphicsEllipseItem* t, QGraphicsSimpleTextItem* lbl) {
        m_toggle = t; m_toggleLbl = lbl;
    }
    void updateToggleSign(bool collapsed) {
        if (m_toggleLbl) m_toggleLbl->setText(collapsed ? "+" : "\xe2\x88\x92"); // − (minus sign)
        if (m_toggle)    m_toggle->setBrush(collapsed ? QColor(200,80,0) : QColor(40,90,180));
    }
    void setHighlight(bool on) { m_highlighted = on; update(); }

protected:
    void paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override {
        p->setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRoundedRect(boundingRect(), m_radius, m_radius);
        p->fillPath(path, brush());
        p->setPen(m_highlighted ? QPen(QColor(255,220,0),3) : pen());
        p->drawPath(path);
    }
    void mousePressEvent(QGraphicsSceneMouseEvent* e) override {
        m_pinned = true; velocity = {}; QGraphicsRectItem::mousePressEvent(e);
    }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e) override {
        m_pinned = false; QGraphicsRectItem::mouseReleaseEvent(e);
    }

private:
    int     m_nodeId = -1;
    bool    m_isDir  = false;
    bool    m_pinned = false;
    bool    m_highlighted = false;
    qreal   m_radius = 4.0;
    qreal   m_nodeW  = kMinNodeW;
    QString m_label;
    QFont   m_font;
    QColor  m_textColor;
    QGraphicsEllipseItem*     m_toggle    = nullptr;
    QGraphicsSimpleTextItem*  m_toggleLbl = nullptr;
};

static NodeItem* findNodeItem(QGraphicsItem* item) {
    while (item) {
        if (auto* n = dynamic_cast<NodeItem*>(item)) return n;
        item = item->parentItem();
    }
    return nullptr;
}

// ── GraphView ─────────────────────────────────────────────────────────────────

GraphView::GraphView(QWidget* parent) : QGraphicsView(parent) {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(AnchorUnderMouse);
    setResizeAnchor(AnchorUnderMouse);
    viewport()->setCursor(Qt::OpenHandCursor);
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &GraphView::onPhysicsTick);
}

void GraphView::setGraphModel(const GraphModel& model) {
    m_model = model;
    m_collapsed.clear();
    layoutAndRender();
}

void GraphView::searchNodes(const QString& query) {
    if (m_nodeItems.empty()) return;
    QString q = query.trimmed().toLower();
    for (auto* item : m_nodeItems) {
        if (!item) continue;
        item->setHighlight(!q.isEmpty() && item->toolTip().toLower().contains(q));
    }
}

void GraphView::expandAll() {
    m_collapsed.clear();
    for (auto* item : m_nodeItems) if (item) item->setVisible(true);
    for (auto* item : m_nodeItems) if (item) item->updateToggleSign(false);
    rebuildEdges();
}

void GraphView::collapseAll() {
    // Collapse all non-root directory nodes.
    for (const auto& n : m_model.nodes) {
        if (n.isDirectory && n.id != 0 && !n.childIds.empty())
            m_collapsed.insert(n.id);
    }
    // Hide everything except root and its direct children.
    for (auto* item : m_nodeItems) if (item) item->setVisible(false);
    if (!m_nodeItems.empty() && m_nodeItems[0]) {
        m_nodeItems[0]->setVisible(true);
        m_nodeItems[0]->updateToggleSign(false);
        for (int childId : m_model.nodes[0].childIds) {
            if (m_nodeItems[childId]) {
                m_nodeItems[childId]->setVisible(true);
                m_nodeItems[childId]->updateToggleSign(true);
            }
        }
    }
    rebuildEdges();
}

void GraphView::wheelEvent(QWheelEvent* e) {
    scale(e->angleDelta().y() > 0 ? 1.15 : (1.0/1.15), e->angleDelta().y() > 0 ? 1.15 : (1.0/1.15));
    e->accept();
}

void GraphView::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        QGraphicsItem* hit = itemAt(e->pos());
        if (auto* ell = dynamic_cast<QGraphicsEllipseItem*>(hit)) {
            QVariant v = ell->data(0);
            if (v.isValid()) {
                int nodeId = v.toInt();
                bool nowCollapsed = !m_collapsed.count(nodeId);
                if (nowCollapsed) m_collapsed.insert(nodeId);
                else              m_collapsed.erase(nodeId);
                setSubtreeVisible(nodeId, !nowCollapsed);
                if (m_nodeItems[nodeId]) m_nodeItems[nodeId]->updateToggleSign(nowCollapsed);
                rebuildEdges();
                e->accept(); return;
            }
        }
        if (!findNodeItem(hit)) {
            m_panning = true; m_panStart = e->pos();
            viewport()->setCursor(Qt::ClosedHandCursor);
            e->accept(); return;
        }
    }
    QGraphicsView::mousePressEvent(e);
}

void GraphView::mouseMoveEvent(QMouseEvent* e) {
    if (m_panning) {
        QPoint d = e->pos() - m_panStart; m_panStart = e->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - d.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - d.y());
        e->accept(); return;
    }
    QGraphicsView::mouseMoveEvent(e);
}

void GraphView::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && m_panning) {
        m_panning = false;
        viewport()->setCursor(Qt::OpenHandCursor);
        e->accept(); return;
    }
    QGraphicsView::mouseReleaseEvent(e);
}

void GraphView::computeSubtreeLayout(int nodeId, qreal& xCursor, int depth,
                                      std::unordered_map<int,QPointF>& out) {
    const GraphNode& node = m_model.nodes[nodeId];
    bool collapsed = m_collapsed.count(nodeId) > 0;
    qreal nodeW = m_nodeItems[nodeId] ? m_nodeItems[nodeId]->nodeW() : kMinNodeW;

    if (node.childIds.empty() || collapsed) {
        out[nodeId] = QPointF(xCursor, depth * kVSpacing);
        xCursor += nodeW + kHSpacing;
        return;
    }
    qreal subtreeStart = xCursor;
    for (int childId : node.childIds)
        computeSubtreeLayout(childId, xCursor, depth + 1, out);
    qreal subtreeEnd = xCursor - kHSpacing;
    out[nodeId] = QPointF((subtreeStart + subtreeEnd - nodeW) / 2.0, depth * kVSpacing);
}

void GraphView::setSubtreeVisible(int nodeId, bool visible) {
    for (int childId : m_model.nodes[nodeId].childIds) {
        if (m_nodeItems[childId]) m_nodeItems[childId]->setVisible(visible);
        bool childCollapsed = m_collapsed.count(childId) > 0;
        if (visible && !childCollapsed)
            setSubtreeVisible(childId, true);
        else if (!visible)
            setSubtreeVisible(childId, false);
    }
}

void GraphView::updateToggleLabel(int nodeId) {
    if (m_nodeItems[nodeId])
        m_nodeItems[nodeId]->updateToggleSign(m_collapsed.count(nodeId) > 0);
}

void GraphView::rebuildEdges() {
    for (auto* line : m_edgeItems) m_scene->removeItem(line);
    m_edgeItems.clear(); m_edgePairs.clear();
    QPen edgePen(QColor(160,160,160));
    for (const auto& n : m_model.nodes) {
        if (n.parentId < 0) continue;
        if (!m_nodeItems[n.id]      || !m_nodeItems[n.id]->isVisible())      continue;
        if (!m_nodeItems[n.parentId]|| !m_nodeItems[n.parentId]->isVisible()) continue;
        auto* line = m_scene->addLine(QLineF(), edgePen);
        line->setZValue(-1);
        m_edgeItems.push_back(line);
        m_edgePairs.emplace_back(n.parentId, n.id);
    }
}

void GraphView::layoutAndRender() {
    m_scene->clear();
    m_nodeItems.clear(); m_edgeItems.clear(); m_edgePairs.clear();
    m_tickCount = 0; m_timer->stop();
    if (m_model.nodes.empty()) return;

    // Auto-collapse folders with more than 8 direct children.
    for (const auto& n : m_model.nodes) {
        if (n.isDirectory && (int)n.childIds.size() > 8)
            m_collapsed.insert(n.id);
    }

    // Node cap: folder-only above 400, message above 2000.
    const int totalNodes = static_cast<int>(m_model.nodes.size());
    bool folderOnlyMode = false;
    if (totalNodes > 2000) {
        m_scene->addText("Folder too large to visualize (>2000 nodes).\nUse \"Export Folder Structure\" for a text view.");
        return;
    } else if (totalNodes > 400) {
        folderOnlyMode = true;
    }

    // First pass: create NodeItems so computeSubtreeLayout can read nodeW().
    m_nodeItems.assign(m_model.nodes.size(), nullptr);
    for (const auto& n : m_model.nodes) {
        QString label = QString::fromStdString(n.name.empty() ? "(root)" : n.name);
        if (folderOnlyMode && !n.isDirectory) continue;
        auto* item = new NodeItem(n.id, n.isDirectory, n.id==0, label);
        m_nodeItems[n.id] = item;
    }

    // Compute layout positions.
    std::unordered_map<int,QPointF> seedPos;
    qreal xCursor = 0.0;
    computeSubtreeLayout(0, xCursor, 0, seedPos);

    // Depth bands.
    std::map<int,int> depthCount;
    for (const auto& n : m_model.nodes) depthCount[n.depth]++;
    for (auto& [depth, cnt] : depthCount) {
        qreal bandY = depth * kVSpacing - 12;
        QColor band = (depth%2==0) ? QColor(230,235,255,55) : QColor(220,245,220,55);
        auto* rect = m_scene->addRect(-30, bandY, xCursor+60, kNodeH+24,
                                       QPen(Qt::NoPen), QBrush(band));
        rect->setZValue(-2);
    }

    // Add nodes to scene with positions.
    for (const auto& n : m_model.nodes) {
        NodeItem* item = m_nodeItems[n.id];
        item->setPos(seedPos.count(n.id) ? seedPos[n.id] : QPointF(0,0));
        m_scene->addItem(item);

        // Toggle button for expandable dirs.
        if (n.isDirectory && !n.childIds.empty()) {
            qreal nw = item->nodeW();
            auto* toggle = new QGraphicsEllipseItem(
                nw - kToggleR*2 - 5, (kNodeH - kToggleR*2)/2,
                kToggleR*2, kToggleR*2, item);
            toggle->setBrush(QBrush(QColor(40,90,180)));
            toggle->setPen(QPen(Qt::white,1.5));
            toggle->setData(0, n.id);
            toggle->setZValue(2);

            auto* plus = new QGraphicsSimpleTextItem("+", toggle);
            QFont f; f.setBold(true); f.setPointSize(7);
            plus->setFont(f); plus->setBrush(Qt::white);
            QRectF pr = plus->boundingRect();
            plus->setPos((kToggleR*2 - pr.width())/2, (kToggleR*2 - pr.height())/2);
            item->setToggle(toggle, plus);
        }
    }

    rebuildEdges();
    for (size_t i = 0; i < m_edgePairs.size(); ++i) {
        int pid = m_edgePairs[i].first, cid = m_edgePairs[i].second;
        qreal pw = m_nodeItems[pid]->nodeW(), cw = m_nodeItems[cid]->nodeW();
        m_edgeItems[i]->setLine(QLineF(
            m_nodeItems[pid]->pos() + QPointF(pw/2, kNodeH/2),
            m_nodeItems[cid]->pos() + QPointF(cw/2, kNodeH/2)));
    }

    m_scene->setSceneRect(m_scene->itemsBoundingRect().adjusted(-200,-200,200,200));
    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    m_timer->start(16);
}

void GraphView::onPhysicsTick() {
    const int n = static_cast<int>(m_nodeItems.size());
    if (n == 0) return;

    if (n <= kPhysicsNodeCap) {
        std::vector<QPointF> forces(n, QPointF(0,0));
        for (int i = 0; i < n; ++i) {
            if (!m_nodeItems[i]->isVisible()) continue;
            QPointF pi = m_nodeItems[i]->pos() + QPointF(m_nodeItems[i]->nodeW()/2, kNodeH/2);
            for (int j = i+1; j < n; ++j) {
                if (!m_nodeItems[j]->isVisible()) continue;
                QPointF d  = pi - (m_nodeItems[j]->pos() + QPointF(m_nodeItems[j]->nodeW()/2, kNodeH/2));
                qreal dsq  = qMax(d.x()*d.x()+d.y()*d.y(), 1.0);
                QPointF f  = (d/qSqrt(dsq)) * (kRepulsion/dsq);
                forces[i] += f; forces[j] -= f;
            }
        }
        for (const auto& [a,b] : m_edgePairs) {
            qreal aw = m_nodeItems[a]->nodeW(), bw = m_nodeItems[b]->nodeW();
            qreal springLen = (aw + bw) / 2.0 + 50.0;
            QPointF d   = (m_nodeItems[b]->pos()+QPointF(bw/2,kNodeH/2))
                        - (m_nodeItems[a]->pos()+QPointF(aw/2,kNodeH/2));
            qreal dist  = qMax(qSqrt(d.x()*d.x()+d.y()*d.y()), 1.0);
            QPointF f   = (d/dist)*((dist - springLen)*kSpringStrength);
            forces[a]  += f; forces[b] -= f;
        }
        for (int i = 0; i < n; ++i) {
            NodeItem* item = m_nodeItems[i];
            if (!item->isVisible()||item->isPinned()) { item->velocity={}; continue; }
            item->velocity = (item->velocity + forces[i]) * kDamping;
            qreal spd = qSqrt(item->velocity.x()*item->velocity.x()+item->velocity.y()*item->velocity.y());
            if (spd > kMaxSpeed) item->velocity *= (kMaxSpeed/spd);
            item->setPos(item->pos()+item->velocity);
        }
    }

    for (size_t i = 0; i < m_edgePairs.size(); ++i) {
        int pid = m_edgePairs[i].first, cid = m_edgePairs[i].second;
        qreal pw = m_nodeItems[pid]->nodeW(), cw = m_nodeItems[cid]->nodeW();
        m_edgeItems[i]->setLine(QLineF(
            m_nodeItems[pid]->pos()+QPointF(pw/2,kNodeH/2),
            m_nodeItems[cid]->pos()+QPointF(cw/2,kNodeH/2)));
    }

    if (++m_tickCount % 30 == 0)
        m_scene->setSceneRect(m_scene->itemsBoundingRect().adjusted(-200,-200,200,200));
}
