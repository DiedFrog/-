需要配置好glfw，glad和glm库
c_cpp_properties文件：
{
  "configurations": [
    {
      "name": "windows-gcc-x64",
      "includePath": [
        "${workspaceFolder}/**",
        "${workspaceFolder}/include",
        "${workspaceFolder}/include/glad",
        "${workspaceFolder}/include/GLFW",
        "${workspaceFolder}/include/glm"
      ],
      "compilerPath": "C:/msys64/ucrt64/bin/gcc.exe",
      "cStandard": "${default}",
      "cppStandard": "${default}",
      "intelliSenseMode": "windows-gcc-x64",
      "compilerArgs": [
        ""
      ]
    }
  ],
  "version": 4
}
tasks文件配置：
{
    "tasks": [
        {
            "type": "cppbuild",
            "label": "C/C++: g++.exe 生成活动文件",
            "command": "C:/msys64/ucrt64/bin/g++.exe",
            "args": [
                "-fdiagnostics-color=always",
                "-g",
                "${workspaceFolder}/src/*.cpp",
                "${workspaceFolder}/src/glad.c",
                "-o",
                "${workspaceFolder}\\bin\\main.exe",
                "-I${workspaceFolder}/include",
                "-L${workspaceFolder}/lib",
                "-lglfw3",
                "-lopengl32",
                "-lgdi32"
            ],
            "options": {
                "cwd": "${workspaceFolder}"
            },
            "problemMatcher": [
                "$gcc"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "detail": "调试器生成的任务。"
        }
    ],
    "version": "2.0.0"
}
