#ifndef __INCLUDE_MAINWINDOW_H
#define __INCLUDE_MAINWINDOW_H

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QStringListModel>
#include <QMessageBox>
#include <QMainWindow>
#include <monitor.hpp>

namespace Ui {
class MainWindow;
}

class MainWindow: public QMainWindow, public Monitor {
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	virtual void sendPosition(const QPointF& point, bool fixed) override;
	virtual void sendTreePath(const QString& path, bool fixed) override;
	virtual void sendError(const QString& message) override;

private slots:
	void on_buttonPlace_clicked();

	void on_buttonReset_clicked();
	void on_buttonCheck_clicked();
	void on_buttonMode_clicked();

	void on_buttonFalse_clicked();
	void on_buttonTrue_clicked();
	void on_buttonSavePath_clicked();
	void on_buttonInvert_clicked();
	void on_buttonUp_clicked();
	void on_checkInvert_clicked();
	void on_checkChord_clicked();
	void on_checkPoint_clicked();
	void on_checkFill_clicked();

	void on_buttonPrint_clicked();
	void on_buttonSave_clicked();
	void on_buttonOpen_clicked();

	void on_listView_clicked();

private:
	QStringListModel *listModel;
	Ui::MainWindow *ui;
};

#endif //__INCLUDE_MAINWINDOW_H
