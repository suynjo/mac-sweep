#include <QApplication>
#include "scanner.h"
#include "gui/main_window.h"

int main(int argc, char** argv) {
    auto candidates = collect_candidates();

    QApplication app(argc, argv);
    MainWindow window(std::move(candidates));
    window.show();

    return app.exec();
}