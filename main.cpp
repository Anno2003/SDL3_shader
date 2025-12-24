#define SDL_MAIN_USE_CALLBACKS 1
#include <glad/gl.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>

static SDL_Window *window = NULL;
static SDL_GLContext gl_context = NULL;
static GLuint shaderProgram;
static GLuint VAO, VBO;

void CheckShaderCompilation(GLuint shader, const char* type) {
    GLint success;
    GLchar infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        std::cerr << "Shader Compilation Error (" << type << "): " << infoLog << std::endl;
    }
}

void CheckShaderLinking(GLuint program) {
    GLint success;
    GLchar infoLog[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, NULL, infoLog);
        std::cerr << "Shader Linking Error: " << infoLog << std::endl;
    }
}

GLuint LoadShader(const char* vertexPath, const char* fragmentPath) {
    std::ifstream vShaderFile(vertexPath);
    std::ifstream fShaderFile(fragmentPath);
    std::stringstream vShaderStream, fShaderStream;

    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();

    std::string vShaderCode = vShaderStream.str();
    std::string fShaderCode = fShaderStream.str();

    const char* vShaderSource = vShaderCode.c_str();
    const char* fShaderSource = fShaderCode.c_str();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vShaderSource, NULL);
    glCompileShader(vertexShader);
    CheckShaderCompilation(vertexShader, "VERTEX");

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fShaderSource, NULL);
    glCompileShader(fragmentShader);
    CheckShaderCompilation(fragmentShader, "FRAGMENT");

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    CheckShaderLinking(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void CheckGLError(const char* function) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL Error in " << function << ": " << err << std::endl;
    }
}

/* init */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]){
	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
    /* Create the window */
	window = SDL_CreateWindow("Glass Shader", 600, 600, SDL_WINDOW_TRANSPARENT|SDL_WINDOW_ALWAYS_ON_TOP|SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL); 
    if (!window) {
        SDL_Log("Couldn't create window : %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
	SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "60");
	// create gl context
	gl_context = SDL_GL_CreateContext(window);
	if (gl_context == NULL){
		SDL_Log("Couldn't create gl context: %s", SDL_GetError());
        return SDL_APP_FAILURE;
	}
	SDL_GL_MakeCurrent(window, gl_context);
	
	int version = gladLoadGL((GLADloadfunc) SDL_GL_GetProcAddress);
    printf("GL %d.%d\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	shaderProgram = LoadShader("vertex_shader.glsl", "frag_shader.glsl");
	
	float vertices[] = {
		// Pos      // UV (Texture Coords)
		-1.0f,  1.0f,  0.0f, 1.0f, // Top-Left
		1.0f,  1.0f,  1.0f, 1.0f, // Top-Right
		1.0f, -1.0f,  1.0f, 0.0f, // Bottom-Right

		-1.0f,  1.0f,  0.0f, 1.0f, // Top-Left
		1.0f, -1.0f,  1.0f, 0.0f, // Bottom-Right
		-1.0f, -1.0f,  0.0f, 0.0f  // Bottom-Left
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

    return SDL_APP_CONTINUE;
}

/* updates on event */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event){
    if (event->type == SDL_EVENT_KEY_DOWN ||
        event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE;
}

/* loop */
SDL_AppResult SDL_AppIterate(void *appstate){
	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	glViewport(0, 0, w, h);
	
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
    
	glUseProgram(shaderProgram);
	CheckGLError("glUseProgram");
	
	glUniform2f(glGetUniformLocation(shaderProgram, "resolution"), (float)w, (float)h);
	CheckGLError("glUniform2f");

	glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
	
	SDL_GL_SwapWindow(window);
	
    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result){
	glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
}
