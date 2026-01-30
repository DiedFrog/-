#include <iostream>
#include <vector>
#include <cmath>


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define Width  800
#define Height 600
#define PI 3.14159265358979323846

// 顶点着色器源码
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;      // 模型矩阵: 物体的位置、旋转、缩放
uniform mat4 view;       // 视图矩阵: 摄像机位置
uniform mat4 projection; // 投影矩阵: 3D到2D的投影

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
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
        FragPos = vec3(model * vec4(aPos, 1.0));
        gl_Position = projection * view * model * vec4(aPos, 1.0);
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
        vec2 gridCoord = FragPos.xz / gridSize;
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

class Ground : public Object{
private:
    Program groundprogram;
    
    static std::vector<float> generateVertices(float size) {
        std::vector<float> vertices = {
            // 第一个三角形
            -size, -1.0f, -size,  // 左下
             size, -1.0f, -size,  // 右下
            -size, -1.0f,  size,  // 左上
            
            // 第二个三角形
             size, -1.0f, -size,  // 右下
             size, -1.0f,  size,  // 右上
            -size, -1.0f,  size   // 左上
        };
        return vertices;
    }

public:
    // 创建一个平面地面
    Ground(float size = 20.0f):Ground(20.0f,vertexShaderSource,fragmentShaderSource) {}

    Ground(float size, const char* vertexSource, const char* fragmentSource)
        : Object(generateVertices(size).data(), generateVertices(size).size()),
          groundprogram(vertexSource, fragmentSource) {}
    
    void SetUniform(glm::mat4& view,glm::mat4& projection){
        glm::mat4 groundModel = glm::mat4(1.0f);

        groundprogram.setMat4("model", groundModel);
        groundprogram.setMat4("view", view);
        groundprogram.setMat4("projection", projection);
        groundprogram.setVec3("lightColor", glm::vec3(0.9f, 0.9f, 0.9f));
        groundprogram.setVec3("darkColor", glm::vec3(0.5f, 0.5f, 0.5f));
        groundprogram.setFloat("gridSize", 1.0f);
    }

    Program& GetProgram() {
        return groundprogram;
    }
};

class Cube : public Object{
private:
    inline static float cubeVertices[108] = {
        // 后面
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        
        // 前面
        -0.5f, -0.5f,  0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        
        // 左面
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        
        // 右面
        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        
        // 底面
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,
        
        // 顶面
        -0.5f,  0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f
    };
    Program cubeprogram;
public:
    Cube() : Object(cubeVertices,108),cubeprogram(){}

    void SetUniform(glm::mat4& cubeModel,glm::mat4& view,glm::mat4& projection){

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
    Ground ground(20.0f,groundVertexShaderSource,groundFragmentShaderSource);
    unsigned int cubeshader[5];
    for(int i=0;i<5;i++){
        cubeshader[i] = cube[i].GetProgram().getShaderProgram();
    }
    unsigned int groundShader = ground.GetProgram().getShaderProgram();


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
            cubeModel[i] = glm::translate(cubeModel[i], glm::vec3(0.0f, 0.0f, 0.0f));  // 放在地面中心上方
            cubeModel[i] = glm::translate(cubeModel[i],glm::vec3(5.0f*cos(2*i*PI/5.0),0.0f,-5.0f*sin(2*i*PI/5.0)));
            cubeModel[i] = glm::rotate(cubeModel[i], time * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));
            
            // 设置立方体的uniform
            cube[i].SetUniform(cubeModel[i],view,projection);
            cube[i].Draw();
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
