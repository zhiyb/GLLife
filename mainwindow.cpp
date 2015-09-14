#include <QtWidgets>
#include "mainwindow.h"
#include "glwidget.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	QWidget *w = new QWidget(this);
	setCentralWidget(w);
	resize(800, 800);

	QHBoxLayout *hlayout = new QHBoxLayout(w);
	GLWidget *glw = new GLWidget;
	hlayout->addWidget(glw);

	connect(glw, SIGNAL(titleUpdate(QString)), this, SLOT(setWindowTitle(QString)));
}

MainWindow::~MainWindow()
{

}
