## 2-апта. Үшбұрыш (VBO + VAO)

**Қосылады:** вершина деректері, буферлер, уақытша шейдерлер.

`main` ішіндегі `=== 2-АПТА ===` белгісінің орнына:

```cpp
// Вершина деректері — NDC координаттарында (-1 .. 1)
float vertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
};

unsigned int vao, vbo;
glGenVertexArrays(1, &vao);
glGenBuffers(1, &vbo);

glBindVertexArray(vao);                                  // VAO байлаймыз
glBindBuffer(GL_ARRAY_BUFFER, vbo);                      // VBO байлаймыз
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),          // деректі GPU-ға
             vertices, GL_STATIC_DRAW);

// GPU-ға байттарды қалай оқу керегін түсіндіреміз:
// location=0, 3 float, нормаланбаған, қадам 3 float, ығысу 0
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

glBindVertexArray(0);
```

Шейдерлер әзірге ең қарапайым күйде — файлдың басына, `main`-нен бұрын:

```cpp
const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
void main() { gl_Position = vec4(aPos, 1.0); }
)";

const char* fragmentSrc = R"(
#version 330 core
out vec4 FragColor;
void main() { FragColor = vec4(1.0, 0.5, 0.2, 1.0); }
)";
```

Компиляция (уақытша, 3-аптада жақсартамыз) — `=== 3-АПТА ===` орнына:

```cpp
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
```

Циклдің ішінде, `glClear`-дан кейін:

```cpp
glUseProgram(shader);
glBindVertexArray(vao);
glDrawArrays(GL_TRIANGLES, 0, 3);
```

Тазалау бөлімінде:

```cpp
glDeleteVertexArrays(1, &vao);
glDeleteBuffers(1, &vbo);
glDeleteProgram(shader);
```

**Тапсырмалар**
1. Екінші үшбұрыш қос (бір массивте, 6 вершина).
2. `GL_TRIANGLES` орнына `GL_LINE_LOOP` қойып көр.
3. `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)` — wireframe.
4. Үшбұрыштың түсін өзгерт.

---