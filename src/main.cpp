#include "main_window.h"
#include <QApplication>
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        QApplication app(argc, argv);
        
        MainWindow window;
        return app.exec();
    } catch (const std::exception& e) {
        std::cerr << "Dastur xatosi: " << e.what() << std::endl;
        return 1;
    }
}