
#include "MapPage.h"
#include "MapBridge.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QLabel>
#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QWebChannel>
#include <QSizePolicy>
#include <QUrl>
#include <QtMath>
#include <QTimer>
#include <QShowEvent>
#include <QDebug>
#include <QVariant>

#include <cmath>

namespace {

QFrame* createMetricCard(
    const QString& title,
    QLabel*& valueLabel,
    const QString& initialValue)
{
    auto* card = new QFrame;
    card->setObjectName("mapMetricCard");

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(5);

    auto* titleLabel = new QLabel(title);
    titleLabel->setObjectName("mapMetricTitle");

    valueLabel = new QLabel(initialValue);
    valueLabel->setObjectName("mapMetricValue");

    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);

    return card;
}

} // namespace

MapPage::MapPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    setupWebChannel();

    qDebug() << "[MapPage] Iniciando carregamento do mapa";

    webView_->load(QUrl(QStringLiteral("qrc:/map/index.html")));
}

void MapPage::setupUi()
{
    qDebug() << "[MapPage] Configurando interface";

    // =================================================
    // Main layout
    // =================================================

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(20);

    // =================================================
    // Header
    // =================================================

    auto* headerLayout = new QHBoxLayout;

    auto* titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(3);

    auto* titleLabel = new QLabel(tr("Mapa de Alcance"));
    titleLabel->setObjectName("mapPageTitle");

    auto* subtitleLabel = new QLabel(
        tr("Planejamento de rota e autonomia do veículo elétrico")
    );
    subtitleLabel->setObjectName("mapPageSubtitle");

    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);

    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();

    statusLabel_ = new QLabel(tr("Inicializando mapa..."));
    statusLabel_->setObjectName("mapStatusLabel");

    headerLayout->addWidget(statusLabel_);

    mainLayout->addLayout(headerLayout);

    // =================================================
    // Summary cards
    // =================================================

    auto* metricsLayout = new QGridLayout;
    metricsLayout->setSpacing(10);

    auto* rangeCard = createMetricCard(
        tr("AUTONOMIA DISPONÍVEL"),
        rangeValue_,
        "-- km"
    );

    auto* routeCard = createMetricCard(
        tr("DISTÂNCIA DA ROTA"),
        routeValue_,
        "-- km"
    );

    auto* energyCard = createMetricCard(
        tr("ENERGIA ESTIMADA"),
        energyValue_,
        "-- kWh"
    );

    auto* arrivalCard = createMetricCard(
        tr("BATERIA NA CHEGADA"),
        arrivalSocValue_,
        "-- %"
    );

    metricsLayout->addWidget(rangeCard, 0, 0);
    metricsLayout->addWidget(routeCard, 0, 1);
    metricsLayout->addWidget(energyCard, 0, 2);
    metricsLayout->addWidget(arrivalCard, 0, 3);

    mainLayout->addLayout(metricsLayout);

    // =================================================
    // Map container
    // =================================================

    auto* mapContainer = new QFrame(this);
    mapContainer->setObjectName("mapContainer");

    mapContainer->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
    );

    auto* mapLayout = new QVBoxLayout(mapContainer);
    mapLayout->setContentsMargins(0, 0, 0, 0);
    mapLayout->setSpacing(0);

    // Única instância do QWebEngineView
    webView_ = new QWebEngineView(mapContainer);
    webView_->setObjectName("mapWebView");

    webView_->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
    );

    webView_->settings()->setAttribute(
        QWebEngineSettings::JavascriptEnabled,
        true
    );

    webView_->settings()->setAttribute(
        QWebEngineSettings::LocalContentCanAccessRemoteUrls,
        true
    );

    webView_->settings()->setAttribute(
        QWebEngineSettings::LocalContentCanAccessFileUrls,
        true
    );

    mapLayout->addWidget(webView_);

    // O mapa ocupa todo o espaço restante da página.
    mainLayout->addWidget(mapContainer, 1);

    // =================================================
    // Styles
    // =================================================

    setStyleSheet(R"(
        QWidget {
            background-color: #0B1220;
            color: #DCE8F2;
            font-family: "Inter", "Segoe UI", sans-serif;
        }

        QLabel#mapPageTitle {
            color: #EAF4FC;
            font-size: 21px;
            font-weight: 700;
        }

        QLabel#mapPageSubtitle {
            color: #8BAAC2;
            font-size: 12px;
        }

        QLabel#mapStatusLabel {
            color: #00C9BA;
            font-size: 11px;
            padding: 6px 10px;
            background-color: #132B3B;
            border: 1px solid #1D5260;
            border-radius: 6px;
        }

        QFrame#mapMetricCard {
            background-color: #111C2D;
            border: 1px solid #233A54;
            border-radius: 8px;
        }

        QLabel#mapMetricTitle {
            color: #8BAAC2;
            font-size: 10px;
            font-weight: 600;
        }

        QLabel#mapMetricValue {
            color: #00C9BA;
            font-size: 19px;
            font-weight: 700;
        }

        QFrame#mapContainer {
            background-color: #111C2D;
            border: 1px solid #233A54;
            border-radius: 10px;
        }
    )");

    // =================================================
    // WebEngine diagnostics
    // =================================================

    connect(
        webView_,
        &QWebEngineView::loadFinished,
        this,
        [this](bool success) {

            qDebug() << "[MapPage] loadFinished:"
                     << "success =" << success
                     << "url =" << webView_->url();

            qDebug() << "[MapPage] Título:"
                     << webView_->title();

            statusLabel_->setText(
                success ? tr("Mapa carregado")
                        : tr("Erro ao carregar o mapa")
            );

            if (!success) {
                return;
            }

            // Diagnóstico dos recursos carregados.
            webView_->page()->runJavaScript(
                "({"
                "  url: location.href,"
                "  title: document.title,"
                "  ready: document.readyState,"
                "  mapElement: !!document.getElementById('map'),"
                "  leaflet: typeof L,"
                "  webChannel: typeof QWebChannel,"
                "  scripts: Array.from(document.scripts).map(s => s.src),"
                "  styles: Array.from(document.styleSheets).map(s => s.href)"
                "})",
                [](const QVariant& result) {
                    qDebug() << "[MapPage] Diagnóstico JS:"
                             << result;
                }
            );

            // Diagnóstico das dimensões e da inicialização do Leaflet.
            webView_->page()->runJavaScript(
                "({"
                "  mapWidth: document.getElementById('map')?.clientWidth,"
                "  mapHeight: document.getElementById('map')?.clientHeight,"
                "  windowWidth: window.innerWidth,"
                "  windowHeight: window.innerHeight,"
                "  leafletMap: typeof map,"
                "  leafletVersion: L.version,"
                "  mapInitialized: !!document.getElementById('map')?._leaflet_id"
                "})",
                [](const QVariant& result) {
                    qDebug() << "[MapPage] Diagnóstico do mapa:"
                             << result;
                }
            );
        }
    );

    qDebug() << "[MapPage] Interface configurada";
}

// =====================================================
// WebChannel
// =====================================================

void MapPage::setupWebChannel()
{
    webChannel_ = new QWebChannel(webView_);
    bridge_ = new MapBridge(this);

    webChannel_->registerObject(
        QStringLiteral("mapBridge"),
        bridge_
    );

    webView_->page()->setWebChannel(webChannel_);

    // Evento de limpeza da rota.
    connect(
        bridge_,
        &MapBridge::routeCleared,
        this,
        [this]() {

            hasRoute_ = false;
            routeDistanceKm_ = 0.0;
            routeDurationMinutes_ = 0.0;

            // A autonomia do veículo continua válida.
            updateVehicleIndicators();

            routeValue_->setText("-- km");
            energyValue_->setText("-- kWh");
            arrivalSocValue_->setText("-- %");

            statusLabel_->setText(tr("Aguardando nova rota"));
        }
    );

    // Recebe os dados da rota calculada pelo JavaScript.
    connect(
        bridge_,
        &MapBridge::routeCalculatedFromMap,
        this,
        [this](double distanceKm, double durationMinutes) {
            updateTripIndicators(distanceKm, durationMinutes);
        }
    );
}

// =====================================================
// Trip summary
// =====================================================

void MapPage::setTripSummary(
    double rangeKm,
    double routeKm,
    double energyKwh,
    double arrivalSocPercent)
{
    const double displayedArrivalSoc =
        std::isfinite(arrivalSocPercent)
            ? qBound(0.0, arrivalSocPercent, 100.0)
            : 0.0;

    rangeValue_->setText(
        QString("%1 km").arg(rangeKm, 0, 'f', 1)
    );

    routeValue_->setText(
        QString("%1 km").arg(routeKm, 0, 'f', 1)
    );

    energyValue_->setText(
        QString("%1 kWh").arg(energyKwh, 0, 'f', 1)
    );

    arrivalSocValue_->setText(
        QString("%1 %").arg(displayedArrivalSoc, 0, 'f', 1)
    );
}

// =====================================================
// Vehicle data
// =====================================================

void MapPage::setVehicleData(double capacityKwh, double currentSocPercent, double consumptionKwhPer100Km, double ratedRangeKm) {
    capacityKwh_ = capacityKwh;
    currentSocPercent_ = currentSocPercent;
    consumptionKwhPer100Km_ = consumptionKwhPer100Km;
    ratedRangeKm_ = ratedRangeKm;

    updateVehicleIndicators();

    qDebug() << "MAP VEHICLE DATA:"
         << "Capacity:" << capacityKwh
         << "SoC:" << currentSocPercent
         << "Consumption:" << consumptionKwhPer100Km
         << "Rated range:" << ratedRangeKm;

    if (hasRoute_) {
        updateTripIndicators(routeDistanceKm_, routeDurationMinutes_);
    }
}

// =====================================================
// Vehicle indicators
// =====================================================

void MapPage::updateVehicleIndicators()
{
    const bool validData =
        std::isfinite(capacityKwh_) &&
        std::isfinite(currentSocPercent_) &&
        std::isfinite(consumptionKwhPer100Km_) &&
        std::isfinite(ratedRangeKm_) &&
        capacityKwh_ > 0.0 &&
        currentSocPercent_ >= 0.0 &&
        currentSocPercent_ <= 100.0 &&
        consumptionKwhPer100Km_ > 0.0 &&
        ratedRangeKm_ > 0.0;

    if (!validData) {
        rangeValue_->setText("-- km");
        return;
    }

    const double availableRange =
        ratedRangeKm_ * (currentSocPercent_ / 100.0);

    rangeValue_->setText(
        QString("%1 km").arg(availableRange, 0, 'f', 1)
    );

    qDebug() << "MAP RANGE:"
         << "Rated range:" << ratedRangeKm_
         << "SoC:" << currentSocPercent_
         << "Calculated:"
         << ratedRangeKm_ * (currentSocPercent_ / 100.0);
}

// =====================================================
// Trip indicators
// =====================================================

void MapPage::updateTripIndicators(
    double distanceKm,
    double durationMinutes)
{
    // -------------------------------------------------
    // Validação dos dados da rota
    // -------------------------------------------------

    const bool invalidRouteData =
        !std::isfinite(distanceKm) ||
        !std::isfinite(durationMinutes) ||
        distanceKm < 0.0 ||
        durationMinutes < 0.0;

    if (invalidRouteData) {
        hasRoute_ = false;
        routeDistanceKm_ = 0.0;
        routeDurationMinutes_ = 0.0;

        routeValue_->setText("-- km");
        energyValue_->setText("-- kWh");
        arrivalSocValue_->setText("-- %");

        statusLabel_->setText(tr("Dados da rota inválidos"));
        return;
    }

    // Guarda a rota para permitir recalcular os indicadores
    // quando os dados do veículo forem alterados.
    routeDistanceKm_ = distanceKm;
    routeDurationMinutes_ = durationMinutes;
    hasRoute_ = true;

    // A distância da rota independe dos dados do veículo.
    routeValue_->setText(
        QString("%1 km").arg(distanceKm, 0, 'f', 1)
    );

    // -------------------------------------------------
    // Validação dos dados do veículo
    // -------------------------------------------------

    const bool invalidVehicleData =
        !std::isfinite(capacityKwh_) ||
        !std::isfinite(currentSocPercent_) ||
        !std::isfinite(consumptionKwhPer100Km_) ||
        capacityKwh_ <= 0.0 ||
        consumptionKwhPer100Km_ <= 0.0 ||
        currentSocPercent_ < 0.0 ||
        currentSocPercent_ > 100.0;

    if (invalidVehicleData) {
        energyValue_->setText("-- kWh");
        arrivalSocValue_->setText("-- %");

        statusLabel_->setText(
            tr("Informe dados válidos do veículo e da bateria")
        );
        return;
    }

    // -------------------------------------------------
    // Cálculos energéticos
    // -------------------------------------------------

    const double availableEnergyKwh =
        capacityKwh_ * currentSocPercent_ / 100.0;

    const double availableRangeKm =
        availableEnergyKwh /
        (consumptionKwhPer100Km_ / 100.0);

    const double estimatedEnergyKwh =
        distanceKm * consumptionKwhPer100Km_ / 100.0;

    // SoC calculado sem limitar.
    // É utilizado para identificar autonomia insuficiente.
    const double arrivalSocPercent =
        currentSocPercent_ -
        (estimatedEnergyKwh / capacityKwh_) * 100.0;

    // -------------------------------------------------
    // Atualização dos cards
    // -------------------------------------------------

    rangeValue_->setText(
        QString("%1 km").arg(availableRangeKm, 0, 'f', 1)
    );

    energyValue_->setText(
        QString("%1 kWh").arg(estimatedEnergyKwh, 0, 'f', 1)
    );

    // Limita somente o valor apresentado na interface.
    // O cálculo original permanece inalterado.
    const double displayedArrivalSoc =
        qBound(0.0, arrivalSocPercent, 100.0);

    arrivalSocValue_->setText(
        QString("%1 %").arg(displayedArrivalSoc, 0, 'f', 1)
    );

    // -------------------------------------------------
    // Status da viagem
    // -------------------------------------------------

    if (arrivalSocPercent < 0.0) {
        statusLabel_->setText(
            tr("Autonomia insuficiente para concluir a rota")
        );
    } else {
        statusLabel_->setText(
            tr("Rota calculada · %1 min")
                .arg(qRound(durationMinutes))
        );
    }
}

// =====================================================
// Show event
// =====================================================

void MapPage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    qDebug() << "\n========== MAP PAGE SHOW ==========";

    QTimer::singleShot(300, this, [this]() {

        qDebug() << "[MapPage] Após exibição:";
        qDebug() << "MapPage size:" << size();
        qDebug() << "MapPage geometry:" << geometry();
        qDebug() << "MapPage visible:" << isVisible();

        qDebug() << "WebView size:" << webView_->size();
        qDebug() << "WebView geometry:" << webView_->geometry();
        qDebug() << "WebView visible:" << webView_->isVisible();

        qDebug() << "WebView parent:"
                 << webView_->parentWidget()->metaObject()->className();

        webView_->page()->runJavaScript(
            "({"
            "  windowWidth: window.innerWidth,"
            "  windowHeight: window.innerHeight,"
            "  mapWidth: document.getElementById('map').clientWidth,"
            "  mapHeight: document.getElementById('map').clientHeight"
            "})",
            [](const QVariant& result) {
                qDebug() << "[MapPage] Dimensões JS após exibição:"
                         << result;
            }
        );

        // Atualiza o tamanho do Leaflet após a página ficar visível.
        webView_->page()->runJavaScript(
            "if (typeof map !== 'undefined') { map.invalidateSize(true); }"
        );
    });

    qDebug() << "===================================\n";
}
