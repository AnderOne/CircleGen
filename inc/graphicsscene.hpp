#ifndef __INCLUDE_GRAPHICSSCENE_H
#define __INCLUDE_GRAPHICSSCENE_H

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QLabel>
#include <QFile>
#include "monitor.hpp"
#include <cmath>
#include <memory>
#include <set>

class GraphicsScene: public QGraphicsScene {
	Q_OBJECT
public:
	static constexpr char ANY = 'x';

	enum Mode { Free, Tree, Test };

	explicit GraphicsScene(QObject *parent = nullptr);

	~GraphicsScene();

	QPointF pointFromScene(const QPointF& point) const;
	QPointF pointToScene(const QPointF& point) const;
	bool loadFromFile(QFile *file);
	bool saveToFile(QFile *file) const;

	void check(std::vector<std::string>& result) const;

	void setMonitor(Monitor *monitor) {
		_monitor = monitor;
	}
	Mode getMode() const { return _mode; }
	void setMode(Mode mode);

	void setVisibleKnots(bool visible) {
		_visibleKnots = visible;
		updateKnots();
	}
	void setFilledArea(bool filled) {
		_filledArea = filled;
		update();
	}
	bool placeToLocal(const QPointF &loc, bool inv);
	bool placeToPoint(const QPointF &pos);
	bool placeToChord(bool inv);
	bool isFixedPath() const;
	bool test(const QPointF &point);
	bool goToBack();
	bool goToNext(bool ans);
	bool goToInv();
	bool goToPath(const std::string &path);
	void savePath();
	void start();
	void clear();
	void reset();
	void init();

protected:
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
	virtual void wheelEvent(QGraphicsSceneWheelEvent *wheelEvent) override;

private:

	struct CircleItem: public QGraphicsEllipseItem {

		CircleItem(int index): QGraphicsEllipseItem(0, 0, 1, 1), _index(index) {}

		virtual ~CircleItem() {}

		void update()
		{
			if (auto gs = dynamic_cast<GraphicsScene*>(scene())) {
				const QPointF offset(_radius, _radius);
				auto p1 = gs->pointToScene(_center - offset);
				auto p2 = gs->pointToScene(_center + offset);
				auto x1 = std::min(p1.x(), p2.x());
				auto x2 = std::max(p1.x(), p2.x());
				auto y1 = std::min(p1.y(), p2.y());
				auto y2 = std::max(p1.y(), p2.y());
				this->setRect(0, 0, x2 - x1, y2 - y1);
				QColor hiddenColor = _filled? QColor(220, 220, 220) :
				                              QColor(200, 200, 200);
				if (_filled) {
					QBrush brush; brush.setColor(hiddenColor);
					brush.setStyle(Qt::SolidPattern);
					setBrush(brush);
					setZValue(-1.);
				}
				else {
					setBrush(Qt::NoBrush);
					setZValue(+1.);
				}
				setPos(QPointF(x1, y1));
				QPen pen(
				    _opaque? _colors[_index % _colors.size()]:
				            hiddenColor
				);
				if (_index == 0) {
					pen.setDashPattern({1,4});
					pen.setWidth(3);
				}
				else {
					pen.setWidth(5);
				}
				setPen(pen);
			}
		}

		virtual bool contains(const QPointF& point) const override
		{
			if (auto gs = dynamic_cast<GraphicsScene*>(scene())) {
				const auto& rect = this->rect();
				const auto& c = rect.center();
				qreal dx = point.x() - c.x();
				qreal dy = point.y() - c.y();
				qreal dq = dx * dx + dy * dy;
				qreal r = rect.width() / 2.;
				qreal d = std::sqrt(dq);
				return std::abs(d - r) < 10;
			}
			return false;
		}

		bool containsPoint(const QPointF& point) const {
			qreal dx = point.x() - _center.x();
			qreal dy = point.y() - _center.y();
			return std::sqrt(dx * dx + dy * dy) < _radius + 1.e-7;
		}

		const QPointF& getCenter() const { return _center; }
		qreal  getRadius() const { return _radius; }
		bool isFilled() const { return _filled; }
		bool isOpaque() const { return _opaque; }
		int getIndex() const { return _index; }

		void setCenter(const QPointF& center) {
			_center = center;
			update();
		}
		void setRadius(qreal radius) {
			_radius = radius;
			update();
		}
		void setFilled(bool ok) {
			_filled = ok;
		}
		void setOpaque(bool ok) {
			_opaque = ok;
		}

	private:
		static const std::vector<QColor> _colors;
		QPointF _center;
		qreal _radius;
		bool _filled = false;
		bool _opaque = true;
		int _index;
	};

	struct KnotItem: public QGraphicsEllipseItem {

		KnotItem(): QGraphicsEllipseItem(0, 0, 10, 10) {}

		virtual ~KnotItem() {}

		void update()
		{
			auto gs = dynamic_cast<GraphicsScene*>(scene());
			if (!gs) return;
			auto p = gs->pointToScene(_point);
			p -= QPointF(5, 5);
			setPos(p);
		}

		const QPointF& getPoint() const { return _point; }

		void setPoint(const QPointF& point) {
			_point = point;
			update();
		}

	private:
		QPointF _point;
	};

	struct TreeNode {
		std::array<std::shared_ptr<TreeNode>, 2> _branch;
		CircleItem *_circle;
		QPointF _center;
		bool _fixed = false;
	};

	void convert(TreeNode *node, QJsonObject &object) const;

	void parse(TreeNode *node, int index, const QJsonObject &object);

	void check(TreeNode *node, std::string &path, std::vector<std::string>& result) const;

	CircleItem* addCircle(const QPointF& center, qreal radius);

	void delCircle(const CircleItem* circle);

	KnotItem* addKnot(const QPointF& point);

	void delKnot(const KnotItem* knot);

	void updateKnots();

	void update();

	std::vector<std::shared_ptr<TreeNode>> _treePath;
	std::string _textPath;
	std::shared_ptr<TreeNode> _treeNode;
	std::shared_ptr<TreeNode> _treeRoot;

	std::vector<QGraphicsLineItem*> _lines;

	std::map<int, CircleItem*> _indexToCircle;
	std::set<CircleItem*> _backCircles;
	std::set<CircleItem*> _circles;
	std::set<KnotItem*> _knots;
	CircleItem* _circle = nullptr;
	KnotItem* _knot1 = nullptr;
	KnotItem* _knot2 = nullptr;

	QPointF _prev;

	QPointF _center = {0., 0.};
	qreal _scale = 1.;

	bool _visibleKnots = true;
	bool _filledArea = false;
	Mode _mode = Free;

	Monitor *_monitor = nullptr;
};

#endif //__INCLUDE_GRAPHICSSCENE_H
