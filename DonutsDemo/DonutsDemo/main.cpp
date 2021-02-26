//
//  main.cpp
//  DonutsDemo
//
//  Created by 王海龙 on 2021/2/26.
//
//GLTool.h头文件包含了大部分GLTool中类似C语言的独立函数
#include "GLTools.h"
//矩阵的工具类，可以利用GLMatrixStack加载单元矩阵/矩阵/矩阵相乘/压栈/出栈/缩放/平移/旋转
#include "GLMatrixStack.h"
//矩阵工具类，表示位置，通过设置vOrigin、vForward、vUp来表示位置
#include "GLFrame.h"
/*
 矩阵工具类，用来快速设置正/透视投影矩阵，完成坐标从3D->2D映射的转换
 */
#include "GLFrustum.h"
//变换管道类，用来快速在代码中传输视图矩阵/投影矩阵/视图投影变换矩阵等
#include "GLGeometryTransform.h"
//三角形批次类、帮助类，利用它可以传输顶点/光照/纹理/颜色数据到存储着色器中
#include <GLBatch.h>

//数学库
#include <math.h>
#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

//设置角色帧，作为相机（观察者视角）
GLFrame   viewFrame;
//使用GLFrustum 来设置透视投影
GLFrustum  viewFrustum;
GLTriangleBatch  torusBath; //圆圈批次类
GLMatrixStack   modelViewMatrix; //模型矩阵 存储矩阵的变化，旋转、移动等变化
GLMatrixStack   projectionMatrix; //透视矩阵 存储投影方式  最终是要对顶点进行操作例如矩阵相乘等
GLGeometryTransform  transformPipeline;//变换管道，存储模型视图/投影/模型视图投影矩阵
GLShaderManager  shaderManager;//存储着色器管理工具类

//标记：背面剔除、深度测试
int iCull = 0;
int iDepth = 0;

//渲染场景
void RenderScene() {
    //1.清除窗口和深度缓冲区
    //可以尝试一下不清空颜色/深度缓冲区时.渲染会造成什么问题. 残留数据
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //开启/关闭正背面剔除功能
    if (iCull) {
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
    }else {
        glDisable(GL_CULL_FACE);
    }
    //开启深度测试  根据设置iDepth标记来判断是否开启深度测试
    if (iDepth) {
        glEnable(GL_DEPTH_TEST);
    }else {
        glDisable(GL_DEPTH_TEST);
    }
   
    /* 如果有ZFighting 闪烁问题 打开多边形偏移
     因为开启深度测试后,OpenGL 就不不会再去绘制模型被遮挡的部分. 这样实现的显示更更加真实.但是 由于深度缓冲区精度的限制对于深度相差⾮非常⼩小的情况下.(例例如在同⼀一平⾯面上进⾏行行2次 制),OpenGL 就可能出现不不能正确判断两者的深度值,会导致深度测试的结果不不可预测.显示出来的 现象时交错闪烁.的前⾯面2个画⾯面,交错出现.
     1.启⽤用Polygon Offset ⽅方式 glEnable(GL_POLYGON_OFFSET_FILL)
    参数列列表:
     GL_POLYGON_OFFSET_POINT   对应光栅化模式: GL_POINT
     GL_POLYGON_OFFSET_LINE    对应光栅化模式: GL_LINE
     GL_POLYGON_OFFSET_FILL    对应光栅化模式: GL_FILL
     
    2. 通过glPolygonOffset 来指定.glPolygonOffset 需要2个参数: factor , units 每个Fragment 的深度值都会增加如下所示的偏移量量:
     Offset = ( m * factor ) + ( r * units);
     m : 多边形的深度的斜率的最⼤大值,理理解⼀一个多边形越是与近裁剪⾯面平⾏行行,m 就越接近于0.
     r : 能产⽣生于窗⼝口坐标系的深度值中可分辨的差异最⼩小值.r 是由具体是由具体OpenGL 平台指定的 ⼀一个常量量.
     ⼀一个⼤大于0的Offset 会把模型推到离你(摄像机)更更远的位置,相应的⼀一个⼩小于0的Offset 会把模型拉 近
     ⼀一般⽽而⾔言,只需要将-1.0 和 -1 这样简单赋值给glPolygonOffset 基本可以满⾜足需求.
         
     void glPolygonOffset(Glfloat factor,Glfloat units);
     应⽤用到⽚片段上总偏移计算⽅方程式:
     Depth Offset = (DZ * factor) + (r * units); DZ:深度值(Z值)
     r:使得深度缓冲区产⽣生变化的最⼩小值
     负值，将使得z值距离我们更更近，⽽而正值，将使得z值距离我们更更远， 对于上节课的案例例，我们设置factor和units设置为-1，-1
     
     关闭多边形偏移
     glDisable(GL_POLYGON_OFFSET_FILL)
     */
    
    glPolygonOffset(-1, -1);

    
    //2.把观察者矩阵压入模型矩阵中
    modelViewMatrix.PushMatrix(viewFrame);
    //3.设置绘图颜色
    GLfloat vRed[] = {1.0f,0.0f,0.0f,1.0f};
    //4.
    //使用平面着色器
    //参数1：平面着色器
    //参数2：模型视图投影矩阵
    //参数3：颜色
   // shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipeline.GetModelViewProjectionMatrix(), vRed);
    
    //使用默认光源着色器
    //通过光源、阴影效果跟提现立体效果
    //参数1：GLT_SHADER_DEFAULT_LIGHT 默认光源着色器
    //参数2：模型视图矩阵
    //参数3：投影矩阵
    //参数4：基本颜色值
    shaderManager.UseStockShader(GLT_SHADER_DEFAULT_LIGHT,transformPipeline.GetModelViewMatrix(),transformPipeline.GetProjectionMatrix(),vRed);
    //5.绘制
    torusBath.Draw();
    //6.出栈  绘制完成回复
    modelViewMatrix.PopMatrix();
    //7.交换缓冲区
    glutSwapBuffers();
    
}

void SetupRC(){
    //1.设置背景颜色
    glClearColor(0.3f, 0.3f, 0.3f, 1.0);
    //2.初始化着色器管理器
    shaderManager.InitializeStockShaders();
    //3.将相机向后移动7个单元：肉眼到物体之间的距离  观察者动物体不动的形式
    viewFrame.MoveForward(10.0);
    //4.创建一个甜甜圈
    //void gltMakeTorus(GLTriangleBatch& torusBatch, GLfloat majorRadius, GLfloat minorRadius, GLint numMajor, GLint numMinor);
    //参数1：GLTriangleBatch 容器帮助类
    //参数2：外边缘半径
    //参数3：内边缘半径
    //参数4、5：主半径和从半径的细分单元数量
    gltMakeTorus(torusBath, 2.0, 0.3, 52, 26);
    //5.点的大小（方便点填充时，肉眼观察）
    glPointSize(4.0f);
}

/**
 键位设置，通过不同的键位对其进行设置
 控制Camera的移动，从而改变视口
 */
void SpecialKeys(int key, int x, int y) {
    //1.判断方向
    if (key == GLUT_KEY_UP) {
        //2.根据方向调整观察者位置 m3dDegToRad:角度转弧度
        //在世界坐标系中沿着X轴转
        viewFrame.RotateWorld(m3dDegToRad(-5.0), 1.0, 0.0, 0.0);
    }
    if (key == GLUT_KEY_DOWN) {
        //在世界坐标系中沿着X轴转
        viewFrame.RotateWorld(m3dDegToRad(5.0), 1.0, 0.0, 0.0);
    }
    if (key == GLUT_KEY_LEFT) {
        //在世界坐标系中沿着Y轴转
        viewFrame.RotateWorld(m3dDegToRad(-5.0), 0.0, 1.0, 0.0);
    }
    if (key == GLUT_KEY_RIGHT) {
        //在世界坐标系中沿着Y轴转
        viewFrame.RotateWorld(m3dDegToRad(5.0), 0.0, 1.0, 0.0);
    }
    //3.重新刷新
    glutPostRedisplay();
}

//窗口改变
void ChangeSize(int w, int h) {
    //1.防止h变为0
    if (h==0) {
        h = 1;
    }
    //2.设置视口窗口尺寸
    glViewport(0, 0, w, h);
    //3.setPerspective函数的参数是一个从顶点方向看去的视场角度即透视角度，形象点理解就是眼睛看向一个方向时睁开的角度（用角度值表示）
    // 设置透视模式，初始化其透视矩阵
    viewFrustum.SetPerspective(35.0f, float(w)/float(h), 1.0f, 100.0f);
    //4.把透视矩阵加载到透视矩阵堆栈中
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    //5.初始化渲染管线
    transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
}

void ProcessMenu(int value) {
    switch (value) {
        case 1:
            iDepth = !iDepth;
            break;
        case 2:
            iCull = !iCull;
            break;
        case 3:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
        case 4:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        case 5:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;
            
        default:
            break;
    }
    glutPostRedisplay();
}


int main(int argc, char* argv[])
{
    gltSetWorkingDirectory(argv[0]);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_SINGLE);
    glutInitWindowSize(1000, 600);
    glutCreateWindow("Geometry Test Program");
    glutReshapeFunc(ChangeSize);
    glutSpecialFunc(SpecialKeys);
    glutDisplayFunc(RenderScene);
    
    //添加右击菜单栏
    // Create the Menu
    glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("Toggle depth test",1);
    glutAddMenuEntry("Toggle cull backface",2);
    glutAddMenuEntry("Set Fill Mode", 3);
    glutAddMenuEntry("Set Line Mode", 4);
    glutAddMenuEntry("Set Point Mode", 5);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        return 1;
    }
    SetupRC();
    glutMainLoop();
    return 0;
}



