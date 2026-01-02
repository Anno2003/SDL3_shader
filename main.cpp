#define SDL_MAIN_USE_CALLBACKS 1
#include <glad/gl.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_dialog.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>

static SDL_Window *window = NULL;
static SDL_GLContext gl_context = NULL;
static GLuint shaderProgram;
static GLuint VAO, VBO;
static bool pendingShaderReload = false;
static bool show_metrics = false;

float clickX, clickY;
bool isLeftDown = false;

// DEFAULT SHADERS ////
static std::string defaultVertexShader   = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
  
out vec4 vertexColor;

void main(){
    gl_Position = vec4(aPos, 1.0);
    vertexColor = vec4(0.0, 0.0, 0.0, 0.0);
}
)";

// default Fragment Shader is also current shader
static std::string defaultFragmentShader = R"(
#version 330 core

uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;
    
void main(){
    vec2 st = gl_FragCoord.xy/u_resolution;
    gl_FragColor = vec4(st.x,st.y,abs(sin(u_time)),abs(cos(u_time)));
} 
)";

// OpenGL Helpers ////
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

GLuint LoadDefaultShader(){
    const char* vsrc = defaultVertexShader.c_str(); 
    const char* fsrc = defaultFragmentShader.c_str();
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader,1,&vsrc,NULL);
    glCompileShader(vertexShader);
    CheckShaderCompilation(vertexShader,"VERTEX");

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader,1,&fsrc,NULL);
    glCompileShader(fragmentShader);
    CheckShaderCompilation(fragmentShader,"FRAGMENT");

    GLuint tempShaderProgram = glCreateProgram();
    glAttachShader(tempShaderProgram,vertexShader);
    glAttachShader(tempShaderProgram, fragmentShader);
    glLinkProgram(tempShaderProgram);
    CheckShaderLinking(tempShaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return tempShaderProgram;
}

void CheckGLError(const char* function) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL Error in " << function << ": " << err << std::endl;
    }
}

// File Saving & Loading ////
static const SDL_DialogFileFilter file_filters[] = {
    { "glsl shader",  "glsl" },
    { "All files",   "*" }
};

static void SDLCALL save_callback(void* userdata, const char* const* filelist, int filter){
    if (!filelist) {
        SDL_Log("An error occured: %s", SDL_GetError());
        return;
    } else if (!*filelist) {
        SDL_Log("The dialog was canceled.");
        return;
    }

    const char* save_path = filelist[0];
    size_t data_size = SDL_strlen(defaultFragmentShader.c_str()) + 1;
    if (SDL_SaveFile(save_path, defaultFragmentShader.c_str(), data_size)) {
        SDL_Log("Successfully saved file: '%s'", save_path);
    } else {
        SDL_Log("Failed to save file: %s", SDL_GetError());
    }

    if (filter < 0) {
        SDL_Log("The current platform does not support fetching "
                "the selected filter, or the user did not select"
                " any filter.");
        return;
    } else if (filter < SDL_arraysize(file_filters)) {
        SDL_Log("The filter selected by the user is '%s' (%s).",
                file_filters[filter].pattern, file_filters[filter].name);
        return;
    }
}

static void SDLCALL load_callback(void* userdata, const char* const* filelist, int filter){// TODO
    if (!filelist) {
        SDL_Log("An error occured: %s", SDL_GetError());
        return;
    } else if (!*filelist) {
        SDL_Log("The user did not select any file.");
        SDL_Log("Most likely, the dialog was canceled.");
        return;
    }

    const char* load_path = filelist[0];
    size_t fileSize = 0;
    void* fileData = SDL_LoadFile(load_path, &fileSize);
    if (fileData == NULL) {
        SDL_Log("Error loading file: ",SDL_GetError()); 
        return;
    }
    defaultFragmentShader = std::string((char*)fileData, fileSize);
    SDL_free(fileData);
    pendingShaderReload = true;
    if (filter < 0) {
        SDL_Log("The current platform does not support fetching "
                "the selected filter, or the user did not select"
                " any filter.");
        return;
    } else if (filter < SDL_arraysize(file_filters)) {
        SDL_Log("The filter selected by the user is '%s' (%s).",
                file_filters[filter].pattern, file_filters[filter].name);
        return;
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
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;      // Enable Multi-Viewport // TODO: doesn't play well with editor
    io.FontGlobalScale = 1.5f;
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init();

    SDL_StartTextInput(window);
    return SDL_APP_CONTINUE;
}

/* updates on event */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event){
    ImGui_ImplSDL3_ProcessEvent(event);
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS; 
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            isLeftDown = true;
            clickX = event->button.x;
            clickY = event->button.y;
        }
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            isLeftDown = false;
        }
    }

    return SDL_APP_CONTINUE;
}

/* loop */
SDL_AppResult SDL_AppIterate(void *appstate){
    if (pendingShaderReload) {
        GLuint newProg = LoadDefaultShader();
        if (newProg != 0) {
            glDeleteProgram(shaderProgram);
            shaderProgram = newProg;
        }
        pendingShaderReload = false;
    }

    float time = SDL_GetTicks()/1000.0f; 
	int w, h;
    float x,y;
    SDL_MouseButtonFlags buttonState = SDL_GetMouseState(&x,&y);
    
	SDL_GetWindowSize(window, &w, &h);
	glViewport(0, 0, w, h);
	
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // shortcuts
    bool request_save =ImGui::Shortcut((ImGuiKey_S|ImGuiMod_Ctrl),ImGuiInputFlags_RouteGlobal);
    bool request_load = ImGui::Shortcut((ImGuiKey_O|ImGuiMod_Ctrl),ImGuiInputFlags_RouteGlobal);
    bool request_preferences = ImGui::Shortcut((ImGuiKey_Comma|ImGuiMod_Ctrl),ImGuiInputFlags_RouteGlobal);
    bool request_quit = ImGui::Shortcut((ImGuiKey_Q|ImGuiMod_Ctrl),ImGuiInputFlags_RouteGlobal);
    bool request_editor = ImGui::Shortcut((ImGuiKey_E|ImGuiMod_Ctrl),ImGuiInputFlags_RouteGlobal);
    if(ImGui::Shortcut((ImGuiKey_F12),ImGuiInputFlags_RouteGlobal)){
        show_metrics = !show_metrics;
    }
    if(show_metrics){
        ImGui::ShowMetricsWindow();
    }
    if(ImGui::BeginMainMenuBar()){
        if (ImGui::BeginMenu("File")) {
            
            if (ImGui::MenuItem("Save Shader", "Ctrl+S")||request_save) {request_save = true;}
            if (ImGui::MenuItem("Load Shader", "Ctrl+O")||request_load) {request_load = true;}
            
            ImGui::Separator();
            //if (ImGui::MenuItem("Preferences", "Ctrl+,")||request_preferences) {request_preferences=true;}//TODO
            if (ImGui::MenuItem("Quit", "Ctrl+Q")||request_quit) { request_quit=true; }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if(request_save){
        SDL_ShowSaveFileDialog(save_callback, NULL, window, file_filters, SDL_arraysize(file_filters), NULL);
    }
    if(request_load){
        SDL_ShowOpenFileDialog(load_callback, NULL, window, file_filters, SDL_arraysize(file_filters), NULL,false);
    }
    // if(request_preferences){//TODO
    //     return SDL_APP_SUCCESS;
    // }
    if(request_quit){
        return SDL_APP_SUCCESS;
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

    // shadertoy uniforms
    // uniform vec3      iResolution;           // viewport resolution (in pixels)
    glUniform3f(glGetUniformLocation(shaderProgram,"iResolution"),(float)w,(float)h,1.0f);
    CheckGLError("glUniform3f");
    // uniform float     iTime;                 // shader playback time (in seconds)
    glUniform1f(glGetUniformLocation(shaderProgram,"iTime"),time);
    CheckGLError("glUniform1f");
    // uniform float     iTimeDelta;            // render time (in seconds)
    // uniform float     iFrameRate;            // shader frame rate
    // uniform int       iFrame;                // shader playback frame
    // uniform float     iChannelTime[4];       // channel playback time (in seconds)
    // uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)
    // uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click
    // Flip Y to match Shadertoy
    float currentY = h - (float)y;
    float lastClickY = h - clickY;

    if (isLeftDown) {   
        // While button is down: xy = current pos, zw = start pos
        glUniform4f(glGetUniformLocation(shaderProgram, "iMouse"), (float)x, currentY, clickX, lastClickY);
    } else {
    // Shadertoy standard: zw becomes negative (or remains last click) when up
    // Usually, you just pass the last coordinates with a negative sign or flag
        glUniform4f(glGetUniformLocation(shaderProgram, "iMouse"), 
                (float)x, currentY, -clickX, -lastClickY);
    }
    CheckGLError("glUniform4f");
    // uniform samplerXX iChannel0..3;          // input channel. XX = 2D/Cube
    // uniform vec4      iDate;                 // (year, month, day, time in seconds)
    // uniform float     iSampleRate;           // sound sample rate (i.e., 44100)

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
