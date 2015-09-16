#include <QDebug>
#include <QOpenGLFramebufferObject>
#include <QFileDialog>
#include <QDialog>
#include <QVBoxLayout>
#include "glwidget.h"

#define RES_SHADER_PFX	":/shaders/"
#define MOVESTEP	10
#define BLOCK_SIZE_W	1024
#define BLOCK_SIZE_H	1024
#define REPORT_FPS_ITVL	2000
#define FATAL_ERROR()	qFatal("%s: %d: %s", __FILE__, __LINE__, __PRETTY_FUNCTION__)

const GLWidget::vertices_t GLWidget::vertices = {
	{{1., 1.}, {-1., 1.}, {-1., -1.}, {1., -1.}},	// Drawing vertex
	{{1, 1}, {0, 1}, {0, 0}, {1, 0}},		// Texture vertex
};

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
	zoom = 0;
	move[0] = 0;
	move[1] = 0;
	step = 0;
	framebuffer.def = 0;
	timerID = 0;

	QSurfaceFormat fmt = format();
	fmt.setSamples(0);
	fmt.setVersion(OPENGL_VERSION_MAJOR, OPENGL_VERSION_MINOR);
	fmt.setOption(0);
	fmt.setProfile(QSurfaceFormat::CoreProfile);
	fmt.setDepthBufferSize(0);
	fmt.setStencilBufferSize(0);
	fmt.setAlphaBufferSize(0);
	fmt.setSamples(0);
	fmt.setSwapBehavior(QSurfaceFormat::SingleBuffer);
	setFormat(fmt);
	QSurfaceFormat::setDefaultFormat(fmt);
	setFormat(fmt);

	setFocusPolicy(Qt::StrongFocus);
	setAutoFillBackground(false);
}

void GLWidget::initializeGL()
{
	initializeOpenGLFunctions();
	QPair<int, int> version = format().version();
	if (version.first < OPENGL_VERSION_MAJOR ||
			(version.first == OPENGL_VERSION_MAJOR && version.second < OPENGL_VERSION_MINOR)) {
		qWarning("OpenGL version " OPENGL_VERSION " not available.");
		FATAL_ERROR();
	}

	glClearColor(0.125, 0.125, 0.125, 1.);

	// Generate vertex arrays
	vertex_data_t data;
	glGenVertexArrays(1, &data.vao);
	glBindVertexArray(data.vao);
	GLuint aBuffer;
	glGenBuffers(1, &aBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, aBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

	// Initialise render to screen program
	if (!genGenericProgram(&render, RES_SHADER_PFX "render.fsh"))
		FATAL_ERROR();
	render.data = data;

	// Initialise binarization program
	if (!genGenericProgram(&bin, RES_SHADER_PFX "binarization.fsh"))
		FATAL_ERROR();
	bin.data = data;

	// Initialise iteration program
	if (!genGenericProgram(&iteration, RES_SHADER_PFX "iteration.fsh"))
		FATAL_ERROR();
	iteration.data = data;

	// Generate demo textures
	texture.demo.orig = loadTexture(":/texture.png", &texture.demo.width, &texture.demo.height);
	glGenTextures(1, &texture.demo.bin);
	glBindTexture(GL_TEXTURE_2D, texture.demo.bin);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	setTexSwizzle(GL_ONE, GL_ZERO, GL_RED, GL_ZERO);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, texture.demo.width, texture.demo.height, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

	// Generate point drawing texture
	glGenTextures(1, &texture.point);
	glBindTexture(GL_TEXTURE_2D, texture.point);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	setTexSwizzle(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

	// Generate render buffer textures
	glGenTextures(2, buffer.buffers);
	for (int i = 0; i != 2; i++) {
		if (buffer.buffers[i] == 0) {
			qWarning("Cannot generate texture units for rendering.");
			FATAL_ERROR();
		}
		glBindTexture(GL_TEXTURE_2D, buffer.buffers[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, BLOCK_SIZE_W, BLOCK_SIZE_H, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	}

	// Render binarized texture
	glGenFramebuffers(1, &framebuffer.off);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.off);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.demo.bin, 0);

	glViewport(0, 0, texture.demo.width, texture.demo.height);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(bin.program);
	glBindVertexArray(bin.data.vao);
	glBindTexture(GL_TEXTURE_2D, texture.demo.orig);
	glUniform2i(bin.loc.vpSize, texture.demo.width, texture.demo.height);
	glUniform2i(bin.loc.texSize, texture.demo.width, texture.demo.height);
	glUniform1i(bin.loc.zoom, 0);
	glUniform2f(bin.loc.move, 0., 0.);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glFlush();

	// Render initial pattern
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer.current(), 0);
	glViewport(0, 0, BLOCK_SIZE_W, BLOCK_SIZE_H);
	glUseProgram(render.program);
	glBindVertexArray(render.data.vao);
	glBindTexture(GL_TEXTURE_2D, texture.demo.bin);
	glUniform2i(render.loc.vpSize, BLOCK_SIZE_W, BLOCK_SIZE_H);
	glUniform2i(render.loc.texSize, BLOCK_SIZE_W, BLOCK_SIZE_H);
	//glUniform2i(render.loc.texSize, texture.demo.width, texture.demo.height);
	glUniform1i(render.loc.zoom, 0);
	glUniform2f(render.loc.move, 0., 0.);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glFlush();

	updateTitle();
	startTimer(REPORT_FPS_ITVL);	// FPS report timer
}

void GLWidget::resizeGL(int w, int h)
{
	// Qt window system fbo cannot be queried in initializeGL
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer.def);
	glViewport(0, 0, w, h);
}

void GLWidget::paintGL()
{
	if (step == 0 && draw.isEmpty())	// If no off-screen rendering required
		goto render;

	// Start off-screen rendering (calculations, iterations)
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.off);
	glViewport(0, 0, BLOCK_SIZE_W, BLOCK_SIZE_H);

	// Iterations
	if (step != 0) {
		glUseProgram(iteration.program);
		glBindVertexArray(iteration.data.vao);
		glUniform2i(iteration.loc.vpSize, BLOCK_SIZE_W, BLOCK_SIZE_H);
		glUniform2i(iteration.loc.texSize, BLOCK_SIZE_W, BLOCK_SIZE_H);
		glUniform1i(render.loc.zoom, 0);
		glUniform2f(render.loc.move, 0., 0.);

		for (; step != 0; step--) {
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer.next(), 0);
			glBindTexture(GL_TEXTURE_2D, buffer.current());
			glClear(GL_COLOR_BUFFER_BIT);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			glFlush();
			buffer.swap();
		}
	}

	// Drawings
	if (!draw.isEmpty()) {
		glUseProgram(render.program);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer.current(), 0);
		glBindVertexArray(render.data.vao);
		glBindTexture(GL_TEXTURE_2D, texture.point);
		glUniform2i(render.loc.vpSize, BLOCK_SIZE_W, BLOCK_SIZE_H);
		glUniform2i(render.loc.texSize, BLOCK_SIZE_W, BLOCK_SIZE_H);
		glUniform1i(render.loc.zoom, 0);
		glUniform2f(render.loc.move, 0., 0.);

		// Draw life
		setTexSwizzle(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
		for (QSet<int64_t>::iterator it = draw.life.begin(); it != draw.life.end(); it = draw.life.erase(it)) {
			intPack_t ip;
			ip.i64 = *it;
			renderRegion(QRect(ip.i32.x, ip.i32.y, 1., 1.));
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}
		// Draw death
		setTexSwizzle(GL_ONE, GL_ONE, GL_ZERO, GL_ZERO);
		for (QSet<int64_t>::iterator it = draw.death.begin(); it != draw.death.end(); it = draw.death.erase(it)) {
			intPack_t ip;
			ip.i64 = *it;
			renderRegion(QRect(ip.i32.x, ip.i32.y, 1., 1.));
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}
		glFlush();
	}

	// Restore framebuffer and settings
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.def);
	glViewport(0, 0, width(), height());
	renderRegion();

render:
	glUseProgram(render.program);
	glBindVertexArray(render.data.vao);
	glBindTexture(GL_TEXTURE_2D, buffer.current());
	glUniform2i(render.loc.vpSize, width(), height());
	glUniform2i(render.loc.texSize, BLOCK_SIZE_W, BLOCK_SIZE_H);
	glUniform1i(render.loc.zoom, zoom);
	glUniform2f(render.loc.move, move[0], move[1]);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glFlush();

	// Computation time report
	if (!timer.isNull()) {
		int msec = timer.elapsed();
		timer = QTime();
		QMessageBox::information(this, tr("GLLife"), tr("Computation takes %1 seconds.").arg((float)msec / 1000.));
	}
	reportFPS.counter++;
}

void GLWidget::timerEvent(QTimerEvent *e)
{
	if (e->timerId() == timerID) {
		step++;
		update();
	} else	// FPS report timer
		updateTitle();
}

void GLWidget::wheelEvent(QWheelEvent *e)
{
	e->accept();
	zoom += e->angleDelta().y() / 120;
	updateTitle();
	update();
}

void GLWidget::mousePressEvent(QMouseEvent *e)
{
	e->accept();
	if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
		drawPoint(mapToTexture(e->pos()), e->buttons() & Qt::RightButton);
	prevPos = e->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *e)
{
	e->accept();
	if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier) {
		drawPoint(mapToTexture(e->pos()), e->buttons() & Qt::RightButton);
		prevPos = e->pos();
		return;
	}
	QPointF p = e->pos() - prevPos;
	move[0] += -p.x() * pow(2, -zoom);
	move[1] += p.y() * pow(2, -zoom);
	prevPos = e->pos();
	updateTitle();
	update();
}

void GLWidget::keyPressEvent(QKeyEvent *e)
{
	switch (e->key()) {
	case ' ':	// Iteration 1 step
		step++;
		break;
	case 'a':	// Automatic iteration
	case 'A':
		if (timerID) {
			killTimer(timerID);
			timerID = 0;
		} else {
			int itvl = QInputDialog::getInt(this, tr("GLLife"), tr("Please specify iteration interval (ms):"), 0, 0);
			if (itvl)
				timerID = startTimer(itvl);
		}
		break;
	case 's':	// Skip multiple iterations
	case 'S':
	{
		int steps = QInputDialog::getInt(this, tr("GLLife"), tr("Please specify iteration steps:"), 0, 0);
		if (steps) {
			step += steps;
			if (steps >= 100)
				timer.start();
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
	case Qt::Key_Up:
		move[1] += MOVESTEP * pow(2, -zoom);
		break;
	case Qt::Key_Down:
		move[1] -= MOVESTEP * pow(2, -zoom);
		break;
	case Qt::Key_Left:
		move[0] -= MOVESTEP * pow(2, -zoom);
		break;
	case Qt::Key_Right:
		move[0] += MOVESTEP * pow(2, -zoom);
		break;
	case '+':	// Zoom in
	case '=':
		zoom++;
		break;
	case '-':	// Zoom out
	case '_':
		zoom--;
		break;
	default:
		return;
	};
	updateTitle();
	update();
	e->accept();
}

QVector2D GLWidget::mapToTexture(QPoint pos)
{
	QVector2D posF(pos.x(), height() - pos.y() - 1);
	QVector2D size(width(), height());
	return mapToTexture((posF - size / 2.) / size);
}

QVector2D GLWidget::mapToTexture(QVector2D vp)
{
	QVector2D vpSize(width(), height());
	return vp * vpSize / pow(2, zoom) + QVector2D(move[0], move[1]);
}

void GLWidget::renderRegion(QRect region)
{
	if (region.width() == 0 || region.height() == 0)
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices.draw), vertices.draw);
	else {
		QVector2D texSize(BLOCK_SIZE_W, BLOCK_SIZE_H);
		QVector2D tl(QVector2D(region.topLeft()) * 2. / texSize);
		QVector2D br(QVector2D(region.bottomRight() + QPoint(1, 1)) * 2. / texSize);
		const GLfloat vertices_draw[4][2] = {{br.x(), tl.y()}, {tl.x(), tl.y()}, {tl.x(), br.y()}, {br.x(), br.y()}};
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices_draw), vertices_draw);
	}
}

void GLWidget::drawPoint(QVector2D pos, bool death)
{
	intPack_t ip;
	ip.i32.x = floor(pos.x());
	ip.i32.y = floor(pos.y());
	(death ? draw.death : draw.life).insert(ip.i64);
	update();
}

void GLWidget::updateTitle()
{
	if (reportFPS.timer.isNull()) {
		reportFPS.fps = 0.;
		reportFPS.timer.start();
		reportFPS.counter = 0;
	} else if (reportFPS.timer.elapsed() >= REPORT_FPS_ITVL) {
		reportFPS.fps = (float)(reportFPS.counter * 1000) / (float)reportFPS.timer.restart();
		reportFPS.counter = 0;
	}
	emit titleUpdate(tr("GLLife <(%1, %2) * %3> %4 FPS %5")
			.arg(move[0]).arg(move[1]).arg(pow(2, zoom)).arg(reportFPS.fps)
			.arg(timerID == 0 ? tr("[Paused]") : tr("")));
}

void GLWidget::setTexSwizzle(GLint a, GLint r, GLint g, GLint b)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, a);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, r);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, g);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, b);
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
	if (!QString(log).isEmpty()) {
		qDebug(QString("Compile log for shader \"%1\":").arg(path).toLocal8Bit());
		qWarning(log);
	}

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_TRUE)
		return shader;
	qWarning("Shader compilation failed.");
	glDeleteShader(shader);
	return 0;
}

GLuint GLWidget::loadShaders(GLWidget::shader_info_t *shaders)
{
	GLuint program = glCreateProgram();
	if (program == 0) {
		qWarning("Cannot create OpenGL program.");
		return 0;
	}

	GLsizei count = 0;
	GLWidget::shader_info_t *shaderp;
	for (shaderp = shaders; shaderp->type != GL_NONE && shaderp->path != 0; shaderp++, count++) {
		GLuint shader = loadShaderFile(shaderp->type, shaderp->path);
		if (shader == 0)
			goto failed;
		glAttachShader(program, shader);
	}
	glLinkProgram(program);

	{	// Cross initialisation error
		// Get program linking log
		GLint logLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		char log[logLength];
		glGetProgramInfoLog(program, logLength, &logLength, log);
		if (!QString(log).isEmpty()) {
			qDebug(QString("Program linking log:").toLocal8Bit());
			int i = 0;
			for (shaderp = shaders; shaderp->type != GL_NONE && shaderp->path != 0; shaderp++)
				qDebug(QString("Shader %1: \"%2\"").arg(++i).arg(shaderp->path).toLocal8Bit());
			qWarning(log);
		}
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

GLuint GLWidget::genGenericProgram(program_t *program, const char *fsh)
{
	shader_info_t render_shaders[] = {
		{GL_VERTEX_SHADER, RES_SHADER_PFX "vertex.vsh"},
		{GL_FRAGMENT_SHADER, fsh},
		{GL_NONE, 0}
	};
	if ((program->program = loadShaders(render_shaders)) == 0)
		return 0;
	glUseProgram(program->program);

	// Retrive locations
	program->loc.vertex = glGetAttribLocation(program->program, "vertex");
	program->loc.texVertex = glGetAttribLocation(program->program, "texVertex");
	program->loc.vpSize = glGetUniformLocation(program->program, "vpSize");
	program->loc.texSize = glGetUniformLocation(program->program, "texSize");
	program->loc.zoom = glGetUniformLocation(program->program, "zoom");
	program->loc.move = glGetUniformLocation(program->program, "move");
	glVertexAttribPointer(program->loc.vertex, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribIPointer(program->loc.texVertex, 2, GL_INT, 0, (void *)sizeof(vertices.draw));
	glEnableVertexAttribArray(program->loc.vertex);
	glEnableVertexAttribArray(program->loc.texVertex);
	return program->program;
}

void GLWidget::buffer_t::swap()
{
	GLuint tmp = buffers[0];
	buffers[0] = buffers[1];
	buffers[1] = tmp;
}
