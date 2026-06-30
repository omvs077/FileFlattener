#pragma once
#include <QGraphicsView>
#include "GraphModel.h"

class QGraphicsScene;

class GraphView : public QGraphicsView {
    Q_OBJECT

public:
    explicit GraphView(QWidget* parent = nullptr);

    void setGraphModel(const GraphModel& model);

private:
    void layoutAndRender();

    QGraphicsScene* m_scene = nullptr;
    GraphModel m_model;
};
