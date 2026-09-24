
#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <vector>

const int WIDTH  = 1000;
const int HEIGHT = 720;

// Пробел басылғанда true болады (1-апта, 3-тапсырма)
bool whiteBackground = false;

// 2-апта, 3-тапсырма: F — wireframe қосу/өшіру
// (сеткадағы әр ұяшықтың 2 үшбұрышқа бөлінгенін көру үшін ыңғайлы)
bool wireframeMode = false;

// ---------------------------------------------------------------------
//  Сетка: N x M ұяшық. Әр ұяшық — квадрат, ол диагональ бойынша
//  2 үшбұрышқа бөлінеді (GPU тек үшбұрыш сала алады).
// ---------------------------------------------------------------------
const int GRID_COLS = 10;
const int GRID_ROWS = 10;

// Ұяшықтың тек 2-үшбұрышы ғана боялады (colorB), 1-үшбұрыш
// мүлдем салынбайды — сол орында фон көрініп тұрады. Сол себепті
// вершиналарды ортақ пайдалана алмаймыз (EBO жарамайды): бір ғана
// үшбұрыш салынатын болғандықтан, әр ұяшыққа тек 3 вершина (боялатын
// үшбұрышқа тиесілі) шығарамыз, әр вершинаға позиция (x,y,z) +
// түс (r,g,b) — 6 float қосамыз.
const float colorB[3] = { 0.95f, 0.45f, 0.15f }; // боялатын үшбұрыш — қызғылт-сары

std::vector<float> buildGridVertices(int cols, int rows) {
    std::vector<float> verts;
    verts.reserve((size_t)cols * rows * 3 * 6);

    const float left = -0.9f, right = 0.9f;
    const float bottom = -0.9f, top = 0.9f;

    auto pushVertex = [&verts](float x, float y, const float* color) {
        verts.push_back(x);
        verts.push_back(y);
        verts.push_back(0.0f);
        verts.push_back(color[0]);
        verts.push_back(color[1]);
        verts.push_back(color[2]);
    };

    for (int row = 0; row < rows; ++row) {
        float y0 = bottom + (top - bottom) * (float)row / (float)rows;
        float y1 = bottom + (top - bottom) * (float)(row + 1) / (float)rows;
        for (int col = 0; col < cols; ++col) {
            float x0 = left + (right - left) * (float)col / (float)cols;
            float x1 = left + (right - left) * (float)(col + 1) / (float)cols;

            // 1-үшбұрыш (жоғарғы-сол, төменгі-сол, жоғарғы-оң) — боялмайды,
            // сондықтан мұнда мүлдем салынбайды.

            // 2-үшбұрыш: жоғарғы-оң, төменгі-сол, төменгі-оң — colorB
            pushVertex(x1, y1, colorB);
            pushVertex(x0, y0, colorB);
            pushVertex(x1, y0, colorB);
        }
    }
    return verts;
}

// ---------------------------------------------------------------------
//  2-апта: уақытша шейдерлер (3-аптада жақсартамыз)
// ---------------------------------------------------------------------
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

const char* fragmentSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() { FragColor = vec4(vColor, 1.0); }
)";

// ---------------------------------------------------------------------
//  Терезе өлшемі өзгергенде шақырылады
// ---------------------------------------------------------------------
void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

// ---------------------------------------------------------------------
//  Пернетақтаны тексеру. Әр кадрда шақырылады.
// ---------------------------------------------------------------------
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        whiteBackground = true;
    }
    // 2-апта, 3-тапсырма: wireframe қосу (сетканың ұяшықтарын көру үшін)
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        wireframeMode = true;
    }
}

// =====================================================================
//  MAIN
// =====================================================================
int main() {

    // -----------------------------------------------------------------
    //  1. GLFW-ны іске қосу
    // -----------------------------------------------------------------
    if (!glfwInit()) {
        std::cerr << "GLFW іске қосылмады\n";
        return -1;
    }

    // Қандай OpenGL нұсқасы керек екенін айтамыз.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    // -----------------------------------------------------------------
    //  2. Терезе жасау
    // -----------------------------------------------------------------
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
                                          "Компьютерлік графика",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "Терезе жасалмады. Видеокарта OpenGL 3.3-ті "
                     "қолдамауы мүмкін.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);              // осы терезенің контексі белсенді
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(0);                         // VSync өшірулі (1-апта, 4-тапсырма)

    // -----------------------------------------------------------------
    //  3. GLAD: OpenGL функцияларын жүктеу
    //     Контекст белсенді болғаннан КЕЙІН ғана. Ретін бұзсаң — бәрі құлайды.
    // -----------------------------------------------------------------
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    // -----------------------------------------------------------------
    //  2-апта: Сетка (VBO + VAO), әр вершинада позиция + түс
    // -----------------------------------------------------------------
    std::vector<float> gridVertices = buildGridVertices(GRID_COLS, GRID_ROWS);
    int gridVertexCount = (int)(gridVertices.size() / 6); // 6 float әр вершинаға

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);                                  // VAO байлаймыз
    glBindBuffer(GL_ARRAY_BUFFER, vbo);                      // VBO байлаймыз
    glBufferData(GL_ARRAY_BUFFER,
                 gridVertices.size() * sizeof(float),
                 gridVertices.data(), GL_STATIC_DRAW);

    // Әр вершина: [x, y, z, r, g, b] — қадам 6 float
    // location=0: позиция (3 float, ығысу 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // location=1: түс (3 float, ығысу 3 float)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Шейдерлерді компиляциялау (уақытша, 3-аптада жақсартамыз)
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexSrc, nullptr);
    glCompileShader(vs);

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentSrc, nullptr);
    glCompileShader(fs);

    unsigned int shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // -----------------------------------------------------------------
    //  4. Негізгі цикл
    // -----------------------------------------------------------------
    int frameCount = 0;
    double lastFpsTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {

        processInput(window);

        // --- Экранды тазалау ---
        if (whiteBackground) {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            float t = (float)glfwGetTime();
            float r = (std::sin(t * 3.0f) + 1.0f) * 0.5f * 0.3f;
            float g = (std::sin(t * 2.0f) + 1.0f) * 0.5f * 0.3f;
            glClearColor(r, g, 0.35f, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT);

        // --- 2-апта, 3-тапсырма: wireframe ---
        glPolygonMode(GL_FRONT_AND_BACK, wireframeMode ? GL_LINE : GL_FILL);

        // --- Сетканы салу ---
        glUseProgram(shader);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, gridVertexCount);

        // --- FPS есептеу (1-апта, 4-тапсырма) ---
        frameCount++;
        double now = glfwGetTime();
        if (now - lastFpsTime >= 1.0) {
            std::cout << "FPS: " << frameCount << "\n";
            frameCount = 0;
            lastFpsTime = now;
        }

        glfwSwapBuffers(window);   // дайын кадрды экранға шығару
        glfwPollEvents();          // пернетақта/тінтуір оқиғаларын өңдеу
    }

    // -----------------------------------------------------------------
    //  5. Тазалау
    // -----------------------------------------------------------------

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}


// =====================================================================
//  2-АПТА ТАПСЫРМАЛАРЫ
// =====================================================================
//  1. Екінші үшбұрыш қосу (бір массивте, 6 вершина) — орындалды,
//     кейін төмендегі сеткаға дейін кеңейтілді.
//
//  2. GL_TRIANGLES орнына GL_LINE_LOOP қойып көру — жеке үшбұрыш
//     мысалында тексерілді (толық сеткада мағынасы жоқ, себебі
//     бір glDrawArrays шақыруында бәрі бір LINE_LOOP-қа тұйықталар еді).
//
//  3. Wireframe: "F" пернесін бас — glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)
//     іске қосылады. Сетканың әр ұяшығындағы диагональ (2 үшбұрышқа
//     бөлінуі) осы режимде анық көрінеді.
//
//  4. Үшбұрыштың түсі өзгертілді: fragmentSrc ішінде
//     vec4(1.0, 0.5, 0.2, 1.0) [қызғылт-сары] орнына
//     vec4(0.2, 0.6, 0.9, 1.0) [көк] қойылды.
// =====================================================================


// =====================================================================
//  СЕТКА (N x M) КЕҢЕЙТУІ
// =====================================================================
//  GRID_COLS x GRID_ROWS өлшемді сетка құрылды (жоғарыда өзгертуге
//  болады). Әр ұяшық — квадрат, ол диагональ бойынша 2 үшбұрышқа
//  бөлінеді:
//
//      topLeft ---- topRight
//         |  \          |
//         |    \        |
//         |      \      |
//      bottomLeft -- bottomRight
//
//  Вершиналар buildGridVertices()-те генерацияланады.
// =====================================================================


// =====================================================================
//  ҰЯШЫҚТАҒЫ БІР ҮШБҰРЫШТЫ ҒАНА БОЯУ
// =====================================================================
//  Әр ұяшықтың 1-үшбұрышы мүлдем салынбайды (сол орында фон
//  көрінеді), ал 2-үшбұрышы colorB (қызғылт-сары) түсінде — бүкіл
//  сеткада бірдей, диагональ бойымен қайталанатын өрнек шығады.
//
//  Неге EBO (индекстер) енді жарамайды: көршілес ұяшықтар бір
//  бұрыштық нүктені ортақ пайдаланады, ал енді әр ұяшыққа тек 3
//  вершина (боялатын үшбұрышқа тиесілі) керек — олар индекспен
//  ортақ пайдалануға жарамайды. Сондықтан әр ұяшыққа арналған 3
//  вершинаны тікелей жазамыз (EBO-сыз, glDrawArrays арқылы), әр
//  вершинада позициямен қатар түс те (location=1, vec3) беріледі.
// =====================================================================
