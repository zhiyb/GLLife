#include <QDebug>
#include <QOpenGLFramebufferObject>
#include <QFileDialog>
#include <QDialog>
#include <QVBoxLayout>
#include "glwidget.h"

#define RES_SHADER_PFX	":/shaders/"
#define MOVESTEP	10
#define BLOCK_SIZE	1024

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
	zoom = 0;
	move[0] = 0;
	move[1] = 0;
	step = 0;
	framebuffer.def = 0;

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
}

void GLWidget::initializeGL()
{
	initializeOpenGLFunctions();
	qDebug() << format();

	glClearColor(0.125, 0.125, 0.125, 1.);

	// Initialise render to screen program
	shader_info_t render_shaders[] = {
		{GL_VERTEX_SHADER, RES_SHADER_PFX "vertex.vsh"},
		{GL_FRAGMENT_SHADER, RES_SHADER_PFX "render.fsh"},
		{GL_NONE, 0}
	};
	if ((render.program = loadShaders(render_shaders)) == 0)
		qFatal("%s: %d: %s", __FILE__, __LINE__, __PRETTY_FUNCTION__);
	glUseProgram(render.program);

	// Retrive locations
	render.loc.vertex = glGetAttribLocation(render.program, "vertex");
	render.loc.vpSize = glGetUniformLocation(render.program, "vpSize");
	render.loc.texSize = glGetUniformLocation(render.program, "texSize");
	render.loc.zoom = glGetUniformLocation(render.program, "zoom");
	render.loc.move = glGetUniformLocation(render.program, "move");
	glVertexAttribIPointer(render.loc.vertex, 2, GL_INT, 0, 0);
	glEnableVertexAttribArray(render.loc.vertex);

	// Generate vertex arrays
	glGenVertexArrays(1, &render.data.vao);
	glBindVertexArray(render.data.vao);
	GLuint aBuffer;
	GLint vertices[4][2] = {{1, 1}, {-1, 1}, {-1, -1}, {1, -1}};
	glGenBuffers(1, &aBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, aBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Initialise binarization program
	// Using the same set of vertex data as the render program
	bin = render;
	shader_info_t bin_shaders[] = {
		{GL_VERTEX_SHADER, RES_SHADER_PFX "vertex.vsh"},
		{GL_FRAGMENT_SHADER, RES_SHADER_PFX "binarization.fsh"},
		{GL_NONE, 0}
	};
	if ((bin.program = loadShaders(bin_shaders)) == 0)
		qFatal("%s: %d: %s", __FILE__, __LINE__, __PRETTY_FUNCTION__);
	glUseProgram(bin.program);
	glVertexAttribIPointer(bin.loc.vertex, 2, GL_INT, 0, 0);
	glEnableVertexAttribArray(bin.loc.vertex);

	// Initialise iteration program
	// Using the same set of vertex data as the render program
	iteration = render;
	shader_info_t it_shaders[] = {
		{GL_VERTEX_SHADER, RES_SHADER_PFX "vertex.vsh"},
		{GL_FRAGMENT_SHADER, RES_SHADER_PFX "iteration.fsh"},
		{GL_NONE, 0}
	};
	if ((iteration.program = loadShaders(it_shaders)) == 0)
		qFatal("%s: %d: %s", __FILE__, __LINE__, __PRETTY_FUNCTION__);
	glUseProgram(iteration.program);
	glVertexAttribIPointer(iteration.loc.vertex, 2, GL_INT, 0, 0);
	glEnableVertexAttribArray(iteration.loc.vertex);

	// Generate textures
	texture.debug.orig = loadTexture(":/texture.png", &texture.debug.width, &texture.debug.height);
	glGenTextures(1, &texture.debug.bin);
	glBindTexture(GL_TEXTURE_2D, texture.debug.bin);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ZERO);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ZERO);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, texture.debug.width, texture.debug.height, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
	glGenTextures(2, buffer.buffers);
	for (int i = 0; i != 2; i++) {
		if (buffer.buffers[i] == 0) {
			qWarning("Cannot generate texture units for rendering.");
			qFatal("%s: %d: %s", __FILE__, __LINE__, __func__);
		}
		glBindTexture(GL_TEXTURE_2D, buffer.buffers[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, BLOCK_SIZE, BLOCK_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	}

	// Render binarized texture
	glGenFramebuffers(1, &framebuffer.off);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.off);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.debug.bin, 0);

	glViewport(0, 0, texture.debug.width, texture.debug.height);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(bin.program);
	glBindVertexArray(bin.data.vao);
	glBindTexture(GL_TEXTURE_2D, texture.debug.orig);
	glUniform2i(bin.loc.vpSize, texture.debug.width, texture.debug.height);
	glUniform2i(bin.loc.texSize, texture.debug.width, texture.debug.height);
	glUniform1i(bin.loc.zoom, 0);
	glUniform2f(bin.loc.move, 0., 0.);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glFlush();

	// Render initial pattern
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer.current(), 0);
	glViewport(0, 0, BLOCK_SIZE, BLOCK_SIZE);
	glUseProgram(render.program);
	glBindVertexArray(render.data.vao);
	glBindTexture(GL_TEXTURE_2D, texture.debug.bin);
	glUniform2i(render.loc.vpSize, BLOCK_SIZE, BLOCK_SIZE);
	glUniform2i(render.loc.texSize, texture.debug.width, texture.debug.height);
	glUniform1i(render.loc.zoom, 0);
	glUniform2f(render.loc.move, 0., 0.);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glFlush();

	updateTitle();
}

void GLWidget::resizeGL(int w, int h)
{
	// Qt window system fbo cannot be queried in initializeGL
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer.def);
	glViewport(0, 0, w, h);
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

	if (step == 0)	// If no off-screen rendering required
		goto render;

	// Start off-screen rendering (calculations, iterations)
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.off);
	glViewport(0, 0, BLOCK_SIZE, BLOCK_SIZE);

	// Begin iterations
	glUseProgram(iteration.program);
	glBindVertexArray(iteration.data.vao);
	glUniform2i(iteration.loc.vpSize, BLOCK_SIZE, BLOCK_SIZE);
	glUniform2i(iteration.loc.texSize, BLOCK_SIZE, BLOCK_SIZE);
	glUniform1i(render.loc.zoom, 0);
	glUniform2f(render.loc.move, 0., 0.);

	// Execute iteration steps
	for (; step != 0; step--) {
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer.next(), 0);
		glBindTexture(GL_TEXTURE_2D, buffer.current());
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glFlush();
		buffer.swap();
	}

	// Bind to default fbo again
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.def);
	glViewport(0, 0, width(), height());

render:
	glUseProgram(render.program);
	glBindVertexArray(render.data.vao);
	glBindTexture(GL_TEXTURE_2D, buffer.current());
	glUniform2i(render.loc.vpSize, width(), height());
	glUniform2i(render.loc.texSize, BLOCK_SIZE, BLOCK_SIZE);
	glUniform1i(render.loc.zoom, zoom);
	glUniform2f(render.loc.move, move[0], move[1]);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glFlush();

	// Computation time report
	if (!start.isNull()) {
		QTime now(QTime::currentTime()), from(start);
		start = QTime();
		QMessageBox::information(this, tr("GLLife"), tr("Computation takes %1.%2 seconds.").arg(from.secsTo(now)).arg(from.msecsTo(now) % 1000));
	}
}

void GLWidget::wheelEvent(QWheelEvent *e)
{
	zoom += -e->angleDelta().y() / 120;
	updateTitle();
	update();
	QOpenGLWidget::wheelEvent(e);
}

void GLWidget::mousePressEvent(QMouseEvent *e)
{
	prevPos = e->pos();
	e->accept();
	QOpenGLWidget::mousePressEvent(e);
}

void GLWidget::mouseMoveEvent(QMouseEvent *e)
{
	QPointF p = e->pos() - prevPos;
	move[0] += -p.x() * pow(2, zoom);
	move[1] += p.y() * pow(2, zoom);
	prevPos = e->pos();
	updateTitle();
	update();
	e->accept();
	QOpenGLWidget::mouseMoveEvent(e);
}

void GLWidget::keyPressEvent(QKeyEvent *e)
{
	switch (e->key()) {
	case '.':	// Iteration 1 step
		step++;
		break;
	case 's':
	case 'S':
	{
		int steps = QInputDialog::getInt(this, tr("GLLife"), tr("Please specify iteration steps:"), 0, 0);
		if (steps) {
			step += steps;
			if (steps >= 100)
				start = QTime::currentTime();
		}
		break;
	}
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
		pause = !pause;
		break;
	case Qt::Key_Up:
		move[1] += MOVESTEP * pow(2, zoom);
		break;
	case Qt::Key_Down:
		move[1] -= MOVESTEP * pow(2, zoom);
		break;
	case Qt::Key_Left:
		move[0] -= MOVESTEP * pow(2, zoom);
		break;
	case Qt::Key_Right:
		move[0] += MOVESTEP * pow(2, zoom);
		break;
	case '+':	// Zoom in
	case '=':
		zoom--;
		break;
	case '-':	// Zoom out
	case '_':
		zoom++;
		break;
	default:
		goto skip;
	};
	updateTitle();
	update();
	e->accept();
skip:
	QOpenGLWidget::keyPressEvent(e);
}

void GLWidget::updateTitle()
{
	emit titleUpdate(tr("GLLife <(%1, %2) * %3> %4")
			 .arg(move[0]).arg(move[1]).arg(1. / pow(2, zoom))
			 .arg(pause ? tr("[Paused] ") : tr("")));
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

void GLWidget::buffer_t::swap()
{
	GLuint tmp = buffers[0];
	buffers[0] = buffers[1];
	buffers[1] = tmp;
}
