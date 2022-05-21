#include "jsonscenereader.h"
#include "mainwindow.h"
#include "raytracer.h"
#include "scene.h"
#include "ui_mainwindow.h"
#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200

#include <OpenCL/opencl.h>

#include <openclraytracerharness.h>
#include <iostream>
#include <QImage>
#include <QMessageBox>
#include <glm/glm.hpp>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    const int width = 512, height = 512;

    JsonSceneReader reader;

    // TODO: Configure.
    QFile file("/Users/nathankorzekwa/Projects/opencl-raytracer/scene_files/PT_glassBallBox.json");
    Scene s = reader.load_scene(file);
    Raytracer rt(width, height, ":/main.cl", 10000);

    try {
        rt.set_scene(s);

        auto render = rt.render(10);
        m_gfx_scene.clear();
        m_gfx_scene.addPixmap(QPixmap::fromImage(render));
        ui->preview->setScene(&m_gfx_scene);
    } catch (std::domain_error &ex) {
        std::cerr << "Error!" << std::endl;
        QMessageBox box;
        box.setText("Error setting up OpenCL!");
        box.show();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

