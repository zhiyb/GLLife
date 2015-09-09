#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QtWidgets>
#include <QOpenGLFunctions_3_3_Core>

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
	Q_OBJECT
public:
	explicit GLWidget(QWidget *parent = 0);

signals:
	void titleUpdate(QString str);

protected:
	void initializeGL();
	void resizeGL(int w, int h);
	void paintGL();
	void wheelEvent(QWheelEvent *e);
	void mousePressEvent(QMouseEvent *e) {data.prevPos = e->pos();}
	void mouseMoveEvent(QMouseEvent *e);
	void keyPressEvent(QKeyEvent *e);

private:
	void updateTitle();
	GLuint loadShader(GLenum type, const QByteArray& context);
	GLuint loadShaderFile(GLenum type, QString path);

	struct data_t {
		struct loc_t {
			GLint vertex, projection;
			GLint zoom, position;
			GLint dim, animation;
		} loc;
		bool pause;
		GLuint program, fsh, vsh;
		GLfloat zoom;
		GLdouble moveX, moveY;
		QPoint prevPos;
		QVector<QVector2D> vertex;
		QMatrix4x4 projection;
	} data;
};

#endif // GLWIDGET_H
