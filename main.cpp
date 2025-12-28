#define SDL_MAIN_USE_CALLBACKS 1
#include <glad/gl.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
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

const char* defaultVertexShader   = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
  
    out vec4 vertexColor;

    void main(){
        gl_Position = vec4(aPos, 1.0);
        vertexColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
)";

const char* defaultFragmentShader = R"(
    #version 330 core

    uniform vec2 u_resolution;
    uniform vec2 u_mouse;
    uniform float u_time;
    
    void main(){
        vec2 st = gl_FragCoord.xy/u_resolution;
        gl_FragColor = vec4(st.x,st.y,abs(sin(u_time)),abs(cos(u_time)));
    } 
)";

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

GLuint LoadDefaultShader(){
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader,1,&defaultVertexShader,NULL);
    glCompileShader(vertexShader);
    CheckShaderCompilation(vertexShader,"VERTEX");

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader,1,&defaultFragmentShader,NULL);
    glCompileShader(fragmentShader);
    CheckShaderCompilation(fragmentShader,"FRAGMENT");

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram,vertexShader);
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
	window = SDL_CreateWindow("SDL3 Shader", 600, 600, SDL_WINDOW_TRANSPARENT|SDL_WINDOW_ALWAYS_ON_TOP|SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL); 
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
	
	shaderProgram = LoadDefaultShader();
	
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;        // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;      // Enable Multi-Viewport
    io.FontGlobalScale = 1.5f;
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init();

    return SDL_APP_CONTINUE;
}

/* updates on event */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event){
    ImGui_ImplSDL3_ProcessEvent(event);
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE;
}

/* loop */
SDL_AppResult SDL_AppIterate(void *appstate){
    float time = SDL_GetTicks()/1000.0f; 
	int w, h;
    float x,y;
    SDL_MouseButtonFlags buttonState = SDL_GetMouseState(&x,&y);
    
	SDL_GetWindowSize(window, &w, &h);
	glViewport(0, 0, w, h);
	
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if(ImGui::BeginMainMenuBar()){
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Shader", "Ctrl+S")) { /* Do something */ }
            if (ImGui::MenuItem("Load Shader", "Ctrl+O")) { /* Do something */ }
            ImGui::Separator();
            if (ImGui::MenuItem("Preferences")) {}
            if (ImGui::MenuItem("Quit")) { return SDL_APP_SUCCESS; }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Edit Fragment Shader")){}
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
    
	glUseProgram(shaderProgram);
	CheckGLError("glUseProgram");

    glUniform1f(glGetUniformLocation(shaderProgram,"u_time"),time);
    CheckGLError("glUniform1f");
	
	glUniform2f(glGetUniformLocation(shaderProgram, "u_resolution"), (float)w, (float)h);
	CheckGLError("glUniform2f");

    glUniform2f(glGetUniformLocation(shaderProgram,"u_mouse"),x,y);
    CheckGLError("glUniform2f");

	glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
	
    // Rendering
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable){
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
    
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }
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
