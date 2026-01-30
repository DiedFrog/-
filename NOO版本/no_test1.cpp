#include <iostream>
#include <vector>
#include <cmath>


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define Width  1600
#define Height 1200
#define PI 3.14159265358979323846

// 顶点着色器源码
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 viewPos = view * model * vec4(aPos, 1.0);
    
    // 双曲面参数：使用加法 (+) 让世界向上卷曲
    // 0.02 是一个比较适中的曲率
    float curveAmount = 0.2;
    float distanceSquared = viewPos.x * viewPos.x + viewPos.z * viewPos.z;
    
    // Y 轴随着距离平方而增加，形成“碗状”世界
    viewPos.y += distanceSquared * curveAmount;
    
    gl_Position = projection * viewPos;
}
)";

// 片段着色器源码
const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";


// 创建地面着色器
const char* groundVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    out vec3 FragPos;
    
    void main()
    {
        // 放大地面，双曲面空间需要广阔的视野才能感受到“围墙”感
        vec3 largePos = aPos;
        largePos.x *= 15.0;
        largePos.z *= 15.0;

        FragPos = vec3(model * vec4(largePos, 1.0));
        
        vec4 viewPos = view * model * vec4(largePos, 1.0);
        
        // 关键修改：+= 制造负曲率
        float curveAmount = 0.2;
        float distanceSquared = viewPos.x * viewPos.x + viewPos.z * viewPos.z;
        viewPos.y += distanceSquared * curveAmount;
        
        gl_Position = projection * viewPos;
    }
)";

const char* groundFragmentShaderSource = R"(
    #version 330 core
    in vec3 FragPos;
    out vec4 FragColor;
    
    uniform vec3 lightColor;
    uniform vec3 darkColor;
    uniform float gridSize;
    
    void main()
    {
        vec2 gridCoord = FragPos.xz / 2.0;
        vec2 gridPos = floor(gridCoord);
        
        float pattern = mod(gridPos.x + gridPos.y, 2.0);
        vec3 baseColor = mix(lightColor, darkColor, pattern);
        
        // 网格线效果
        vec2 gridLines = abs(fract(gridCoord) - 0.5) * 2.0;
        float lineWidth = 0.05;
        float lines = smoothstep(0.0, lineWidth, gridLines.x) *
                        smoothstep(0.0, lineWidth, gridLines.y);
        
        vec3 finalColor = mix(vec3(0.2), baseColor, lines);
        FragColor = vec4(finalColor, 1.0);
    }
)";
class Program {
private:
    unsigned int vertexShader;
    unsigned int fragmentShader;
    unsigned int shaderProgram;
    
public:
    // 构造函数：编译和链接着色器（使用默认的立方体着色器）
    Program() {
        createShaderProgram(vertexShaderSource, fragmentShaderSource);
    }
    
    // 使用自定义着色器源码
    Program(const char* vertexSource, const char* fragmentSource) {
        createShaderProgram(vertexSource, fragmentSource);
    }
    
    void createShaderProgram(const char* vertexSource, const char* fragmentSource) {
        // 编译顶点着色器
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        glCompileShader(vertexShader);
        
        // 检查顶点着色器编译错误
        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        
        // 编译片段着色器
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
        glCompileShader(fragmentShader);
        
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        
        // 链接着色器程序
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if(!success) {
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }
        
        // 删除着色器对象
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    
    // 获取着色器程序ID
    unsigned int getShaderProgram() const {
        return shaderProgram;
    }
    
    // 设置uniform变量
    void setVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3fv(glGetUniformLocation(shaderProgram, name.c_str()), 1, &value[0]);
    }
    
    void setFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(shaderProgram, name.c_str()), value);
    }
    
    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
};
class Camera {
private:
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    float speed;
    float yaw;   // 偏航角
    float pitch;   // 俯仰角
    float lastX;
    float lastY;
    bool firstMouse;
    void updateCameraVectors() {
        // 计算新的前向量
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }
    
public:
    Camera() {
        cameraPos = glm::vec3(0.0f, 0.0f, 4.0f);
        cameraFront = glm::vec3(0.0f, -0.3f, -1.0f);
        cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        speed = 2.5f;
        yaw = -90.0f;
        pitch = 0.0f;
        lastX = Width / 2.0f;
        lastY = Height / 2.0f;
        firstMouse = true;
    }
    
    // 获取视图矩阵
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    }
    
    // 处理键盘输入
    void ProcessKeyboard(GLFWwindow* window, float deltaTime) {
        float velocity = speed * deltaTime;
        
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += cameraFront * velocity;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= cameraFront * velocity;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            cameraPos += cameraUp * velocity;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            cameraPos -= cameraUp * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
        float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;
        
        yaw += xoffset;
        pitch += yoffset;
        
        // 限制俯仰角
        if(constrainPitch) {
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
        }
        updateCameraVectors();
    }
};

class Engine
{
private:
    GLFWwindow* window;
    Camera camera;
    float lastFrame;
    float deltaTime;

    static bool firstMouse;
    static float lastX;
    static float lastY;

    static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
        // 获取Engine实例
        void* ptr = glfwGetWindowUserPointer(window);
        if (ptr == nullptr) return;
        
        Engine* engine = static_cast<Engine*>(ptr);
        
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }
        
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // y坐标反向
        
        lastX = xpos;
        lastY = ypos;
        
        engine->camera.ProcessMouseMovement(xoffset, yoffset);
    }

public:
    
    GLFWwindow* get_window() const { return window; }
    Camera& get_camera() {return camera; }

    void init(){
        init_glfw();
        creat_window();
        init_glad();
        lastFrame = 0.0f;
    }
    void init_glfw(){
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }
    void init_glad(){
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cout<<"err: 1"<<std::endl;
        }   
    }
    void creat_window() {
        window = glfwCreateWindow(Width, Height, "TEST", NULL, NULL);
        if (window == NULL) {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return;
        }
        
        glfwMakeContextCurrent(window);
        
        // 设置窗口指针（关键！）
        glfwSetWindowUserPointer(window, this);
        
        // 设置回调
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, mouse_callback);
        
        // 隐藏光标并捕获
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    static void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
        glViewport(0, 0, w, h);
    }

    void processInput(GLFWwindow* window) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        // 计算帧时间
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // 摄像机控制
        camera.ProcessKeyboard(window, deltaTime);
    }
};
bool Engine::firstMouse = true;
float Engine::lastX = Width / 2.0f;
float Engine::lastY = Height / 2.0f;

class Object{
private:
    unsigned int VBO, VAO;
    int vertexCount;
public:
    
    Object(float* vertices, int size) {
        vertexCount = size / 3;  // 每个顶点3个float
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, size * sizeof(float), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // 析构函数：清理资源
    ~Object() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    void Draw(){
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);
    }
};

class Ground : public Object {
private:
    Program groundprogram;

    static std::vector<float> generateVertices(float size) {
        std::vector<float> vertices;
        // 定义细分步数，步数越多，弯曲越平滑
        int resolution = 500; 
        float step = (size * 2.0f) / resolution;

        for (int i = 0; i < resolution; ++i) {
            for (int j = 0; j < resolution; ++j) {
                // 计算当前格子的四个角坐标
                float x = -size + i * step;
                float z = -size + j * step;

                // 每个小格子由两个三角形组成
                // 三角形 1
                vertices.insert(vertices.end(), { x,        -1.0f, z });
                vertices.insert(vertices.end(), { x + step, -1.0f, z });
                vertices.insert(vertices.end(), { x,        -1.0f, z + step });

                // 三角形 2
                vertices.insert(vertices.end(), { x + step, -1.0f, z });
                vertices.insert(vertices.end(), { x + step, -1.0f, z + step });
                vertices.insert(vertices.end(), { x,        -1.0f, z + step });
            }
        }
        return vertices;
    }

public:
    // 构造函数：增加默认尺寸，并确保调用正确的生成逻辑
    Ground(float size = 100.0f) : Ground(size, groundVertexShaderSource, groundFragmentShaderSource) {}

    Ground(float size, const char* vertexSource, const char* fragmentSource)
        : Object(generateVertices(size).data(), (int)generateVertices(size).size()),
          groundprogram(vertexSource, fragmentSource) {}

    void SetUniform(glm::mat4& view, glm::mat4& projection) {
        glm::mat4 groundModel = glm::mat4(1.0f);
        groundprogram.setMat4("model", groundModel);
        groundprogram.setMat4("view", view);
        groundprogram.setMat4("projection", projection);
        groundprogram.setVec3("lightColor", glm::vec3(0.9f, 0.9f, 0.9f));
        groundprogram.setVec3("darkColor", glm::vec3(0.5f, 0.5f, 0.5f));
        // 格子大小可以根据需要调整，让纹理看起来更舒服
        groundprogram.setFloat("gridSize", 2.0f); 
    }

    Program& GetProgram() {
        return groundprogram;
    }
};

class Cube : public Object {
private:
    Program cubeprogram;

    // 辅助函数：生成一个细分平面的顶点
    static void addFace(std::vector<float>& vertices, glm::vec3 normal, int res) {
        // 根据法线确定两个切向量，用于在该面内移动
        glm::vec3 side1 = glm::vec3(normal.y, normal.z, normal.x);
        glm::vec3 side2 = glm::cross(normal, side1);
        float step = 1.0f / res;

        for (int i = 0; i < res; ++i) {
            for (int j = 0; j < res; ++j) {
                // 计算当前格子的四个角 (-0.5 到 0.5)
                auto getPoint = [&](int u, int v) {
                    return normal * 0.5f + (side1 * ((u * step) - 0.5f)) + (side2 * ((v * step) - 0.5f));
                };

                glm::vec3 p1 = getPoint(i, j);
                glm::vec3 p2 = getPoint(i + 1, j);
                glm::vec3 p3 = getPoint(i, j + 1);
                glm::vec3 p4 = getPoint(i + 1, j + 1);

                // 三角形 1
                vertices.insert(vertices.end(), { p1.x, p1.y, p1.z, p2.x, p2.y, p2.z, p3.x, p3.y, p3.z });
                // 三角形 2
                vertices.insert(vertices.end(), { p2.x, p2.y, p2.z, p4.x, p4.y, p4.z, p3.x, p3.y, p3.z });
            }
        }
    }

    static std::vector<float> generateSubdividedCube(int res) {
        std::vector<float> vertices;
        // 六个面的法向量
        std::vector<glm::vec3> faces = {
            {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
        };
        for (auto& f : faces) addFace(vertices, f, res);
        return vertices;
    }

public:
    // 默认细分程度 res=10，意味着每个面有 10x10=100 个小方格（200个三角形）
    Cube(int res = 10) : Object(generateSubdividedCube(res).data(), (int)generateSubdividedCube(res).size()), cubeprogram() {}

    void SetUniform(glm::mat4& cubeModel, glm::mat4& view, glm::mat4& projection) {
        cubeprogram.setMat4("model", cubeModel);
        cubeprogram.setMat4("view", view);
        cubeprogram.setMat4("projection", projection);
    }

    Program& GetProgram() {
        return cubeprogram;
    }
};


int main()
{
    Engine engine;
    engine.init();
    GLFWwindow* window = engine.get_window();
    Camera& camera = engine.get_camera();
    Cube cube[5];
    Cube cube1;
    Ground ground(20.0f,groundVertexShaderSource,groundFragmentShaderSource);
    unsigned int cubeshader[5];
    for(int i=0;i<5;i++){
        cubeshader[i] = cube[i].GetProgram().getShaderProgram();
    }
    unsigned int groundShader = ground.GetProgram().getShaderProgram();
    unsigned int cube1shader = cube1.GetProgram().getShaderProgram();

    glEnable(GL_DEPTH_TEST);

    /*主循环*/
    while(!glfwWindowShouldClose(window)){
        float time = glfwGetTime();
        engine.processInput(window);
        //渲染天空
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)Width / (float)Height, 0.1f, 100.0f);

        //渲染地面
        glUseProgram(groundShader);
        ground.SetUniform(view,projection);
        ground.Draw();

        glm::mat4 cubeModel[5];
        // 2. 渲染立方体
        for(int i=0;i<5;i++){
            glUseProgram(cubeshader[i]);
        
            // 立方体模型矩阵 - 旋转并放置在地面上方
            cubeModel[i] = glm::mat4(1.0f);
            cubeModel[i] = glm::translate(cubeModel[i], glm::vec3(0.0f, 1.0f, 0.0f));  // 放在地面中心上方
            cubeModel[i] = glm::translate(cubeModel[i],glm::vec3(5.0f*cos(2*i*PI/5.0),0.0f,-5.0f*sin(2*i*PI/5.0)));
            //cubeModel[i] = glm::rotate(cubeModel[i], time * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));
            
            // 设置立方体的uniform
            cube[i].SetUniform(cubeModel[i],view,projection);
            cube[i].Draw();
        }
        glUseProgram(cube1shader);
        glm::mat4 cube1model = glm::mat4(1.0f);
        cube1model = glm::scale(cube1model,glm::vec3(1.0f, 10.0f, 1.0f));
        cube1model = glm::translate(cube1model, glm::vec3(3.0f, 0.0f, 3.0f));
        cube1.SetUniform(cube1model,view,projection);
        cube1.Draw();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}