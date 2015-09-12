#include <QDebug>
#include <QOpenGLFramebufferObject>
#include <QFileDialog>
#include <QDialog>
#include <QVBoxLayout>
#include "glwidget.h"

#define RESPFX		":/shaders/"
#define ZOOMSTEP	0.25
#define DIM		640

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
	data.zoom = 0;
	data.moveX = 0;
	data.moveY = 0;
	data.vsh = 0;
	data.fsh = 0;
	data.program = 0;

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

	glGenVertexArrays(1, &data.vao);
	glBindVertexArray(data.vao);
	GLint vertices[4][2] = {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}};
	glGenBuffers(1, &data.buffer);
	glBindBuffer(GL_ARRAY_BUFFER, data.buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	shader_info_t shaders[] = {
		{GL_VERTEX_SHADER, RESPFX "vertex.vsh"},
		{GL_FRAGMENT_SHADER, RESPFX "display.fsh"},
		{GL_NONE, 0}
	};
	data.program = loadShaders(shaders);
	if (!data.program) {
		qApp->quit();
		return;
	}

	data.loc.vertex = glGetAttribLocation(data.program, "vertex");
	data.loc.projection = glGetUniformLocation(data.program, "projection");
	data.loc.colour = glGetUniformLocation(data.program, "colour");
	//data.loc.dim = glGetUniformLocation(data.program, "DIM");
	//data.loc.zoom = glGetUniformLocation(data.program, "zoom");
	//data.loc.animation = glGetUniformLocation(data.program, "animation");
	//data.loc.position = glGetUniformLocation(data.program, "position");
	glUseProgram(data.program);
	glUniform3i(data.loc.colour, 102, 204, 255);
	glVertexAttribIPointer(data.loc.vertex, 2, GL_INT, 0, 0/*sdata.vertices*/);
	glEnableVertexAttribArray(data.loc.vertex);

	glClearColor(0.0, 0.0, 0.0, 1.0);
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

	glUniformMatrix4fv(data.loc.projection, 1, GL_FALSE, data.projection.constData());
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glFlush();
	//glFinish();
#if 0
	if (!data.saving)
		return;
	data.saving = false;

	QImage *img = saveDialog->img;
	QImage imgRen(data.save.blockSize, QImage::Format_RGB888);
	if (img == 0) {
		saveDialog->failed(tr("img allocation failed"));
		return;
	}
	if (img->isNull()) {
		saveDialog->failed(tr("img is null"));
		return;
	}
	if (imgRen.isNull()) {
		saveDialog->failed(tr("imgRen is null"));
		return;
	}

	QOpenGLFramebufferObject fbo(data.save.blockSize);
	fbo.bind();
	glViewport(0, 0, data.save.blockSize.width(), data.save.blockSize.height());
	QPoint pos;
	int w = data.save.blockSize.width() * data.save.blockCount.width();
	int h = data.save.blockSize.height() * data.save.blockCount.height();
	float asp = (float)h / (float)w;
render:
	// Select region
	data.projection.setToIdentity();
	float left = float(2 * pos.x() - data.save.blockCount.width()) / data.save.blockCount.width();
	float top = float(2 * pos.y() - data.save.blockCount.height()) / data.save.blockCount.height();
	data.projection.ortho(left, left + 2. / data.save.blockCount.width(),
			      asp * top, asp * (top + 2. / data.save.blockCount.height()), -1., 1.);
	render();

	QPoint imgPos(pos.x() * data.save.blockSize.width(), pos.y() * data.save.blockSize.height());
	glReadPixels(0, 0, imgRen.width(), imgRen.height(), GL_RGB, GL_UNSIGNED_BYTE, imgRen.bits());
	unsigned char *imgPtr = img->scanLine(img->height() - imgPos.y() - 1) + imgPos.x() * 3;
	for (int i = 0; i < imgRen.height(); i++) {
		memcpy(imgPtr, imgRen.constScanLine(i), imgRen.bytesPerLine());
		imgPtr -= img->bytesPerLine();
	}

	if (pos.x() != data.save.blockCount.width() - 1)
		pos.setX(pos.x() + 1);
	else if (pos.y() != data.save.blockCount.height() - 1) {
		pos.setX(0);
		pos.setY(pos.y() + 1);
	} else {
		fbo.release();
		resizeGL(width(), height());
		saveDialog->done();
		return;
	}
	goto render;
#endif
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
	glGetAttachedShaders(data.program, count, &count, sh);
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
