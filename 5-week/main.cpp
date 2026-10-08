#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>

const int WIDTH  = 1000;
const int HEIGHT = 720;

// ---------------------------------------------------------------------
//  Режимдер (пернелер 1-4):
//    1 — базалық бөлім: шеңбер бойымен қозғалатын төртбұрыш (EBO)
//    2 — 1-тапсырма: индекстер {0,1,2}, count = 3 (бір үшбұрыш)
//    3 — 2-тапсырма: бесбұрыш (5 вершина, 3 үшбұрыш, 9 индекс)
//    4 — 3-тапсырма: төртбұрыш екі рет (бір VAO, екі draw)
//  Tab — wireframe қосу/өшіру (қосымша)
// ---------------------------------------------------------------------
int  mode = 1;
bool wireframeMode = false;

// ---------------------------------------------------------------------
//  Шейдерлер: offset uniform (4-аптадағы анимация сол күйінде)
// ---------------------------------------------------------------------
const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform vec2 uOffset;
out vec3 vColor;
void main() {
    gl_Position = vec4(aPos.xy + uOffset, aPos.z, 1.0);
    vColor = aColor;
}
)";

const char* fragmentSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() { FragColor = vec4(vColor, 1.0); }
)";

unsigned int compileShader(GLenum type, const char* src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Шейдер қатесі:\n" << log << "\n";
    }
    return s;
}

// ---------------------------------------------------------------------
//  Mesh: VAO + VBO + EBO. EBO VAO байланған кезде жасалады (ең маңызды!)
// ---------------------------------------------------------------------
struct Mesh {
    unsigned int vao, vbo, ebo;
    int indexCount;
};

Mesh createMesh(const float* vertices, size_t vertexBytes,
                const unsigned int* indices, size_t indexBytes) {
    Mesh m{};
    m.indexCount = (int)(indexBytes / sizeof(unsigned int));

    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ebo);

    glBindVertexArray(m.vao);                              // VAO бірінші

    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexBytes, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);          // VAO ішінде
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBytes, indices, GL_STATIC_DRAW);

    const int STRIDE = 6 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    // ЕСКЕРТУ: GL_ELEMENT_ARRAY_BUFFER-ді 0-ге босатпаймыз — ол VAO күйінің бөлігі.
    return m;
}

void destroyMesh(Mesh& m) {
    glDeleteVertexArrays(1, &m.vao);
    glDeleteBuffers(1, &m.vbo);
    glDeleteBuffers(1, &m.ebo);
}

void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    static bool tabWasDown = false;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) mode = 1;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) mode = 2;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) mode = 3;
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) mode = 4;

    // Tab: бір басылғанда бір рет ауыстыру
    bool tabDown = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
    if (tabDown && !tabWasDown) wireframeMode = !wireframeMode;
    tabWasDown = tabDown;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW іске қосылмады\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "5-апта: EBO",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "Терезе жасалмады\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(1);

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    // -----------------------------------------------------------------
    //  1. Вершина деректері: 4 вершина (6 емес), әрқайсысы 6 float
    // -----------------------------------------------------------------
    float vertices[] = {
        // позиция          // түс
         0.3f,  0.3f, 0.0f,  1.0f, 0.0f, 0.0f,   // 0 — оң жоғарғы
         0.3f, -0.3f, 0.0f,  0.0f, 1.0f, 0.0f,   // 1 — оң төменгі
        -0.3f, -0.3f, 0.0f,  0.0f, 0.0f, 1.0f,   // 2 — сол төменгі
        -0.3f,  0.3f, 0.0f,  1.0f, 1.0f, 0.0f    // 3 — сол жоғарғы
    };

    // 2. Индекс массиві: диагональ 1–3, сондықтан 1 мен 3 екі рет
    unsigned int indices[] = {
        0, 1, 3,    // бірінші үшбұрыш
        1, 2, 3     // екінші үшбұрыш
    };

    // 1-тапсырма: бұзылған индекстер — тек {0,1,2}, count = 3
    unsigned int brokenIndices[] = { 0, 1, 2 };

    // 2-тапсырма: бесбұрыш. Бұрыштар −90°, −18°, 54°, 126°, 198°.
    // 5 вершина, 3 үшбұрыш "желпуіш" (fan): 0 вершина — ортақ.
    //        2
    //     3     1        нөмірлеу сағат тіліне қарсы
    //       4   0
    float pentagon[5 * 6];
    const float pentColors[5][3] = {
        {1.0f, 0.2f, 0.2f}, {1.0f, 0.8f, 0.1f}, {0.2f, 1.0f, 0.3f},
        {0.2f, 0.8f, 1.0f}, {0.8f, 0.3f, 1.0f}
    };
    const float PI = 3.14159265358979f;
    for (int i = 0; i < 5; ++i) {
        float deg = -90.0f + 72.0f * (float)i;
        float a = deg * PI / 180.0f;
        pentagon[i * 6 + 0] = std::cos(a) * 0.3f;
        pentagon[i * 6 + 1] = std::sin(a) * 0.3f;
        pentagon[i * 6 + 2] = 0.0f;
        pentagon[i * 6 + 3] = pentColors[i][0];
        pentagon[i * 6 + 4] = pentColors[i][1];
        pentagon[i * 6 + 5] = pentColors[i][2];
    }
    unsigned int pentIndices[] = {
        0, 1, 2,
        0, 2, 3,
        0, 3, 4
    };

    Mesh quad   = createMesh(vertices, sizeof(vertices), indices, sizeof(indices));
    Mesh broken = createMesh(vertices, sizeof(vertices), brokenIndices, sizeof(brokenIndices));
    Mesh pent   = createMesh(pentagon, sizeof(pentagon), pentIndices, sizeof(pentIndices));

    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    unsigned int shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);
    glDeleteShader(vs);
    glDeleteShader(fs);
    int locOffset = glGetUniformLocation(shader, "uOffset");

    std::cout << "1 — төртбұрыш (EBO), 2 — индекс {0,1,2}, 3 — бесбұрыш, "
                 "4 — екі төртбұрыш, Tab — wireframe\n";

    // -----------------------------------------------------------------
    //  Негізгі цикл
    // -----------------------------------------------------------------
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, wireframeMode ? GL_LINE : GL_FILL);

        float angle = (float)glfwGetTime();
        float ox = std::cos(angle) * 0.4f;
        float oy = std::sin(angle) * 0.4f;

        glUseProgram(shader);

        if (mode == 1) {
            glUniform2f(locOffset, ox, oy);
            glBindVertexArray(quad.vao);
            glDrawElements(GL_TRIANGLES, quad.indexCount, GL_UNSIGNED_INT, 0);
        } else if (mode == 2) {
            glUniform2f(locOffset, ox, oy);
            glBindVertexArray(broken.vao);
            glDrawElements(GL_TRIANGLES, broken.indexCount, GL_UNSIGNED_INT, 0);
        } else if (mode == 3) {
            glUniform2f(locOffset, ox, oy);
            glBindVertexArray(pent.vao);
            glDrawElements(GL_TRIANGLES, pent.indexCount, GL_UNSIGNED_INT, 0);
        } else {
            // 3-тапсырма: бір VAO, екі uniform + екі draw. Геометрия қайта жіберілмейді.
            glBindVertexArray(quad.vao);
            glUniform2f(locOffset, -0.4f, 0.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glUniform2f(locOffset, 0.4f, 0.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    destroyMesh(quad);
    destroyMesh(broken);
    destroyMesh(pent);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}
