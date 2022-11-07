#include <graphicsscene.hpp>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

static std::vector<QPointF> intersect(const QPointF &c1, qreal r1, const QPointF &c2, qreal r2)
{
	qreal x1 = c1.x(), x2 = c2.x(), y1 = c1.y(), y2 = c2.y();
	qreal dx = x2 - x1;
	qreal dy = y2 - y1;
	qreal dq = dx * dx + dy * dy, d = std::sqrt(dq);
	if ((d < std::abs(r2 - r1) + 1.e-7) ||
	    (d > r1 + r2 + 1.e-7)) {
		return {};
	}
	dx = dx / d; dy = dy / d;
	qreal nx = - dy;
	qreal ny = dx;
	qreal a = (r1 * r1 - r2 * r2 + dq) / (2 * d);
	qreal h = std::sqrt(r1 * r1 - a * a);
	qreal x0 = x1 + a * dx;
	qreal y0 = y1 + a * dy;
	x1 = x0 + h * nx;
	y1 = y0 + h * ny;
	x2 = x0 - h * nx;
	y2 = y0 - h * ny;
	if (std::abs(h) > 1.e-7) {
		return {
			QPointF(x1, y1),
			QPointF(x2, y2)
		};
	}
	return {
		QPointF(x1, y1)
	};
}

static std::vector<QPointF> place(const QPointF &p1, const QPointF &p2, qreal r)
{
	qreal x1 = p1.x(), x2 = p2.x(), y1 = p1.y(), y2 = p2.y();
	qreal x0 = (x1 + x2) / 2;
	qreal y0 = (y1 + y2) / 2;
	qreal dx = x2 - x1;
	qreal dy = y2 - y1;
	qreal dq = dx * dx + dy * dy;
	qreal d = std::sqrt(dq);
	dx = dx / d; dy = dy / d;
	qreal nx = - dy;
	qreal ny = dx;
	qreal h = r * r - dq / 4;
	if (h < 1.e-7) return {};
	h = std::sqrt(h);
	x0 += h * nx;
	y0 += h * ny;
	return {
		QPointF(x0, y0)
	};
}

const std::vector<QColor> GraphicsScene::CircleItem::_colors = {
    QColor(  0,   0,   0),
    QColor(255,   0,   0),
    QColor(255, 127,   0),
    QColor(255, 255,   0),
    QColor(  0, 255,   0),
    QColor(  0, 255, 255),
    QColor(  0,   0, 255),
    QColor(255,   0, 255)
};

GraphicsScene::GraphicsScene(QObject *parent):
    QGraphicsScene(parent) {
}

GraphicsScene::~GraphicsScene()
{
}

QPointF GraphicsScene::pointFromScene(const QPointF& point) const
{
	auto p = (point - _center) / _scale;
	return QPointF(p.x(), -p.y());
}

QPointF GraphicsScene::pointToScene(const QPointF& point) const
{
	QPointF p(point.x(), -point.y());
	return _center + _scale * p;
}

void GraphicsScene::parse(TreeNode *node, int index, const QJsonObject &object)
{
	auto center = object["center"].toArray();
	auto x = center[0].toDouble();
	auto y = center[1].toDouble();
	node->_center = QPointF(x, y);
	node->_circle = _indexToCircle.at(index);
	auto branch = object["branch"].toArray();
	for (int i = 0; i < std::min(2, int(branch.size())); ++i) {
		auto obj = branch[i].toObject();
		if (!obj.empty()) {
			auto next = std::make_shared<TreeNode>();
			parse(next.get(), index + 1, obj);
			node->_branch[i] = next;
			next->_fixed = true;
		}
	}
	node->_fixed = true;
}

bool GraphicsScene::loadFromFile(QFile *file)
{
	QByteArray data = file->readAll();

	QJsonDocument doc(QJsonDocument::fromJson(data));
	clear();
	QJsonObject obj = doc.object();
	QJsonArray rad = obj["radius"].toArray();
	for (auto r: rad) {
		addCircle({0., 0.}, r.toDouble());
	}
	obj = obj["search"].toObject();
	_treeRoot = std::make_shared<TreeNode>();
	parse(_treeRoot.get(), 1, obj);
	if (_mode == Mode::Free)
		_mode = Mode::Tree;
	start();

	return true;
}

void GraphicsScene::convert(TreeNode *node, QJsonObject &object) const
{
	object["center"] = QJsonArray(
	    {node->_center.x(), node->_center.y()}
	);
	std::array<QJsonObject, 2> branch;
	for (int i = 0; i < 2; ++i) {
		auto *next = node->_branch[i].get();
		if (next) {
			convert(next, branch[i]);
		}
	}
	object["branch"] = QJsonArray(
	    {branch[0], branch[1]}
	);
}

bool GraphicsScene::saveToFile(QFile *file) const
{
	if (!_treeRoot) return false;

	QJsonArray radius;
	for (auto [index, circle] : _indexToCircle) {
		radius.append(circle->getRadius());
	}
	QJsonObject object;
	object["radius"] = radius;
	QJsonObject branch;
	convert(
	    _treeRoot.get(),
	    branch
	);
	object["search"] =
	    branch;

	file->write(
	    QJsonDocument(object).toJson(
	    QJsonDocument::Compact
	    )
	);
	return true;
}

void GraphicsScene::check(TreeNode *node, std::string &path, std::vector<std::string> &result) const
{
	if (!node || (path.size() == _circles.size() - 2)) return;

	for (int i = 0; i < 2; ++i) {
		path += '0' + i;
		auto* next = node->_branch[i].get();
		if (next) {
			check(next, path, result);
		}
		else {
			int n = _circles.size() - 2;
			result.push_back(path);
			auto &s = result.back();
			s.resize(n, 'x');
		}
		path.pop_back();
	}
}

void GraphicsScene::check(std::vector<std::string> &result) const
{
	result.clear();
	if (!_treeRoot) return;

	auto node = _treeRoot;
	std::string path;
	check(
	node.get(), path,
	result
	);
}

bool GraphicsScene::test(const QPointF &point)
{
	if (_mode != Mode::Test) return false;

	if (_indexToCircle.empty() || !_indexToCircle.at(0)->containsPoint(point)) {
		return false;
	}
	_knot1 = addKnot(point);
	_knot2 = nullptr;
	updateKnots();
	start();

	CircleItem *circ = nullptr;
	_treeNode = _treeRoot;
	while (_treeNode) {
		auto center = _treeNode->_center; circ = _treeNode->_circle;
		circ->setCenter(center);
		int ans = circ->containsPoint(point);
		circ->setVisible(true);
		circ->setOpaque(ans);
		if (!_treeNode->_branch[ans] ||
		    !goToNext(ans)) {
			break;
		}
	}
	if (!circ ||
	    (circ->getIndex() < _circles.size() - 1) ||
	    !circ->containsPoint(point)) {
		if (_monitor)
			_monitor->sendError(
			"Bad solution!"
			);
		return false;
	}
	return true;
}

void GraphicsScene::setMode(Mode mode)
{
	_circle = nullptr;
	_knot1 = nullptr;
	_knot2 = nullptr;
	if ((_mode == Mode::Test) && (mode == Mode::Tree)) {
		_mode = mode;
		updateKnots();
		return;
	}
	_mode = mode;
	start();
}

void GraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	//...
}

void GraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	//Производим захват объекта
	const auto &scenePos = mouseEvent->scenePos();
	auto point = pointFromScene(scenePos);
	if (_mode != Mode::Test) {
		auto *item = itemAt(scenePos.x(), scenePos.y(), QTransform());
		auto *circle = dynamic_cast<CircleItem*>(item);
		auto *knot = dynamic_cast<KnotItem*>(item);
		if (circle && circle->isEnabled()) {
			_circle = circle;
			if (_knot1 && _knot2) {
				_knot1 = nullptr;
				_knot2 = nullptr;
			}
			_prev = point;
			update();
			return;
		}
		if (knot) {
			if (_knot2 == knot) {
				_knot2 = nullptr;
			}
			else
			if (_knot1 == knot) {
				_knot1 = _knot2;
				_knot2 = nullptr;
			}
			else
			if (_knot2) {
				_knot1 = knot;
				_knot2 = nullptr;
			}
			else
			if (_knot1) {
				_knot2 = knot;
			}
			else {
				_knot1 = knot;
			}
			updateKnots();
			return;
		}
		_circle = nullptr;
		_knot1 = nullptr;
		_knot2 = nullptr;
		updateKnots();
	}
	else {
		test(point);
	}
	if (_monitor) _monitor->sendPosition(point, true);
}

void GraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	if (_monitor) {
		auto point = pointFromScene(mouseEvent->scenePos());
		_monitor->sendPosition(point, false);
	}
	if (_mode == Mode::Test) return;

	if (_circle) {
		auto point = pointFromScene(mouseEvent->scenePos());
		auto off = point - _prev;
		_circle->setCenter(_circle->getCenter() + off);
		_prev = point;
		if (_knot1 && !_knot2) {
			auto p1 = _knot1->getPoint();
			auto c = _circle->getCenter();
			auto r = _circle->getRadius();
			qreal dx = c.x() - p1.x();
			qreal dy = c.y() - p1.y();
			qreal d = std::sqrt(dx * dx + dy * dy);
			dx /= d; dy /= d;
			c = QPointF(p1.x() + dx * r,
			        p1.y() + dy * r);
			_circle->setCenter(c);
		}
		updateKnots();
	}
}

void GraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	_circle = nullptr;
	update();
}

void GraphicsScene::wheelEvent(QGraphicsSceneWheelEvent *wheelEvent)
{
	constexpr qreal factor = 1.03125;

	//Обновляем ЛСК
	qreal f = ((wheelEvent->delta() < 0)? (1. / factor): factor);

	_center = (1 - f) * wheelEvent->scenePos() + f * _center;
	_scale *= f;

	update();
}

GraphicsScene::CircleItem* GraphicsScene::addCircle(const QPointF& center, qreal radius)
{
	auto *circle = new CircleItem(_circles.size());
	_indexToCircle[circle->getIndex()] = circle;
	_circles.insert(circle);
	addItem(circle);

	circle->setCenter(center);
	circle->setRadius(radius);

	return circle;
}

void GraphicsScene::delCircle(const CircleItem* circle)
{
	_indexToCircle.erase(circle->getIndex());
	_circles.erase(circle);
	removeItem(circle);
	delete circle;
}

GraphicsScene::KnotItem* GraphicsScene::addKnot(const QPointF &point)
{
	auto *knot = new KnotItem();
	_knots.insert(knot);
	addItem(knot);

	knot->setPoint(point);
	knot->setZValue(4.);
	knot->setBrush(QBrush(QColor(255, 255, 255)));
	QPen pen(QColor(0, 0, 0)); pen.setWidth(2);
	knot->setPen(pen);

	return knot;
}

void GraphicsScene::delKnot(const KnotItem* knot)
{
	this->removeItem(knot);
	_knots.erase(knot);
	delete knot;
}

void GraphicsScene::updateKnots()
{
	std::vector<KnotItem*> knotsToDelete;
	knotsToDelete.reserve(_knots.size());
	for (const auto &knot : _knots) {
		if ((knot != _knot1) && (knot != _knot2)) {
			knotsToDelete.push_back(knot);
		}
	}
	for (const auto &knot: knotsToDelete) {
		_knots.erase(knot);
		removeItem(knot);
		delete knot;
	}
	if (_mode == Mode::Test) {
		update();
		return;
	}

	for (auto it1 = _circles.begin(); it1 != _circles.end(); ++it1) {
		if (!(*it1)->isVisible()) continue;	//Пропускаем скрытые окружности!
		const auto& c1 = (*it1)->getCenter();
		qreal r1 = (*it1)->getRadius();
		for (auto it2 = _circles.begin(); it2 != it1; ++it2) {
			if (!(*it2)->isVisible()) continue;	//Пропускаем скрытые окружности!
			const auto& c2 = (*it2)->getCenter();
			qreal r2 = (*it2)->getRadius();
			auto res = intersect(c1, r1, c2, r2);
			for (const auto& p: res) {
				if (_knot1) {
					const auto &p1 = _knot1->getPoint();
					qreal dx1 = std::abs(p.x() - p1.x());
					qreal dy1 = std::abs(p.y() - p1.y());
					if (std::max(dx1, dy1) < 1.e-7) {
						continue;
					}
				}
				if (_knot2) {
					const auto &p2 = _knot2->getPoint();
					qreal dx2 = std::abs(p.x() - p2.x());
					qreal dy2 = std::abs(p.y() - p2.y());
					if (std::max(dx2, dy2) < 1.e-7) {
						continue;
					}
				}
				addKnot(p);
			}
		}
	}
	update();
}

void GraphicsScene::update()
{
	//Обновляем текстовый путь в дереве
	if (_monitor) {
		_monitor->sendTreePath(_textPath.c_str(), _treeNode && _treeNode->_fixed);
	}

	//Удаляем вспомогательные линии
	for (const auto& line : _lines) { removeItem(line); delete line; }
	_lines.clear();

	//Строим координатную сетку
	constexpr qreal step = 1.;
	qreal min = -2.;
	qreal max = +2.;
	QPen pen(QColor(127, 127, 127));
	pen.setDashPattern({1., 8.});
	for (int i = 0; ; ++i) {
		qreal v = min + i * step; if (v > max) break;
		QPointF px1(v, min), px2(v, max);
		QPointF py1(min, v), py2(max, v);
		auto *lx = new QGraphicsLineItem(
		    QLineF(
		        pointToScene(px1),
		        pointToScene(px2)
		    )
		);
		auto *ly = new QGraphicsLineItem(
		    QLineF(
		        pointToScene(py1),
		        pointToScene(py2)
		    )
		);
		_lines.push_back(lx);
		_lines.push_back(ly);
		lx->setPen(pen);
		ly->setPen(pen);
		addItem(lx);
		addItem(ly);
	}

	//Строим опорную линию
	if (_knot1 && _knot2) {
		const auto &p1 = _knot1->getPoint();
		const auto &p2 = _knot2->getPoint();
		auto *ll = new QGraphicsLineItem(
		    QLineF(
		        pointToScene(p1),
		        pointToScene(p2)
		    )
		);
		pen.setDashPattern({1.});
		pen.setWidth(2);
		_lines.push_back(ll);
		ll->setPen(pen);
		addItem(ll);
	}

	//Обновляем элементы
	if (_treeNode) {
		for (int i = 0; i < _textPath.size(); ++ i) {
			auto circle = _indexToCircle.at(i + 1);
			if (_textPath[i] == '0') {
				circle->setOpaque(false);
				circle->setFilled(_filledArea);
			}
			else {
				circle->setOpaque(true);
				circle->setFilled(0);
			}
		}
	}
	else {
		for (const auto& circle : _circles) {
			circle->setOpaque(!_circle);
			circle->setFilled(false);
		}
		if (_circle) _circle->setOpaque(true);
	}
	for (const auto& circle : _circles) {
		circle->update();
	}
	if (_treeNode) {
		_treeNode->_center = _treeNode->_circle->getCenter();
	}
	//TODO: Перенести логику с раскрашиванием в класс узла!
	//...
	for (const auto& knot : _knots) {
		const auto& point = knot->getPoint();
		QBrush b = knot->brush();
		if (!_circle ||
		    !_circle->containsPoint(point)) {
			b.setColor(QColor(255, 255, 255));
		}
		else {
			b.setColor(QColor(255, 0, 0));
		}
		knot->setBrush(b);
		knot->update();
	}
	for (const auto& knot : { _knot1, _knot2}) {
		if (!knot) continue;
		QBrush b = knot->brush();
		b.setColor(QColor(0, 0, 255));
		knot->setBrush(b);
	}

}

bool GraphicsScene::placeToPoint(const QPointF &pos)
{
	if (_mode != Mode::Tree) return false;

	_treeNode->_circle->setCenter(pos);
	updateKnots();

	return true;
}

bool GraphicsScene::placeToLocal(const QPointF &loc, bool inv)
{
	if (_mode != Mode::Tree || !_knot1 || !_knot2) return false;

	const auto &p1 = _knot1->getPoint(), &p2 = _knot2->getPoint();
	qreal dx = p2.x() - p1.x(), dy = p2.y() - p1.y();
	qreal d = std::sqrt(dx * dx + dy * dy); dx /= d; dy /= d;
	qreal nx = -dy, ny = dx;
	if (inv) {
		nx *= -1; ny *= -1;
	}
	qreal x0 = (p1.x() + p2.x()) / 2.;
	qreal y0 = (p1.y() + p2.y()) / 2.;
	x0 += loc.x() * dx + loc.y() * nx;
	y0 += loc.x() * dy + loc.y() * ny;

	_treeNode->_circle->setCenter(
	QPointF(x0, y0)
	);
	updateKnots();
	return true;
}

bool GraphicsScene::placeToChord(bool inv)
{
	if (_mode != Mode::Tree || !_knot1 && !_knot2) return false;

	auto *c = _treeNode->_circle; qreal r = c->getRadius();
	auto p1 = _knot1->getPoint();
	if (_knot2) {
		auto p2 = _knot2->getPoint();
		if (inv) std::swap(p1, p2);
		auto res = place(p1, p2, r);
		if (res.empty()) {
			return false;
		}
		c->setCenter(res.front());
	}
	else {
		c->setCenter(p1);
	}
	updateKnots();
	return true;
}

bool GraphicsScene::goToBack()
{
	if (_treePath.empty()) return false;

	const auto prev = _treePath.back();
	_treePath.pop_back();

	auto *circle = _treeNode->_circle;
	_treeNode->_center =
	        circle->getCenter();
	circle->setVisible(false);

	circle = prev->_circle;
	circle->setEnabled(true);
	circle->setCenter(
	    prev->_center
	);
	_treeNode = prev;

	_textPath[_treePath.size()] = 'x';

	if (_mode != Mode::Test) {
	    _knot1 = nullptr;
	    _knot2 = nullptr;
	}
	updateKnots();

	return true;
}

bool GraphicsScene::goToNext(bool ans)
{
	if (!_treeNode || _treePath.size() >= _circles.size() - 2)
		return false;

	auto next = _treeNode->_branch[ans];
	if (!next) {
		next = std::make_shared<TreeNode>();
	}

	auto* circle = _treeNode->_circle;
	int index = circle->getIndex() + 1;
	_treeNode->_center =
	        circle->getCenter();
	circle->setEnabled(false);
	circle->setVisible(true);

	auto it = _indexToCircle.find(index);
	if (it == _indexToCircle.end()) {
		return false;
	}

	circle = next->_circle = it->second;
	circle->setCenter(next->_center);
	circle->setEnabled(true);
	circle->setVisible(true);
	_treePath.push_back(_treeNode);
	_treeNode = next;

	_textPath[index - 2] = '0' + ans;

	if (_mode != Mode::Test) {
		_knot1 = nullptr;
		_knot2 = nullptr;
	}
	updateKnots();

	return true;
}

bool GraphicsScene::goToInv()
{
	if (!_treeNode) return false;

	int ans = _textPath[_treePath.size() - 1] == '1';

	if (!goToBack()) return false;

	return goToNext(!ans);
}


bool GraphicsScene::goToPath(const std::string &path)
{
	if (!_treeRoot) return false;
	start();
	for (char c: path) {
		if (c != '0' && c != '1') continue;
		if (!goToNext(c == '1')) {
			return false;
		}
	}
	return true;
}

void GraphicsScene::savePath()
{
	if (_treeNode == nullptr) return;

	for (int i = 1; i < _treePath.size(); ++ i) {
		auto prev = _treePath[i - 1];
		auto node = _treePath[i];
		int ans = (_textPath[i - 1] == '1');
		if (!prev->_branch[ans]) {
			prev->_branch[ans] = node;
		}
	}
	_treeNode->_fixed = true;
	for (auto& node : _treePath) {
		node->_fixed = true;
	}
	if (_treePath.empty()) {
		update();
		return;
	}
	auto prev = _treePath.back();
	int len = _treePath.size();
	int ans = (_textPath[len - 1] == '1');
	prev->_branch[ans] =
	    _treeNode;

	update();
}

void GraphicsScene::clear()
{
	while (!_circles.empty()) delCircle(*_circles.begin());
	_textPath = "";
	_treePath.clear();
	_treeRoot.reset();
	_treeNode.reset();
	_circle = nullptr;
	_knot1 = nullptr;
	_knot2 = nullptr;
	updateKnots();
}

void GraphicsScene::start()
{
	_textPath = std::string(_circles.size() - 2, 'x');

	if (_mode == Mode::Tree || !_treeRoot) {
		if (!_treeRoot) {
			_treeRoot = std::make_shared<TreeNode>();
			auto circle = _indexToCircle.at(1);
			_treeRoot->_circle = circle;
			_treeRoot->_center = circle->getCenter();
		}
		auto circle = _treeRoot->_circle;
		_treeNode = _treeRoot;
		circle->setCenter(
		_treeNode->_center
		);
		_treePath.clear();
		for (const auto& c : _circles) {
			if (c->getIndex() > 1) {
				c->setEnabled(false);
				c->setVisible(false);
			}
			if (c->getIndex() < 1) {
				c->setEnabled(false);
				c->setVisible(true);
			}
		}
		circle->setEnabled(true);
		circle->setVisible(true);
	}
	if (_mode == Mode::Test) {
		for (const auto& c : _circles) {
			if (c->getIndex() > 0) {
				c->setVisible(false);
			}
			c->setEnabled(false);
		}
		auto circle = _treeRoot->_circle;
		_treeNode = _treeRoot;
		circle->setCenter(
		_treeNode->_center
		);
		_treePath.clear();
	}

	updateKnots();
}

void GraphicsScene::reset()
{
	if (!_treeNode || (_mode != Mode::Tree)) return;

	if (_treeNode != _treeRoot) {
		bool ans =
		_textPath[_treePath.size()-1] == '1';
		goToBack();
		_treeNode->_branch[ans].reset();
		goToNext(ans);
		updateKnots();
		return;
	}

	_treeRoot->_circle->setCenter(
	    {0., 0.}
	);
	_treeRoot.reset();
	start();
}

void GraphicsScene::init()
{
	const auto& rect = this->sceneRect();
	_center = rect.center();
	_scale = std::max(
	    rect.width(), rect.height()
	) * 2.;

	//Создаем базовый набор окружностей
	std::array<qreal, 9> radius = {
		1.0, 0.9, 0.8, 0.7, 0.6,
		0.5, 0.4, 0.3, 0.2
	};
	for (qreal r : radius) {
		addCircle(
		{0., 0.}, r
		);
	}

	_mode = Mode::Free;

	update();
}
