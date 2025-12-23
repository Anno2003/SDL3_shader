#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <sstream>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_GLContext gl_context = NULL;
static GLuint shaderProgram;

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
    if (!SDL_CreateWindowAndRenderer("Glass Shader", 600, 600, SDL_WINDOW_TRANSPARENT|SDL_WINDOW_ALWAYS_ON_TOP|SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL, &window, &renderer)) {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
	
	// create gl context
	gl_context = SDL_GL_CreateContext(window);
	if (gl_context == NULL){
		SDL_Log("Couldn't create gl context: %s", SDL_GetError());
        return SDL_APP_FAILURE;
	}
	
	glewExperimental = GL_TRUE; // Fixes issues with some drivers
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        SDL_Log("GLEW Init Error: %s", SDL_GetError());
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        return SDL_APP_FAILURE;
    }
	
	shaderProgram = LoadShader("vertex_shader.glsl", "blur_shader.glsl");
	
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
    
	const char *message = "Hello World!";
    int w = 0, h = 0;
    float x, y;
    const float scale = 4.0f;

    /* Center the message and scale it up */
    SDL_GetRenderOutputSize(renderer, &w, &h);
    SDL_SetRenderScale(renderer, scale, scale);
    x = ((w / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(message)) / 2;
    y = ((h / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2;

    /* Draw the message */
    /*
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer, x, y, message);
    SDL_RenderPresent(renderer);
	*/
	glClear(GL_COLOR_BUFFER_BIT);
        
	glUseProgram(shaderProgram);
	CheckGLError("glUseProgram");
	
	glUniform1i(glGetUniformLocation(shaderProgram, "screenTexture"), 0);
	CheckGLError("glUniform1i");

	glUniform2f(glGetUniformLocation(shaderProgram, "resolution"), 800.0f, 600.0f);
	CheckGLError("glUniform2f");

	glUniform1f(glGetUniformLocation(shaderProgram, "blurRadius"), 2.0f);
	CheckGLError("glUniform1f");
	
	SDL_GL_SwapWindow(window);
	
    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result){
	SDL_GL_DestroyContext(gl_context);
	SDL_DestroyWindow(window);
}
