#include "ui_mainwindow.h"
#include <mainwindow.hpp>

#include <graphicsscene.hpp>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QColor>
#include <iostream>
#include <set>

using Mode = GraphicsScene::Mode;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {
	ui->setupUi(this);
	ui->graphicsView->getScene()->setMonitor(this);

	this->listModel = new QStringListModel(this);
	ui->listView->setModel(listModel);
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::sendPosition(const QPointF &point, bool fixed)
{
	QString strX = QString::number(point.x()), strY = QString::number(point.y());
	if (!fixed) {
		ui->label->setText("(" + strX + ", " + strY + ")");
	}
	else {
		ui->editX->setText(strX);
		ui->editY->setText(strY);
	}
}

void MainWindow::sendTreePath(const QString &path, bool fixed)
{
	QPalette palette;
	palette.setColor(QPalette::WindowText, fixed? Qt::black: Qt::red);
	ui->labelTreePath->setPalette(palette);
	ui->labelTreePath->setText(path);
}

void MainWindow::sendError(const QString &message)
{
	QMessageBox messageBox;
	messageBox.setFixedSize(500, 200);
	messageBox.critical(
	0, "Error",
	message
	);
}

void MainWindow::on_buttonPrint_clicked()
{
	QGraphicsView *view = ui->graphicsView;

	QImage image(view->width(), view->height(), QImage::Format_ARGB32);
	image.fill(Qt::white);
	QPainter painter(&image);
	view->render(&painter);
	image.save("print.png");
}

void MainWindow::on_buttonSave_clicked()
{
	QString fileName = QFileDialog::getSaveFileName(this);
	if (fileName.isEmpty()) return;
	QFile file(fileName);
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	ui->graphicsView->getScene()->saveToFile(&file);
}

void MainWindow::on_buttonOpen_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this);
	if (fileName.isEmpty()) return;
	QFile file(fileName);
	file.open(QIODevice::ReadOnly | QIODevice::Text);
	ui->graphicsView->getScene()->loadFromFile(&file);

	listModel->removeRows(0, listModel->rowCount());
}

void MainWindow::on_buttonPlace_clicked()
{
	GraphicsScene *scene = ui->graphicsView->getScene();
	const bool inv = ui->checkInvert->isChecked();
	const bool chr = ui->checkChord->isChecked();
	const bool dot = ui->checkPoint->isChecked();
	QPointF point;
	if (dot) {
		bool ok1; qreal x = ui->editX->text().toDouble(&ok1);
		bool ok2; qreal y = ui->editY->text().toDouble(&ok2);
		if (!ok1 || !ok2) {
			sendError("Incorrect format!");
			return;
		}
		point = QPointF(x, y);
	}
	if (scene->getMode() != GraphicsScene::Mode::Test) {
		if (dot) {
			bool ok = chr?
			          scene->placeToLocal(point, inv):
			          scene->placeToPoint(point);
			if (!ok) {
				sendError(
				"Operation failed!"
				);
			}
		}
		else {
			if (!scene->placeToChord(inv)) {
				sendError(
				"Operation failed!"
				);
			}
		}
		return;
	}
	if (dot) {
		scene->test(point);
	}
}

void MainWindow::on_buttonCheck_clicked()
{
	std::vector<std::string> result;
	ui->graphicsView->getScene()->check(result);
	QStringList list;
	for (const auto& r: result) {
		list.append(r.c_str());
	}
	listModel->setStringList(
	    list
	);
}

void MainWindow::on_listView_clicked()
{
	QModelIndex index = ui->listView->currentIndex();
	QString text = index.data(Qt::DisplayRole).toString();
	auto path = text.toStdString();
	ui->graphicsView->getScene()->goToPath(path);
}

void MainWindow::on_buttonReset_clicked()
{
	listModel->removeRows(0, listModel->rowCount());
	ui->graphicsView->getScene()->reset();
}

void MainWindow::on_buttonMode_clicked()
{
	auto mode = ui->graphicsView->getScene()->getMode();
	if (mode == GraphicsScene::Mode::Test) {
		ui->graphicsView->getScene()->setMode(
		    GraphicsScene::Mode::Tree
		);
		ui->buttonMode->setText(
		    "Start");
	}
	else {
		ui->graphicsView->getScene()->setMode(
		    GraphicsScene::Mode::Test
		);
		ui->buttonMode->setText(
		    "Stop");
	}
}

void MainWindow::on_buttonSavePath_clicked()
{
	ui->graphicsView->getScene()->savePath();
}

void MainWindow::on_buttonFalse_clicked()
{
	auto *scene = ui->graphicsView->getScene();
	auto mode = scene->getMode();
	if (mode != Mode::Tree) {
		scene->setMode(Mode::Tree);
	}
	scene->goToNext(false);
}

void MainWindow::on_buttonTrue_clicked()
{
	auto *scene = ui->graphicsView->getScene();
	auto mode = scene->getMode();
	if (mode != Mode::Tree) {
		on_buttonMode_clicked();
	}
	scene->goToNext(true);
}

void MainWindow::on_buttonUp_clicked()
{
	auto *scene = ui->graphicsView->getScene();
	auto mode = scene->getMode();
	if (mode != Mode::Tree) {
		on_buttonMode_clicked();
	}
	scene->goToBack();
}

void MainWindow::on_buttonInvert_clicked()
{
	auto *scene = ui->graphicsView->getScene();
	auto mode = scene->getMode();
	if (mode != Mode::Tree) {
		on_buttonMode_clicked();
	}
	scene->goToInv();
}

void MainWindow::on_checkInvert_clicked()
{
}

void MainWindow::on_checkChord_clicked()
{
	if (!ui->checkPoint->isChecked()) {
		ui->checkChord->setChecked(true);
	}
	ui->checkInvert->setEnabled(
	ui->checkChord->isChecked()
	);
}

void MainWindow::on_checkPoint_clicked()
{
	const bool ok = ui->checkPoint->isChecked();
	ui->checkInvert->setEnabled(!ok);
	ui->checkChord->setChecked(!ok);
	ui->editX->setEnabled(ok);
	ui->editY->setEnabled(ok);
}

void MainWindow::on_checkFill_clicked()
{
	ui->graphicsView->getScene()->setFilledArea(
	ui->checkFill->isChecked()
	);
}
