#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>

const int WIDTH  = 1280;
const int HEIGHT = 720;

float speed = 1.5f;          // радиан/секунд
bool useDt = true;           // 1-тапсырма: D пернесі -> dt-сыз салыстыру

void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

bool dKeyWasPressed = false;

void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    bool dPressed = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    if (dPressed && !dKeyWasPressed) {
        useDt = !useDt;
        std::cout << "useDt = " << useDt
                   << "  (oshirilgende FPS-ke tauelді bolady - 1-tapsyrma)\n";
    }
    dKeyWasPressed = dPressed;

    // 3-тапсырма: W/S — жылдамдықты басқару (мұнда да dt керек)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        speed += 1.0f * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        speed -= 1.0f * dt;
        if (speed < 0.0f) speed = 0.0f;
    }
}

unsigned int makeShader(const char* vsSrc, const char* fsSrc) {
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSrc, nullptr);
    glCompileShader(vs);
    int ok;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[1024]; glGetShaderInfoLog(vs, 1024, nullptr, log); std::cerr << log << "\n"; }

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSrc, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[1024]; glGetShaderInfoLog(fs, 1024, nullptr, log); std::cerr << log << "\n"; }

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { char log[1024]; glGetProgramInfoLog(prog, 1024, nullptr, log); std::cerr << log << "\n"; }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// --- шейдерлер: позиция + түс, uOffset (орын ауыстыру) және uScale (пульсация, 2-тапсырма) ---
const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform vec2 uOffset;
uniform float uScale;

out vec3 vColor;

void main() {
    vec2 scaledPos = aPos.xy * uScale;
    gl_Position = vec4(scaledPos + uOffset, aPos.z, 1.0);
    vColor = aColor;
}
)";

const char* fragmentSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() { FragColor = vec4(vColor, 1.0); }
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
                                          "Seminar 4 - shenber boiynsha qozgalys",
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
    std::cout << "Pernelar: D - dt-ni osh./qos. (1-tapsyrma), W/S - zhyldamdyqty basqaru (3-tapsyrma), ESC - shygu\n";

    float vertices[] = {
        -0.15f, -0.15f, 0.0f,   1.0f, 0.0f, 0.0f,
         0.15f, -0.15f, 0.0f,   0.0f, 1.0f, 0.0f,
         0.0f,   0.15f, 0.0f,   0.0f, 0.0f, 1.0f
    };

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    unsigned int shader = makeShader(vertexSrc, fragmentSrc);

    // locOffset неге бір рет: ат бойынша іздеу бос шығын, циклден бұрын тек бір рет табамыз
    int locOffset = glGetUniformLocation(shader, "uOffset");
    int locScale  = glGetUniformLocation(shader, "uScale");

    float lastFrame = (float)glfwGetTime();   // НАЗАР: нөл емес
    float angle = 0.0f;

    while (!glfwWindowShouldClose(window)) {

        // --- уақыт ---
        float now = (float)glfwGetTime();
        float dt = now - lastFrame;
        lastFrame = now;

        // --- ЖАҢАРТУ: күйді есептеу ---
        processInput(window, dt);

        if (useDt) {
            angle += speed * dt;          // дұрыс: секундқа тәуелді
        } else {
            angle += 0.02f;               // 1-тапсырма: dt-сыз -> FPS-ке тәуелді болады
        }

        float scale = 1.0f + 0.3f * std::sin((float)glfwGetTime() * 2.0f); // 2-тапсырма: пульсация

        // --- СЫЗУ: күйді көрсету ---
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);             // АЛДЫМЕН осы, содан кейін uniform
        glUniform2f(locOffset, std::cos(angle) * 0.4f, std::sin(angle) * 0.4f);
        glUniform1f(locScale, scale);

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

