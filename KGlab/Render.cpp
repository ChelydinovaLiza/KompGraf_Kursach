#include "Render.h"
#include <Windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "GUItextRectangle.h"
#include "MyShaders.h"
#include "Texture.h"


#include "ObjLoader.h"


#include "debout.h"



//внутренняя логика "движка"
#include "MyOGL.h"
extern OpenGL gl;
#include "Light.h"
Light light;
#include "Camera.h"
Camera camera;


bool texturing = true;
bool lightning = true;
bool alpha = false;
bool birds_stopped = false;

//переключение режимов освещения, текстурирования, альфаналожения
void switchModes(OpenGL *sender, KeyEventArg arg)
{
	//конвертируем код клавиши в букву
	auto key = LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR));

	switch (key)
	{
	case 'L':
		lightning = !lightning;
		break;
	case 'T':
		texturing = !texturing;
		break;
	case 'A':
		alpha = !alpha;
		break;
	case 'C':
        birds_stopped = !birds_stopped; // Переключаем состояние птичек
        break;
	}
}

//умножение матриц c[M1][N1] = a[M1][N1] * b[M2][N2]
template<typename T, int M1, int N1, int M2, int N2>
void MatrixMultiply(const T* a, const T* b, T* c)
{
	for (int i = 0; i < M1; ++i)
	{
		for (int j = 0; j < N2; ++j)
		{
			c[i * N2 + j] = T(0);
			for (int k = 0; k < N1; ++k)
			{
				c[i * N2 + j] += a[i * N1 + k] * b[k * N2 + j];
			}
		}
	}
}

//Текстовый прямоугольничек в верхнем правом углу.
//OGL не предоставляет возможности для хранения текста
//внутри этого класса создается картинка с текстом (через виндовый GDI),
//в виде текстуры накладывается на прямоугольник и рисуется на экране.
//Это самый простой способ что то написать на экране
//но ооооочень не оптимальный
GuiTextRectangle text;

//айдишник для текстуры
GLuint texId;
//выполняется один раз перед первым рендером

ObjModel f;


Shader cassini_sh;
Shader phong_sh;
Shader vb_sh;
Shader simple_texture_sh;

Texture stankin_tex, vb_tex, monkey_tex;



void initRender()
{

	cassini_sh.VshaderFileName = "shaders/v.vert";
	cassini_sh.FshaderFileName = "shaders/cassini.frag";
	cassini_sh.LoadShaderFromFile();
	cassini_sh.Compile();

	phong_sh.VshaderFileName = "shaders/v.vert";
	phong_sh.FshaderFileName = "shaders/light.frag";
	phong_sh.LoadShaderFromFile();
	phong_sh.Compile();

	vb_sh.VshaderFileName = "shaders/v.vert";
	vb_sh.FshaderFileName = "shaders/vb.frag";
	vb_sh.LoadShaderFromFile();
	vb_sh.Compile();

	simple_texture_sh.VshaderFileName = "shaders/v.vert";
	simple_texture_sh.FshaderFileName = "shaders/textureShader.frag";
	simple_texture_sh.LoadShaderFromFile();
	simple_texture_sh.Compile();

	stankin_tex.LoadTexture("textures/stankin.png");
	vb_tex.LoadTexture("textures/vb.png");
	monkey_tex.LoadTexture("textures/monkey.png");


	f.LoadModel("models//monkey.obj_m");


	//==============НАСТРОЙКА ТЕКСТУР================
	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	

	//================НАСТРОЙКА КАМЕРЫ======================
	camera.caclulateCameraPos();

	//привязываем камеру к событиям "движка"
	gl.WheelEvent.reaction(&camera, &Camera::Zoom);
	gl.MouseMovieEvent.reaction(&camera, &Camera::MouseMovie);
	gl.MouseLeaveEvent.reaction(&camera, &Camera::MouseLeave);
	gl.MouseLdownEvent.reaction(&camera, &Camera::MouseStartDrag);
	gl.MouseLupEvent.reaction(&camera, &Camera::MouseStopDrag);
	//==============НАСТРОЙКА СВЕТА===========================
	//привязываем свет к событиям "движка"
	gl.MouseMovieEvent.reaction(&light, &Light::MoveLight);
	gl.KeyDownEvent.reaction(&light, &Light::StartDrug);
	gl.KeyUpEvent.reaction(&light, &Light::StopDrug);
	//========================================================
	//====================Прочее==============================
	gl.KeyDownEvent.reaction(switchModes);
	text.setSize(512, 180);
	//========================================================
	   

	camera.setPosition(2, 1.5, 1.5);
	
}
float view_matrix[16];
double full_time = 0;
int location = 0;
void Render(double delta_time)
{    
	

	full_time += delta_time;
	
	//натройка камеры и света
	//в этих функциях находятся OGLные функции
	//которые устанавливают параметры источника света
	//и моделвью матрицу, связанные с камерой.

	if (gl.isKeyPressed('F')) //если нажата F - свет из камеры
	{
		light.SetPosition(camera.x(), camera.y(), camera.z());
	}
	camera.SetUpCamera();
	//забираем моделвью матрицу сразу после установки камера
	//так как в ней отсуствуют трансформации glRotate...
	//она, фактически, является видовой.
	glGetFloatv(GL_MODELVIEW_MATRIX,view_matrix);

	

	light.SetUpLight();

	//рисуем оси
	gl.DrawAxes();

	

	glBindTexture(GL_TEXTURE_2D, 0);

	//включаем нормализацию нормалей
	//чтобв glScaled не влияли на них.

	glEnable(GL_NORMALIZE);  
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	
	//включаем режимы, в зависимости от нажания клавиш. см void switchModes(OpenGL *sender, KeyEventArg arg)
	if (lightning)
		glEnable(GL_LIGHTING);
	if (texturing)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0); //сбрасываем текущую текстуру
	}
		
	if (alpha)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
		
	//=============НАСТРОЙКА МАТЕРИАЛА==============


	//настройка материала, все что рисуется ниже будет иметь этот метериал.
	//массивы с настройками материала
	float  amb[] = { 0.2, 0.2, 0.1, 1. };
	float dif[] = { 0.4, 0.65, 0.5, 1. };
	float spec[] = { 0.9, 0.8, 0.3, 1. };
	float sh = 0.2f * 256;

	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec); 
	//размер блика
	glMaterialf(GL_FRONT, GL_SHININESS, sh);

	//чтоб было красиво, без квадратиков (сглаживание освещения)
	glShadeModel(GL_SMOOTH); //закраска по Гуро      
			   //(GL_SMOOTH - плоская закраска)

	//============ РИСОВАТЬ ТУТ ==============









	//Квадратик с освещением
	phong_sh.UseShader();

	float light_pos[4] = { light.x(),light.y(), light.z(), 1 };
	float light_pos_v[4];
	
	//переносим координаты света в видовые координаты
	MatrixMultiply<float, 1, 4, 4, 4>(light_pos, view_matrix, light_pos_v);

	
	location = glGetUniformLocationARB(phong_sh.program, "Ia");
	glUniform3fARB(location, 1, 1, 1);
	location = glGetUniformLocationARB(phong_sh.program, "Id");
	glUniform3fARB(location, 1, 1, 1);
	location = glGetUniformLocationARB(phong_sh.program, "Is");
	glUniform3fARB(location, 1, 1, 1);


	location = glGetUniformLocationARB(phong_sh.program, "ma");
	glUniform3fARB(location, 0.1, 0.1, 0.8);
	location = glGetUniformLocationARB(phong_sh.program, "md");
	glUniform3fARB(location, 0.8, 0.6, 0.6);
	location = glGetUniformLocationARB(phong_sh.program, "ms");
	glUniform4fARB(location, 255, 255, 0, 300);
		
	
	location = glGetUniformLocationARB(phong_sh.program, "light_pos_v");
	glUniform3fvARB(location,1,light_pos_v);
	
	glPushMatrix();

	glTranslated(0, 0, 0);

	

	if (texturing) {
		vb_sh.UseShader();
		glActiveTexture(GL_TEXTURE0);
		stankin_tex.Bind();
		glActiveTexture(GL_TEXTURE1);
		vb_tex.Bind();
		location = glGetUniformLocationARB(vb_sh.program, "time");
		glUniform1fARB(location, full_time);
		location = glGetUniformLocationARB(vb_sh.program, "tex_stankin");
		glUniform1iARB(location, 0);
		location = glGetUniformLocationARB(vb_sh.program, "tex_vb");
		glUniform1iARB(location, 1);

	}
	location = glGetUniformLocationARB(vb_sh.program, "time");
	//
	/*glActiveTexture(GL_TEXTURE0);
	stankin_tex.Bind();
	glActiveTexture(GL_TEXTURE1);
	vb_tex.Bind();*/

	
	
	
	// Нижняя грань (z=0), нормаль вниз
	glBegin(GL_QUADS);
	
	glNormal3d(0, 0, -1);
	glTexCoord2d(1, 1);
	glVertex3d(2, 2, 0);
	glTexCoord2d(1, 0);
	glVertex3d(2, -2, 0);
	glTexCoord2d(0, 0);
	glVertex3d(-2, -2, 0);
	glTexCoord2d(0, 1);
	glVertex3d(-2, 2, 0);
	glEnd();



	

	// Верхняя грань (z=4), нормаль вверх
	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1);
	glTexCoord2d(1, 1);
	glVertex3d(2, 2, 4);
	glTexCoord2d(1, 0);
	glVertex3d(2, -2, 4);
	glTexCoord2d(0, 0);
	glVertex3d(-2, -2, 4);
	glTexCoord2d(0, 1);
	glVertex3d(-2, 2, 4);
	glEnd();

	// Правая грань (x=2), нормаль вправо
	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glTexCoord2d(0, 0.703);
	glVertex3d(2, 2, 0);
	glTexCoord2d(0, 1);
	glVertex3d(2, 2, 4);
	glTexCoord2d(0.297, 1);
	glVertex3d(2, -2, 4);
	glTexCoord2d(0.297, 0.703);
	glVertex3d(2, -2, 0);
	glEnd();

	// Задняя грань (y=-2), нормаль назад
	glBegin(GL_QUADS);
	glNormal3d(0, -1, 0);
	glTexCoord2d(0.350, 0.703);
	glVertex3d(-2, -2, 0);
	glTexCoord2d(0.350, 1);
	glVertex3d(-2, -2, 4);
	glTexCoord2d(0.649, 1);
	glVertex3d(2, -2, 4);
	glTexCoord2d(0.649, 0.703);
	glVertex3d(2, -2, 0);
	glEnd();


	

	// Левая грань (x=-2), нормаль влево
	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glTexCoord2d(0.703, 0.703);
	glVertex3d(-2, -2, 0);
	glTexCoord2d(0.703, 1);
	glVertex3d(-2, -2, 4);
	glTexCoord2d(1, 1);
	glVertex3d(-2, 2, 4);
	glTexCoord2d(1, 0.703);
	glVertex3d(-2, 2, 0);
	glEnd();

	// Передняя грань (y=2), нормаль вперёд
	glBegin(GL_QUADS);
	glNormal3d(0, 1, 0);
	glTexCoord2d(0, 0);
	glVertex3d(2, 2, 0);
	glTexCoord2d(0, 0.287);
	glVertex3d(2, 2, 4);
	glTexCoord2d(0.297, 0.287);
	glVertex3d(-2, 2, 4);
	glTexCoord2d(0.297, 0);
	glVertex3d(-2, 2, 0);
	glEnd();


	// Цвет крыши
	glColor3f(0.6f, 0.2f, 0.2f);

	// Центр крыши по оси X и высота вершины крыши по Z
	double roofPeakX = 0.0;
	double roofPeakZ = 6.0; // вершина крыши на 2 выше куба

	// Левый скат крыши (прямоугольник)
	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 1); // приблизительная нормаль
	glTexCoord2d(0, 0.345); glVertex3d(-2, 2, 4); // верхняя грань куба
	glTexCoord2d(0.467, 0.345); glVertex3d(-2, -2, 4);
	glTexCoord2d(0.467, 0.643); glVertex3d(0, -2, roofPeakZ); // вершина крыши
	glTexCoord2d(0, 0.643); glVertex3d(0, 2, roofPeakZ);
	glEnd();

	// Правый скат крыши (прямоугольник)
	glBegin(GL_QUADS);
	glNormal3d(1, 0, 1); // приблизительная нормаль
	glTexCoord2d(0.531, 0.345); glVertex3d(2, 2, 4);
	glTexCoord2d(1, 0.345); glVertex3d(2, -2, 4);
	glTexCoord2d(1, 0.643); glVertex3d(0, -2, roofPeakZ);
	glTexCoord2d(0.531, 0.643); glVertex3d(0, 2, roofPeakZ);
	glEnd();

	// Передний фронтон (треугольник)
	glBegin(GL_TRIANGLES);
	glNormal3d(0, 1, 0); // фронтальная нормаль
	glTexCoord2d(0, 0); glVertex3d(-2, 2, 4);
	glTexCoord2d(0.297, 0); glVertex3d(2, 2, 4);
	glTexCoord2d(0.1485, 0.287); glVertex3d(0, 2, roofPeakZ);
	glEnd();

	// Задний фронтон (треугольник)
	glBegin(GL_TRIANGLES);
	glNormal3d(0, -1, 0); // задняя нормаль
	glTexCoord2d(0, 0); glVertex3d(-2, -2, 4);
	glTexCoord2d(0.297, 0); glVertex3d(2, -2, 4);
	glTexCoord2d(0.1485, 0.287); glVertex3d(0, -2, roofPeakZ);
	glEnd();

	glPopMatrix();



	
	float fence_height = 1.5f;
	float fence_width = 0.1f;
	float fence_length = 0.5f;
	int plank_count = 12;
	float fence_depth = 0.05f;

	float rail_length = (plank_count - 1) * fence_length + fence_width; //  добавлено
	float half = rail_length / 2.0f; //  добавлено: половина длины одной стороны
	float offset = 2.6f;

	for (int i = 0; i < 4; ++i)
	{
		glPushMatrix();

		switch (i)
		{
		case 0: // передняя сторона (вдоль X)
			glTranslatef(-offset, offset, 0);
			break;
		case 1: // правая сторона (вдоль Y)
			glTranslatef(offset, offset, 0);
			glRotatef(90, 0, 0, 1); // поворот на 90
			break;
		case 2: // задняя сторона (вдоль X, но в обратную сторону)
			glTranslatef(offset, -offset, 0);
			glRotatef(180, 0, 0, 1); // поворот на 180
			break;
		case 3: // левая сторона (вдоль Y)
			glTranslatef(-offset, -offset, 0);
			glRotatef(-90, 0, 0, 1); // поворот на -90
			break;
		}

		// >>>>>>> ДОБАВЛЕНО: Разворот забора вдоль стороны квадрата
		if (i % 2 == 1) {
			glTranslatef(0, 0, 0); // Ничего не нужно добавлять — ротация уже ориентирует по Y
		}


		// Нарисуем вертикальные планки
		for (int j = 0; j < plank_count; ++j)
		{
			float x_center = j * fence_length;

			glBegin(GL_QUADS);
			
			glNormal3d(0, 0, 1);
			glVertex3d(x_center - fence_width / 2, 0, 0);
			glVertex3d(x_center + fence_width / 2, 0, 0);
			glVertex3d(x_center + fence_width / 2, 0, fence_height);
			glVertex3d(x_center - fence_width / 2, 0, fence_height);

			glNormal3d(0, 0, -1);
			glVertex3d(x_center - fence_width / 2, -fence_depth, 0);
			glVertex3d(x_center + fence_width / 2, -fence_depth, 0);
			glVertex3d(x_center + fence_width / 2, -fence_depth, fence_height);
			glVertex3d(x_center - fence_width / 2, -fence_depth, fence_height);

			glNormal3d(-1, 0, 0);
			glVertex3d(x_center - fence_width / 2, -fence_depth, 0);
			glVertex3d(x_center - fence_width / 2, 0, 0);
			glVertex3d(x_center - fence_width / 2, 0, fence_height);
			glVertex3d(x_center - fence_width / 2, -fence_depth, fence_height);

			glNormal3d(1, 0, 0);
			glVertex3d(x_center + fence_width / 2, -fence_depth, 0);
			glVertex3d(x_center + fence_width / 2, 0, 0);
			glVertex3d(x_center + fence_width / 2, 0, fence_height);
			glVertex3d(x_center + fence_width / 2, -fence_depth, fence_height);

			glNormal3d(0, 1, 0);
			glVertex3d(x_center - fence_width / 2, 0, fence_height);
			glVertex3d(x_center + fence_width / 2, 0, fence_height);
			glVertex3d(x_center + fence_width / 2, -fence_depth, fence_height);
			glVertex3d(x_center - fence_width / 2, -fence_depth, fence_height);

			glNormal3d(0, -1, 0);
			glVertex3d(x_center - fence_width / 2, 0, 0);
			glVertex3d(x_center + fence_width / 2, 0, 0);
			glVertex3d(x_center + fence_width / 2, -fence_depth, 0);
			glVertex3d(x_center - fence_width / 2, -fence_depth, 0);
			glEnd();
		}

		// Нарисуем горизонтальные перекладины
		float rail_height1 = fence_height * 0.25f;
		float rail_height2 = fence_height * 0.75f;
		float rail_thickness = 0.05f;
		float rail_length = (plank_count - 1) * fence_length + fence_width;

		for (int k = 0; k < 2; ++k)
		{
			float z = (k == 0) ? rail_height1 : rail_height2;

			glBegin(GL_QUADS);
			glNormal3d(0, 0, 1);
			glVertex3d(-fence_width / 2, 0, z);
			glVertex3d(rail_length - fence_width / 2, 0, z);
			glVertex3d(rail_length - fence_width / 2, 0, z + rail_thickness);
			glVertex3d(-fence_width / 2, 0, z + rail_thickness);

			glNormal3d(0, 0, -1);
			glVertex3d(-fence_width / 2, -fence_depth, z);
			glVertex3d(rail_length - fence_width / 2, -fence_depth, z);
			glVertex3d(rail_length - fence_width / 2, -fence_depth, z + rail_thickness);
			glVertex3d(-fence_width / 2, -fence_depth, z + rail_thickness);

			glNormal3d(-1, 0, 0);
			glVertex3d(-fence_width / 2, -fence_depth, z);
			glVertex3d(-fence_width / 2, 0, z);
			glVertex3d(-fence_width / 2, 0, z + rail_thickness);
			glVertex3d(-fence_width / 2, -fence_depth, z + rail_thickness);

			glNormal3d(1, 0, 0);
			glVertex3d(rail_length - fence_width / 2, -fence_depth, z);
			glVertex3d(rail_length - fence_width / 2, 0, z);
			glVertex3d(rail_length - fence_width / 2, 0, z + rail_thickness);
			glVertex3d(rail_length - fence_width / 2, -fence_depth, z + rail_thickness);

			glNormal3d(0, 1, 0);
			glVertex3d(-fence_width / 2, 0, z + rail_thickness);
			glVertex3d(rail_length - fence_width / 2, 0, z + rail_thickness);
			glVertex3d(rail_length - fence_width / 2, -fence_depth, z + rail_thickness);
			glVertex3d(-fence_width / 2, -fence_depth, z + rail_thickness);

			glNormal3d(0, -1, 0);
			glVertex3d(-fence_width / 2, 0, z);
			glVertex3d(rail_length - fence_width / 2, 0, z);
			glVertex3d(rail_length - fence_width / 2, -fence_depth, z);
			glVertex3d(-fence_width / 2, -fence_depth, z);
			glEnd();
		}

		glPopMatrix();
	}


//крыша





	glPopMatrix();




	glPopMatrix();

	Shader::DontUseShaders();  // Отключаем шейдеры
	glDisable(GL_TEXTURE_2D);  // Отключаем текстуры
	glDisable(GL_LIGHTING);    // Отключаем освещение

	// ================= ПТИЧКИ =================
	static float birdsTime = 0.0f;          // Время для анимации птичек
	if (!birds_stopped) {                   // Обновляем только если не остановлены
		birdsTime += delta_time * 0.5f;     // Скорость анимации
	}

	int birdCount = 6;
	float radius = 5.0f;
	float centerZ = 10.0f;


	glColor3f(0.0f, 0.0f, 0.0f); // Чёрные птички

	// Увеличиваем толщину линий для птичек
	GLfloat prevLineWidth;
	glGetFloatv(GL_LINE_WIDTH, &prevLineWidth);
	glLineWidth(3.0f); // Толщина линий в 3 раза больше стандартной

	for (int i = 0; i < birdCount; ++i) {
		glPushMatrix();

		// Смещение по кругу (равномерное распределение)
		float circleOffset = i * (2.0f * 3.14159f / birdCount);

		// Координаты по окружности (XY)
		float x = cosf(birdsTime + circleOffset) * radius;
		float y = sinf(birdsTime + circleOffset) * radius;

		// Движение по синусоиде вверх-вниз (Z-координата)
		float z = centerZ + sinf(birdsTime * 2.0f + i) * 1.5f;

		// Перемещаем птичку в расчетную позицию
		glTranslatef(x, y, z);

		// Рассчитываем направление движения для ориентации птички
		float velocityX = -sinf(birdsTime + circleOffset);
		float velocityY = cosf(birdsTime + circleOffset);
		float rotationAngle = atan2f(velocityY, velocityX) * 180.0f / 3.14159f;

		// Поворачиваем птичку по направлению движения
		glRotatef(rotationAngle, 0.0f, 0.0f, 1.0f);

		// Анимация крыльев - взмахи вверх-вниз
		float wingFlap = sinf(birdsTime * 10.0f + i) * 0.5f;

		// Рисуем птичку в форме галочки с двигающимися крыльями
		glBegin(GL_LINES);
		// Тело птички (основание галочки)
		glVertex3f(0.0f, -0.2f, 0.0f);
		glVertex3f(0.0f, 0.0f, 0.0f);

		// Правое крыло (движется вверх-вниз)
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(0.4f, -0.4f + wingFlap, 0.0f);

		// Левое крыло (движется вверх-вниз)
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(-0.4f, -0.4f + wingFlap, 0.0f);
		glEnd();

		glPopMatrix();
	}

	

	
	//===============================================
	
	
	//сбрасываем все трансформации
	glLoadIdentity();
	camera.SetUpCamera();	
	Shader::DontUseShaders();
	//рисуем источник света
	light.DrawLightGizmo();

	//================Сообщение в верхнем левом углу=======================
	glActiveTexture(GL_TEXTURE0);
	//переключаемся на матрицу проекции
	glMatrixMode(GL_PROJECTION);
	//сохраняем текущую матрицу проекции с перспективным преобразованием
	glPushMatrix();
	//загружаем единичную матрицу в матрицу проекции
	glLoadIdentity();

	//устанавливаем матрицу паралельной проекции
	glOrtho(0, gl.getWidth() - 1, 0, gl.getHeight() - 1, 0, 1);

	//переключаемся на моделвью матрицу
	glMatrixMode(GL_MODELVIEW);
	//сохраняем матрицу
	glPushMatrix();
    //сбразываем все трансформации и настройки камеры загрузкой единичной матрицы
	glLoadIdentity();

	//отрисованное тут будет визуалзироватся в 2д системе координат
	//нижний левый угол окна - точка (0,0)
	//верхний правый угол (ширина_окна - 1, высота_окна - 1)

	
	std::wstringstream ss;
	ss << std::fixed << std::setprecision(3);
	ss << "T - " << (texturing ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"текстур" << std::endl;
	ss << "L - " << (lightning ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"освещение" << std::endl;
	ss << "A - " << (alpha ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"альфа-наложение" << std::endl;
	ss << L"F - Свет из камеры" << std::endl;
	ss << L"G - двигать свет по горизонтали" << std::endl;
	ss << L"G+ЛКМ двигать свет по вертекали" << std::endl;
	ss << L"Коорд. света: (" << std::setw(7) <<  light.x() << "," << std::setw(7) << light.y() << "," << std::setw(7) << light.z() << ")" << std::endl;
	ss << L"Коорд. камеры: (" << std::setw(7) << camera.x() << "," << std::setw(7) << camera.y() << "," << std::setw(7) << camera.z() << ")" << std::endl;
	ss << L"Параметры камеры: R=" << std::setw(7) << camera.distance() << ",fi1=" << std::setw(7) << camera.fi1() << ",fi2=" << std::setw(7) << camera.fi2() << std::endl;
	ss << L"delta_time: " << std::setprecision(5)<< delta_time << std::endl;
	ss << L"full_time: " << std::setprecision(2) << full_time << std::endl;

	
	text.setPosition(10, gl.getHeight() - 10 - 180);
	text.setText(ss.str().c_str());
	
	text.Draw();

	//восстанавливаем матрицу проекции на перспективу, которую сохраняли ранее.
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
	
}   



