#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb_image.h>

#include <ft2build.h>
#include FT_FREETYPE_H   

#include "Shader.h"
#include "Sphere.h"
#include "Camera.h"

#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>
#include <wtypes.h>
#include <ctime>
#include <filesystem>
#include <string>
#define _USE_MATH_DEFINES
#include <math.h>

#define TAU (M_PI * 2.0)


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
unsigned int loadTexture(char const* path);
unsigned int loadCubemap(std::vector<std::string> faces);
void GetDesktopResolution(float& horizontal, float& vertical)
{
	RECT desktop;
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &desktop);
	horizontal = desktop.right;
	vertical = desktop.bottom;
}

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool onRotate = false;
bool onFreeCam = true;
bool SkyBoxExtra = false;
float SCREEN_WIDTH = 800, SCREEN_HEIGHT = 600;

glm::vec3 point = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 PlanetPos = glm::vec3(0.0f, 0.0f, 0.0f);
GLfloat lastX = (GLfloat)(SCREEN_WIDTH / 2.0);
GLfloat lastY = (GLfloat)(SCREEN_HEIGHT / 2.0);
float PlanetSpeed = .1f;
int PlanetView = 0;

bool keys[1024];
GLfloat SceneRotateY = 0.0f;
GLfloat SceneRotateX = 0.0f;
bool onPlanet = false;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
	{
		//camera.Position = PlanetPos;
		onPlanet = true;
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			keys[key] = true;
		else if (action == GLFW_RELEASE)
			keys[key] = false;
	}
}

bool firstMouse = true;
GLfloat xoff = 0.0f, yoff = 0.0f;

struct Character {
	GLuint TextureID;   // ID handle of the glyph texture
	glm::ivec2 Size;    // Size of glyph
	glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
	GLuint Advance;    // Horizontal offset to advance to next glyph
};
std::map<GLchar, Character> Characters;
GLuint textVAO, textVBO;

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = (GLfloat)xpos;
		lastY = (GLfloat)ypos;
		firstMouse = false;
	}

	GLfloat xoffset = (GLfloat)(xpos - lastX);
	GLfloat yoffset = (GLfloat)(lastY - ypos);
	xoff = xoffset;
	yoff = yoff;

	lastX = (GLfloat)xpos;
	lastY = (GLfloat)ypos;
	if (onRotate)
	{
		SceneRotateY += yoffset * 0.1f;
		SceneRotateX += xoffset * 0.1f;
	}
	camera.ProcessMouseMovement(xoffset, yoffset);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (yoffset == 1)
		camera.ProcessKeyboard(SCROLL_FORWARD, deltaTime);
	else
	{
		camera.ProcessKeyboard(SCROLL_BACKWARD, deltaTime);
	}
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (onFreeCam && !camera.FreeCam)
	{
		if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
			onRotate = true;
		else onRotate = false;
	}
}

int main() {
	GetDesktopResolution(SCREEN_WIDTH, SCREEN_HEIGHT); // get resolution for create window
	camera.LookAtPos = point;

	/* GLFW INIT */
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	/* GLFW INIT */

	/* GLFW WINDOW CREATION */
	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "LearnOpenGL", glfwGetPrimaryMonitor(), NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	/* GLFW WINDOW CREATION */


	/* LOAD GLAD */
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	/* LOAD GLAD */

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* SHADERS */
	Shader SimpleShader("simpleVS.vs", "simpleFS.fs");
	Shader SkyboxShader("skybox.vs", "skybox.fs");
	Shader texShader("simpleVS.vs", "texFS.fs");
	/* SHADERS */

	float cube[] = {
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
	};
	float skyboxVertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};

	/* SKYBOX GENERATION */
	unsigned int skyboxVAO, skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	/* SKYBOX GENERATION */

	/* VERTEX GENERATION FOR ORBITS */
	std::vector<float> orbVert;
	GLfloat xx;
	GLfloat zz;
	float angl;
	for (int i = 0; i < 2000; i++)
	{
		angl = (float)(M_PI / 2 - i * (M_PI / 1000));
		xx = sin(angl) * 100.0f;
		zz = cos(angl) * 100.0f;
		orbVert.push_back(xx);
		orbVert.push_back(0.0f);
		orbVert.push_back(zz);

	}
	/* VERTEX GENERATION FOR ORBITS */

	/* VAO-VBO for ORBITS*/
	GLuint VBO_t, VAO_t;
	glGenVertexArrays(1, &VAO_t);
	glGenBuffers(1, &VBO_t);
	glBindVertexArray(VAO_t);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_t);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * orbVert.size(), orbVert.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	/* VAO-VBO for ORBITS*/

	/* LOAD TEXTURES */
	unsigned int texture_earth = loadTexture("resources/planets/earth2k.jpg");
	unsigned int texture_moon = loadTexture("resources/planets/2k_moon.jpg");
	unsigned int texture_earth_clouds = loadTexture("resources/planets/2k_earth_clouds.jpg");
	/* LOAD TEXTURES */

	/* SPHERE GENERATION */
	Sphere Venus(12.0f, 36, 18);
	Sphere Earth(11.8f, 36, 18);
	Sphere Moon(5.5f, 36, 18);
	/* SPHERE GENERATION */

	std::vector<std::string> faces
	{
		"resources/skybox/starfield/starfield_rt.tga",
		"resources/skybox/starfield/starfield_lf.tga",
		"resources/skybox/starfield/starfield_up.tga",
		"resources/skybox/starfield/starfield_dn.tga",
		"resources/skybox/starfield/starfield_ft.tga",
		"resources/skybox/starfield/starfield_bk.tga",
	};
	std::vector<std::string> faces_extra
	{
		"resources/skybox/blue/bkg1_right.png",
		"resources/skybox/blue/bkg1_left.png",
		"resources/skybox/blue/bkg1_top.png",
		"resources/skybox/blue/bkg1_bot.png",
		"resources/skybox/blue/bkg1_front.png",
		"resources/skybox/blue/bkg1_back.png",
	};

	unsigned int cubemapTexture = loadCubemap(faces);
	unsigned int cubemapTextureExtra = loadCubemap(faces_extra);
	GLfloat camX = 10.0f;
	GLfloat camZ = 10.0f;

	camera.Position = glm::vec3(0.0f, 250.0f, -450.0f);
	camera.Yaw = 90.0f;
	camera.Pitch = -40.0f;
	camera.ProcessMouseMovement(xoff, yoff);
	camera.FreeCam = false;
	onFreeCam = true;
	glm::mat4 view;
	glm::vec3 PlanetsPositions[9];
	while (!glfwWindowShouldClose(window))
	{

		GLfloat currentFrame = (GLfloat)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		/* ZOOM CONTROL */
		if (!camera.FreeCam)
		{
			if (camera.Position.y < 200 && camera.Position.y > 200.0f)
				camera.MovementSpeed = 300.0f;
			if (camera.Position.y < 125.f && camera.Position.y > 70.0f)
				camera.MovementSpeed = 200.0f;
			if (camera.Position.y < 70.f && camera.Position.y > 50.0f)
				camera.MovementSpeed = 100.0f;
			if (camera.Position.y > 200 && camera.Position.y < 400.0f)
				camera.MovementSpeed = 400.0f;
			if (camera.Position.y > 125.f && camera.Position.y < 200.0f)
				camera.MovementSpeed = 300.0f;
			if (camera.Position.y > 70.f && camera.Position.y < 125.0f)
				camera.MovementSpeed = 200.0f;
		}
		/* ZOOM CONTROL */

		processInput(window); // input

		if (!onFreeCam)
		{
			SceneRotateY = 0.0f;
			SceneRotateX = 0.0f;
		}
		if (camera.FreeCam || PlanetView > 0)
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		// render
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		SimpleShader.Use();

		glm::mat4 model = glm::mat4(1.0f);

		double viewX;
		double viewZ;
		glm::vec3 viewPos;

		SimpleShader.Use();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 10000.0f);
		SimpleShader.setMat4("model", model);
		SimpleShader.setMat4("view", view);
		SimpleShader.setMat4("projection", projection);

		/* EARTH */
		glm::mat4 model_earth;
		xx = sin(glfwGetTime() * PlanetSpeed * 0.55f) * 100.0f * 0.0f * 1.3f;
		zz = cos(glfwGetTime() * PlanetSpeed * 0.55f) * 100.0f * 0.0f * 1.3f;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_earth);
		// model_earth = glm::translate(model_earth, point);
		model_earth = glm::rotate(model_earth, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		model_earth = glm::rotate(model_earth, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
		// model_earth = glm::translate(model_earth, glm::vec3(xx, 0.0f, zz));
		glm::vec3 EarthPoint = glm::vec3(xx, 0.0f, zz);
		PlanetsPositions[2] = glm::vec3(xx, 0.0f, zz);
		model_earth = glm::rotate(model_earth, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.f));
		model_earth = glm::rotate(model_earth, glm::radians(-33.25f), glm::vec3(0.0f, 1.0f, 0.f));
		model_earth = glm::rotate(model_earth, (GLfloat)glfwGetTime() * glm::radians(-33.25f) * 2.0f, glm::vec3(0.0f, 0.0f, 1.f));
		camera.LookAtPos = glm::vec3(model_earth[3][0], model_earth[3][1], model_earth[3][2]);
		SimpleShader.setMat4("model", model_earth);
		Earth.Draw();
		/* EARTH */

		/* EARTH SCALED */
		glm::mat4 model_earth_scaled;
		xx = sin(glfwGetTime() * PlanetSpeed * 0.55f) * 100.0f * 3.0f * 1.3f;
		zz = cos(glfwGetTime() * PlanetSpeed * 0.55f) * 100.0f * 3.0f * 1.3f;
		// float initial_xx = sin(PlanetSpeed * 0.55f) * 100.0f * 3.0f *1.3f;
		// float initial_zz = cos(PlanetSpeed * 0.55f) * 100.0f * 3.0f *1.3f;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_earth);
		model_earth_scaled = glm::translate(model_earth_scaled, point);
		model_earth_scaled = glm::rotate(model_earth_scaled, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		model_earth_scaled = glm::rotate(model_earth_scaled, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
		model_earth_scaled = glm::translate(model_earth_scaled, glm::vec3(xx, 0.0f, zz));
		// model_moon = glm::translate(model_moon, glm::vec3(xx, 0.0f, zz));
		glm::vec3 EarthPoint_scaled = glm::vec3(xx, 0.0f, zz);
		PlanetsPositions[1] = glm::vec3(xx, 0.0f, zz);
		model_earth_scaled = glm::rotate(model_earth_scaled, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.f));
		model_earth_scaled = glm::rotate(model_earth_scaled, glm::radians(-33.25f), glm::vec3(0.0f, 1.0f, 0.f));
		model_earth_scaled = glm::rotate(model_earth_scaled, (GLfloat)glfwGetTime() * glm::radians(-33.25f) * 2.0f, glm::vec3(0.0f, 0.0f, 1.f));
		model_earth_scaled = glm::scale(model_earth_scaled, glm::vec3(3.0f));
		camera.LookAtPos = glm::vec3(model_earth_scaled[3][0], model_earth_scaled[3][1], model_earth_scaled[3][2]);
		SimpleShader.setMat4("model", model_earth_scaled);
		Earth.Draw();
		/* EARTH SCALED */

		/* MOON */
		glm::mat4 model_moon;
		xx = sin(glfwGetTime() * PlanetSpeed * 7.55f) * 100.0f * 0.5f * 1.3f;
		zz = cos(glfwGetTime() * PlanetSpeed * 7.55f) * 100.0f * 0.5f * 1.3f;
		float initial_xx = sin(PlanetSpeed * 7.55f) * 100.0f * 0.5f * 1.3f;
		float initial_zz = cos(PlanetSpeed * 7.55f) * 100.0f * 0.5f * 1.3f;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_moon);
		model_moon = glm::translate(model_moon, EarthPoint);
		model_moon = glm::rotate(model_moon, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		model_moon = glm::rotate(model_moon, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
		model_moon = glm::translate(model_moon, glm::vec3(zz, 0.0f, xx));
		// model_moon = glm::translate(model_moon, glm::vec3(initial_xx, 0.0f, initial_zz));
		model_moon = glm::rotate(model_moon, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.f));
		model_moon = glm::rotate(model_moon, glm::radians(-32.4f), glm::vec3(0.0f, 1.0f, 0.f));
		model_moon = glm::rotate(model_moon, (GLfloat)glfwGetTime() * glm::radians(-32.4f) * 3.1f, glm::vec3(0.0f, 0.0f, 1.f));
		SimpleShader.setMat4("model", model_moon);
		Moon.Draw();
		/* MOON */

		/* ORBITS */
		glBindVertexArray(VAO_t);
		glLineWidth(1.0f);
		glm::mat4 modelorb;
		for (float i = 3; i < 4; i++)
		{
			modelorb = glm::mat4(1);
			modelorb = glm::translate(modelorb, point);
			modelorb = glm::rotate(modelorb, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
			modelorb = glm::rotate(modelorb, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
			modelorb = glm::scale(modelorb, glm::vec3(i * 1.3f, i * 1.3f, i * 1.3f));
			SimpleShader.setMat4("model", modelorb);
			glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)orbVert.size() / 3);

		}
		modelorb = glm::mat4(1);
		modelorb = glm::rotate(modelorb, glm::radians(SceneRotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		modelorb = glm::rotate(modelorb, glm::radians(SceneRotateX), glm::vec3(0.0f, 0.0f, 1.0f));
		modelorb = glm::translate(modelorb, EarthPoint);
		modelorb = glm::scale(modelorb, glm::vec3(0.5f * 1.3f, 0.5f * 1.3f, 0.5f * 1.3f));
		SimpleShader.setMat4("model", modelorb);
		glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)orbVert.size() / 3);
		/* ORBITS */


		/* DRAW SKYBOX */
		glDepthFunc(GL_LEQUAL);
		SkyboxShader.Use();
		view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
		SkyboxShader.setMat4("view", view);
		SkyboxShader.setMat4("projection", projection);
		// skybox cube
		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		if (SkyBoxExtra)
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureExtra);
		else
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS);
		/* DRAW SKYBOX */

		switch (PlanetView)
		{
		case 2:
			viewX = sin(glfwGetTime() * PlanetSpeed * 0.75f) * 100.0f * 4.5f * 1.2f;
			viewZ = cos(glfwGetTime() * PlanetSpeed * 0.75f) * 100.0f * 4.5f * 1.2f;
			viewPos = glm::vec3(viewX, 50.0f, viewZ);
			view = glm::lookAt(viewPos, PlanetsPositions[1], glm::vec3(0.0f, 1.0f, 0.0f));
			break;

		case 3:
			viewX = sin(glfwGetTime() * PlanetSpeed * 0.55f) * 100.0f * 5.5f * 1.2f;
			viewZ = cos(glfwGetTime() * PlanetSpeed * 0.55f) * 100.0f * 5.5f * 1.2f;
			viewPos = glm::vec3(viewX, 50.0f, viewZ);
			view = glm::lookAt(viewPos, PlanetsPositions[2], glm::vec3(0.0f, 1.0f, 0.0f));
			break;

		case 0:
			view = camera.GetViewMatrix();
			break;
		}
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &VAO_t);
	glDeleteBuffers(1, &VBO_t);
	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);

	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		SkyBoxExtra = true;

	if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
	{
		PlanetView = 0;
		onFreeCam = false;
		camera.FreeCam = true;
	}

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		PlanetView = 0;
		onFreeCam = true;
		camera.FreeCam = false;
		camera.Position = glm::vec3(0.0f, 250.0f, -450.0f);
		camera.Yaw = 90.0f;
		camera.Pitch = -40.0f;
		camera.GetViewMatrix();
		camera.ProcessMouseMovement(xoff, yoff);
	}

	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
	{
		PlanetView = 3;
		onFreeCam = false;
		camera.FreeCam = false;
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;

	for (unsigned int i = 0; i < faces.size(); i++)
	{

		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);

		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}