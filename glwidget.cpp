#include <QDebug>
#include <QOpenGLFramebufferObject>
#include <QFileDialog>
#include <QDialog>
#include <QVBoxLayout>
#include "glwidget.h"

#define RES_SHADER_PFX	":/shaders/"
#define ZOOMSTEP	0.25
#define DIM		640

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
	data.zoom = 0;
	data.moveX = 0;
	data.moveY = 0;

	QSurfaceFormat fmt = format();
	fmt.setSamples(0);
	fmt.setVersion(4, 0);
	fmt.setOption(0);
	fmt.setProfile(QSurfaceFormat::CoreProfile);
	fmt.setDepthBufferSize(0);
	fmt.setStencilBufferSize(0);
	fmt.setAlphaBufferSize(0);
	fmt.setSamples(0);
	setFormat(fmt);
	QSurfaceFormat::setDefaultFormat(fmt);
	setFormat(fmt);

	setFocusPolicy(Qt::StrongFocus);
	setAutoFillBackground(false);

	updateTitle();
}

void GLWidget::initializeGL()
{
	initializeOpenGLFunctions();
	qDebug() << format();

	glClearColor(0.0, 0.0, 0.0, 1.0);

	// Initialise render to screen program
	shader_info_t render_shaders[] = {
		{GL_VERTEX_SHADER, RES_SHADER_PFX "vertex.vsh"},
		{GL_FRAGMENT_SHADER, RES_SHADER_PFX "render.fsh"},
		{GL_NONE, 0}
	};
	if ((data.render.program = loadShaders(render_shaders)) == 0) {
		qApp->quit();
		return;
	}
	glUseProgram(data.render.program);

	glGenVertexArrays(1, &data.render.data.vao);
	glBindVertexArray(data.render.data.vao);
	GLint vertices[4][2] = {{1, 1}, {-1, 1}, {-1, -1}, {1, -1}};
	glGenBuffers(1, &data.render.data.aBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, data.render.data.aBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	data.render.loc.vertex = glGetAttribLocation(data.render.program, "vertex");
	data.render.loc.projection = glGetUniformLocation(data.render.program, "projection");
	glVertexAttribIPointer(data.render.loc.vertex, 2, GL_INT, 0, 0);
	glEnableVertexAttribArray(data.render.loc.vertex);

	// Initialise binarization program
	shader_info_t bin_shaders[] = {
		{GL_VERTEX_SHADER, RES_SHADER_PFX "vertex.vsh"},
		{GL_FRAGMENT_SHADER, RES_SHADER_PFX "binarization.fsh"},
		{GL_NONE, 0}
	};
	if ((data.bin.program = loadShaders(bin_shaders)) == 0) {
		qApp->quit();
		return;
	}
	glUseProgram(data.bin.program);

	// Using the same set of data as render program
	data.bin.data.vao = data.render.data.vao;
	data.bin.data.aBuffer = data.render.data.aBuffer;

	data.bin.loc.vertex = glGetAttribLocation(data.bin.program, "vertex");
	data.bin.loc.projection = glGetUniformLocation(data.bin.program, "projection");
	glVertexAttribIPointer(data.bin.loc.vertex, 2, GL_INT, 0, 0);
	glEnableVertexAttribArray(data.bin.loc.vertex);

	// Generate textures
	data.texture.debug.orig = loadTexture(":/texture.png", &data.texture.debug.width, &data.texture.debug.height);
	glGenTextures(1, &data.texture.debug.bin);
	glBindTexture(GL_TEXTURE_2D, data.texture.debug.bin);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, data.texture.debug.width, data.texture.debug.height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	// Render binarized texture
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, data.texture.debug.bin, 0);

	glViewport(0, 0, data.texture.debug.width, data.texture.debug.height);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(data.bin.program);
	glBindVertexArray(data.bin.data.vao);
	glBindTexture(GL_TEXTURE_2D, data.texture.debug.orig);
	glUniformMatrix4fv(data.bin.loc.projection, 1, GL_FALSE, QMatrix4x4().constData());
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glFlush();

	// Bind to default fbo again, and release resources
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &fbo);
}

void GLWidget::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
	float asp = (float)h / (float)w;
	data.projection.setToIdentity();
	data.projection.ortho(-1, 1, -asp, asp, -1, 1);
}

void GLWidget::paintGL()
{
#if 0
	double posX = (data.moveX + 1.) * float(DIM / 2);
	double posY = (data.moveY - 1.) * float(DIM / 2) * -1.;
	glUniform1ui(data.loc.dim, DIM);
	glUniform1f(data.loc.zoom, data.zoom);
	glUniform2f(data.loc.position, posX, posY);
	glVertexAttribPointer(data.loc.vertex, 2, GL_FLOAT, GL_FALSE, 0, data.vertex.constData());

	if (data.loc.animation != -1 && !data.pause) {
		glUniform1f((float)data.loc.animation, QTime::currentTime().msec() / 1000.);
		update();
	}
#endif

	glUseProgram(data.render.program);
	glBindVertexArray(data.render.data.vao);
	glBindTexture(GL_TEXTURE_2D, data.texture.debug.orig);
	glUniformMatrix4fv(data.render.loc.projection, 1, GL_FALSE, data.projection.constData());
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	QMatrix4x4 projection(data.projection);
	projection.translate(1. - 0.25, -(1. - 0.25), 0);
	projection.scale(0.25);
	glUniformMatrix4fv(data.render.loc.projection, 1, GL_FALSE, projection.constData());
	glBindTexture(GL_TEXTURE_2D, data.texture.debug.bin);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glFlush();
}

void GLWidget::wheelEvent(QWheelEvent *e)
{
	float zoom = -(float)e->angleDelta().y() / 120. * ZOOMSTEP;
	data.zoom += zoom;
	updateTitle();
	update();
}

void GLWidget::mouseMoveEvent(QMouseEvent *e)
{
	QPointF p = e->pos() - data.prevPos;
	data.moveX += -p.x() * 2.f * pow(2, data.zoom) / (double)width();
	data.moveY += p.y() * 2.f * pow(2, data.zoom) / (double)width();
	data.prevPos = e->pos();
	updateTitle();
	update();
}

void GLWidget::keyPressEvent(QKeyEvent *e)
{
	const float moveTh = 10;
	switch (e->key()) {
	case 'r':	// Refresh
	case 'R':
		break;
	case 'q':	// Quit
	case 'Q':
	case Qt::Key_Escape:
		qApp->quit();
		return;
	case 'p':	// Pause
	case 'P':
		data.pause = !data.pause;
		break;
	case Qt::Key_Up:
		data.moveY += -moveTh * 2.f * pow(2, data.zoom) / (double)width();
		break;
	case Qt::Key_Down:
		data.moveY += moveTh * 2.f * pow(2, data.zoom) / (double)width();
		break;
	case Qt::Key_Left:
		data.moveX += moveTh * 2.f * pow(2, data.zoom) / (double)width();
		break;
	case Qt::Key_Right:
		data.moveX += -moveTh * 2.f * pow(2, data.zoom) / (double)width();
		break;
	case '+':	// Zoom in
	case '=':
		data.zoom -= ZOOMSTEP;
		break;
	case '-':	// Zoom out
	case '_':
		data.zoom += ZOOMSTEP;
		break;
	};
	updateTitle();
	update();
}

void GLWidget::updateTitle()
{
	emit titleUpdate(tr("GLLife <(%1, %2) * %3> %4")
			 .arg(data.moveX).arg(data.moveY).arg(1. / pow(2, data.zoom))
			 .arg(data.pause ? tr("[Paused] ") : tr("")));
}

GLuint GLWidget::loadShader(GLenum type, const QByteArray& context)
{
	GLuint shader = glCreateShader(type);
	if (shader == 0) {
		qWarning("Cannot create OpenGL shader.");
		return 0;
	}
	const char *p = context.constData();
	GLint length = context.length();
	glShaderSource(shader, 1, &p, &length);
	glCompileShader(shader);

	GLint logLength;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	char log[logLength];
	glGetShaderInfoLog(shader, logLength, &logLength, log);
	qWarning(log);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_TRUE)
		return shader;
	qWarning("Shader compilation failed.");
	glDeleteShader(shader);
	return 0;
}

GLuint GLWidget::loadShaderFile(GLenum type, QString path)
{
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly)) {
		qWarning(QString("Cannot open file %1").arg(path).toLocal8Bit());
		return 0;
	}
	QByteArray context = f.readAll();
	f.close();
	return loadShader(type, context);
}

GLuint GLWidget::loadShaders(GLWidget::shader_info_t *shaders)
{
	GLuint program = glCreateProgram();
	if (program == 0) {
		qWarning("Cannot create OpenGL program.");
		return 0;
	}

	GLsizei count;
	for (count = 0; shaders->type != GL_NONE && shaders->path != 0; shaders++, count++) {
		qDebug(QString("Loading shader source: %1...").arg(shaders->path).toLocal8Bit());
		GLuint shader = loadShaderFile(shaders->type, shaders->path);
		if (shader == 0)
			goto failed;
		glAttachShader(program, shader);
	}
	qDebug("Linking program...");
	glLinkProgram(program);

	{	// Cross initialisation error
		// Get program linking log
		GLint logLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		char log[logLength];
		glGetProgramInfoLog(program, logLength, &logLength, log);
		qWarning(log);
	}

failed:
	// Shaders can be detach and deleted after program linked
	GLuint sh[count], *shp = sh;
	glGetAttachedShaders(program, count, &count, sh);
	while (count--) {
		glDetachShader(program, *shp);
		glDeleteShader(*shp++);
	}

	// Check program linking status
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_TRUE)
		return program;
	glDeleteProgram(program);
	return 0;
}

GLuint GLWidget::loadTexture(QString filepath, GLuint *width, GLuint *height)
{
	GLuint texture;
	QImage img(QImage(filepath).convertToFormat(QImage::Format_RGB888).mirrored());
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, img.width(), img.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, img.constBits());
	if (width)
		*width = img.width();
	if (height)
		*height = img.height();
	return texture;
}
