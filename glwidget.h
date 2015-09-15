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

private:
	void renderBinarized(GLuint texture, GLuint x, GLuint y);

protected:
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

	struct vertex_loc_t {
		GLuint vpSize, texSize, zoom, move;
		GLuint vertex;
	};

	struct vertex_data_t {
		GLuint vao;	// Vertex array object
		GLuint ubo;	// Uniform buffer object
	};

	struct program_t {
		vertex_loc_t loc;
		vertex_data_t data;
		GLuint program;
	};

	program_t bin;		// Binarization
	program_t render;	// Render to screen
	program_t iteration;	// Game of life iteration

	struct texture_t {
		struct debug_t {
			GLuint width, height;
			GLuint orig;
			GLuint bin;
		} debug;
	} texture;

	struct buffer_t {
		void swap();
		GLuint current() {return buffers[0];}
		GLuint next() {return buffers[1];}

		GLuint buffers[2];
	} buffer;

	struct framebuffer_t {
		GLint def;	// Default rendering framebuffer (window system)
		GLuint off;	// Off-screen rendering framebuffer (computation)
	} framebuffer;

	bool pause;
	GLint zoom;
	GLfloat move[2];
	GLuint step;
	QPoint prevPos;
	QTime start;
};

#endif // GLWIDGET_H
