#include "../include/GoldsrcRenderer.h"
#include <QtWidgets/QApplication>
#include <QVulkanInstance>
#include <QVulkanWindow>
#include "../include/VulkanRenderer.h"
#include "IOObj.h"


int main(int vArgc, char *vArgv[])
{
    CIOObj Obj("../data/ball.obj");
    Obj.read();
    //QApplication App(vArgc, vArgv);
    //QVulkanInstance QtVkInstance;
    //QtVkInstance.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");
    //QtVkInstance.create();
    ///*GoldsrcRenderer Window;
    //Window.show();*/
    //CVulkanWindow Window;
    //Window.setVulkanInstance(&QtVkInstance);
    //Window.resize(1024, 768);
    //Window.show();
    //return App.exec();
}
