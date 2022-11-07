#ifndef __INCLUDE_MONITOR_H
#define __INCLUDE_MONITOR_H

#include <QPointF>

class Monitor {
public:
	virtual void sendPosition(const QPointF& point, bool fixed) = 0;

	virtual void sendTreePath(const QString& path, bool fixed) = 0;

	virtual void sendError(const QString& message) = 0;

	//...

	virtual ~Monitor() {}
};

#endif //__INCLUDE_MONITOR_H
