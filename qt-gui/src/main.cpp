#include "../include/GoldsrcRenderer.h"
#include <QtWidgets/QApplication>
#include <QVulkanInstance>
#include <QVulkanWindow>
#include "../include/VulkanRenderer.h"

int main(int vArgc, char *vArgv[])
{
    QApplication App(vArgc, vArgv);
    QVulkanInstance inst;
    inst.create();
    /*GoldsrcRenderer Window;
    Window.show();*/
    VulkanWindow w;
    w.setVulkanInstance(&inst);
    w.resize(1024, 768);
    w.show();
    return App.exec();
}
