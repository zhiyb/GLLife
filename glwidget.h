#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QtWidgets>
#include <QOpenGLFunctions_4_0_Core>

#define OPENGL_VERSION_MAJOR	4
#define OPENGL_VERSION_MINOR	0
#define OPENGL_VERSION		"4.0"

#define OPENGL_FUNC(a, b)	QOpenGLFunctions_##a##_##b##_Core
#define OPENGL_FUNCTIONS(a, b)	OPENGL_FUNC(a, b)

class GLWidget : public QOpenGLWidget,
		protected OPENGL_FUNCTIONS(OPENGL_VERSION_MAJOR, OPENGL_VERSION_MINOR)
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
	void timerEvent(QTimerEvent *e);
	void wheelEvent(QWheelEvent *e);
	void mousePressEvent(QMouseEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void keyPressEvent(QKeyEvent *e);

private:
	struct shader_info_t {
		GLenum type;
		const char *path;
	};

	QVector2D mapToTexture(QPoint pos);
	QVector2D mapToTexture(QVector2D vp);
	void renderRegion(QRect region = QRect());
	void drawPoint(QVector2D pos, bool death);
	void updateTitle();
	void setTexSwizzle(GLint a, GLint r, GLint g, GLint b);
	GLuint loadShaderFile(GLenum type, QString path);
	GLuint loadShaders(shader_info_t *shaders);
	GLuint loadTexture(QString filepath, GLuint *width, GLuint *height);

	struct vertex_loc_t {
		GLuint vpSize, texSize, zoom, move;
		GLuint vertex, texVertex;
	};

	struct vertex_data_t {
		GLuint vao;	// Vertex array object
	};

	struct program_t {
		vertex_loc_t loc;
		vertex_data_t data;
		GLuint program;
	};

	GLuint genGenericProgram(program_t *program, const char *fsh);

	program_t bin;		// Binarization
	program_t render;	// Render to screen
	program_t iteration;	// Game of life iteration

	struct texture_t {
		struct debug_t {
			GLuint width, height;
			GLuint orig;
			GLuint bin;
		} demo;

		GLuint point;
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

	struct report_fps_t {
		QTime timer;
		unsigned int counter;
		float fps;
	} reportFPS;

	union intPack_t {
		int64_t i64;
		struct i32Pack_t {
			int32_t x;
			int32_t y;
		} i32;
	};

	struct draw_t {
		bool isEmpty() {return life.isEmpty() && death.isEmpty();}

		QSet<int64_t> life, death;	// Use intPack for conversion
	} draw;

	GLint zoom;
	GLfloat move[2];
	GLuint step;
	QPoint prevPos;
	QTime timer;
	int timerID;

	static const struct vertices_t {
		GLfloat draw[4][2];
		GLint texture[4][2];
	} vertices;
};

#endif // GLWIDGET_H
