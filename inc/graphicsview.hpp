#ifndef __INCLUDE_GRAPHICSVIEW_H
#define __INCLUDE_GRAPHICSVIEW_H

#include <graphicsscene.hpp>
#include <QGraphicsView>
#include <QScrollBar>

class GraphicsView: public QGraphicsView {
	Q_OBJECT
public:
	explicit GraphicsView(QWidget *parent = nullptr);
	~GraphicsView();

	GraphicsScene *getScene() { return scene; }

protected:
	virtual void resizeEvent(QResizeEvent *event) override;

private:
	QGraphicsPixmapItem *item = nullptr;
	GraphicsScene *scene;
};

#endif //__INCLUDE_GRAPHICSVIEW_H
