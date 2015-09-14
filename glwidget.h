#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QtWidgets>
#include <QOpenGLFunctions_4_0_Core>

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_0_Core
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
	void mousePressEvent(QMouseEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void keyPressEvent(QKeyEvent *e);

private:
	struct shader_info_t {
		GLenum type;
		const char *path;
	};

	void updateTitle();
	GLuint loadShader(GLenum type, const QByteArray& context);
	GLuint loadShaderFile(GLenum type, QString path);
	GLuint loadShaders(shader_info_t *shaders);
	GLuint loadTexture(QString filepath, GLuint *width, GLuint *height);

	struct bin_t {
		struct loc_t {
			GLint vertex, width, projection;
		} loc;
		struct data_t {
			GLuint vao, aBuffer;
		} data;
		GLuint buffer;
		GLuint program;
	} bin;		// Binarization

	struct render_t {
		struct loc_t {
			GLint vertex, width, projection;
		} loc;
		struct data_t {
			GLuint vao, aBuffer;
		} data;
		GLuint buffer;
		GLuint program;
	} render;	// Render to screen

	struct texture_t {
		struct debug_t {
			GLuint width, height;
			GLuint orig;
			GLuint bin;
		} debug;
	} texture;

	bool pause;
	GLfloat zoom;
	GLdouble moveX, moveY;
	QPoint prevPos;
	QMatrix4x4 projection;
};

#endif // GLWIDGET_H
