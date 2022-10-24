#include <graphicsscene.hpp>
#include <graphicsview.hpp>
#include <QGraphicsPixmapItem>
#include <QResizeEvent>
#include <QWheelEvent>

GraphicsView::GraphicsView(QWidget *parent): QGraphicsView(parent) {
	scene = new GraphicsScene(this);
	scene->setSceneRect(0, 0, width() - 20, height() - 20);
	scene->init();
	this->setMouseTracking(true);
	this->setScene(scene);
}

void GraphicsView::resizeEvent(QResizeEvent *event) {
	fitInView(
	    0, 0, width() - 20, height() - 20,
	    Qt::KeepAspectRatio
	);
}

GraphicsView::~GraphicsView() {
	delete scene;
}
