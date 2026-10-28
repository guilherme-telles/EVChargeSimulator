#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <QMessageBox>

#include <filesystem>
#include <optional>

#include "MainWindow.h"
#include "ev/app/SimulationService.h"
#include "ev/calculator/ChargeCalculator.h"
#include "ev/catalog/XmlVehicleCatalog.h"
#include "ev/persistence/XmlScenarioRepository.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("EV Charge Simulator"));
    QLocale::setDefault(QLocale(QLocale::Portuguese, QLocale::Brazil));  // RN-06

    const ev::calculator::ChargeCalculator calculator;
    const ev::persistence::XmlScenarioRepository repository;
    ev::app::SimulationService service(calculator, repository);

    const QString catalogPath = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("data/vehicles.xml"));
    auto loaded = ev::catalog::loadCatalog(std::filesystem::path(catalogPath.toStdU16String()));
    std::optional<ev::catalog::XmlVehicleCatalog> catalog;
    if (loaded.ok()) {
        catalog = std::move(*loaded.catalog);
    }

    MainWindow window(service, catalog ? &*catalog : nullptr);
    window.show();
    if (!catalog) {
        QMessageBox::warning(&window, QObject::tr("Catálogo indisponível"),
                             QObject::tr("O catálogo de veículos não pôde ser carregado. Use a entrada manual."));
    }
    return app.exec();
}
