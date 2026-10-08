#include <glad/gl.h>
#include <GLFW/glfw3.h>
 
#include <cmath>
#include <iostream>
 
const int WIDTH  = 1280;
const int HEIGHT = 720;
 
bool invertColors = false;   // 1-тапсырма: I пернесі -> 1.0 - vColor
bool iKeyWasPressed = false;
 
bool sameColor = false;      // 2-тапсырма: U пернесі -> үшеуіне бір түс (интерполяция жоғалады)
bool uKeyWasPressed = false;
 
void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}
 
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
 
    bool iPressed = glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS;
    if (iPressed && !iKeyWasPressed) {
        invertColors = !invertColors;
        std::cout << "invertColors = " << invertColors << "\n";
    }
    iKeyWasPressed = iPressed;
 
    bool uPressed = glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS;
    if (uPressed && !uKeyWasPressed) {
        sameColor = !sameColor;
        std::cout << "sameColor = " << sameColor
                   << "  (uшеуіне де бір түс - интерполяция көрінбейді)\n";
    }
    uKeyWasPressed = uPressed;
}
 
// -----------------------------------------------------------------
//  makeShader() — компиляция мен линковканы қате тексерумен жасайды
// -----------------------------------------------------------------
unsigned int makeShader(const char* vsSrc, const char* fsSrc) {
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSrc, nullptr);
    glCompileShader(vs);
 
    int ok;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(vs, 1024, nullptr, log);
        std::cerr << "VERTEX SHADER QATESI:\n" << log << "\n";
    }
 
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSrc, nullptr);
    glCompileShader(fs);
 
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(fs, 1024, nullptr, log);
        std::cerr << "FRAGMENT SHADER QATESI:\n" << log << "\n";
    }
 
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
 
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        std::cerr << "LINK QATESI:\n" << log << "\n";
    }
 
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}
 
// --- шейдерлер: позиция + түс атрибуты, интерполяция демонстрациясы үшін ---
const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
 
out vec3 vColor;
 
void main() {
    gl_Position = vec4(aPos, 1.0);
    vColor = aColor;
}
)";
 
// 3-тапсырма үшін: осы жолды "vec4(vColr, 1.0)" деп әдейі қате жазып көр —
// терминалда дәл қай жолда, не себепті қате екенін көресің.
const char* fragmentSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
 
uniform bool uInvert;
uniform bool uSameColor;
 
void main() {
    vec3 c = vColor;
    if (uSameColor) {
        c = vec3(1.0, 0.5, 0.2); // барлық фрагмент осы бір түс - градиент жоғалады
    }
    if (uInvert) {
        c = 1.0 - c;
    }
    FragColor = vec4(c, 1.0);
}
)";
 
int main() {
 
    if (!glfwInit()) {
        std::cerr << "GLFW iske qosylmady\n";
        return -1;
    }
 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
 
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
                                          "Seminar 3 - tusti ushburysh",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "Tereze zhasalmady.\n";
        glfwTerminate();
        return -1;
    }
 
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(1);
 
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD zhuktelmedi\n";
        glfwTerminate();
        return -1;
    }
 
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";
    std::cout << "Pernelar: I - tusti toнkeru, U - birdei tus (interpolyaciyany salystyru), ESC - shygu\n";
 
    // -----------------------------------------------------------------
    //  Вершина деректері: позиция (3 float) + түс (3 float) = 6 float
    // -----------------------------------------------------------------
    float vertices[] = {
        // позиция            // түс
        -0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,  // қызыл
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,  // жасыл
         0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f   // көк
    };
 
    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
 
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
 
    // location=0: позиция, stride 6 float, ығысу 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
 
    // location=1: түс, stride 6 float, ығысу 3 float
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
 
    glBindVertexArray(0);
 
    unsigned int shader = makeShader(vertexSrc, fragmentSrc);
 
    int locInvert = glGetUniformLocation(shader, "uInvert");
    int locSame   = glGetUniformLocation(shader, "uSameColor");
 
    while (!glfwWindowShouldClose(window)) {
 
        processInput(window);
 
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
 
        glUseProgram(shader);              // ЕҢ АЛДЫМЕН осы, содан кейін uniform
        glUniform1i(locInvert, invertColors);
        glUniform1i(locSame, sameColor);
 
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
 
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
 
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shader);
 
    glfwTerminate();
    return 0;
}
 
