#include "MainWindow.h"
#include "ValidationDashboard.h"
#include "PremiumVisuals.h"

#include <QAction>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequence>
#include <QLabel>
#include <QLocale>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QTimer>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>
#include <QAbstractItemView>
#include <QTimer>
#include <QDoubleSpinBox>


#include <filesystem>

#include "ev/app/ResultPresenter.h"
#include "ev/domain/ValidationError.h"
#include "Map/MapPage.h"

using namespace ev::domain;

namespace {

constexpr const char* kStyle = R"(
    QMainWindow, QWidget#rootBackground {
        background: #080F1C;
        color: #E8F2FC;
    }

    QWidget#inputPanel, QWidget#rightArea {
        background: #080F1C;
    }

    QScrollArea {
        border: 0px;
        background: #080F1C;
    }

    QScrollBar:vertical {
        background: #0F1B2C;
        width: 9px;
        margin: 1px;
        border: none;
    }

    QScrollBar::handle:vertical {
        background: #36516A;
        border-radius: 4px;
        min-height: 28px;
    }

    QScrollBar::handle:vertical:hover {
        background: #4E6E8C;
    }

    QScrollBar::add-line:vertical,
    QScrollBar::sub-line:vertical {
        height: 0px;
    }

    QScrollBar::add-page:vertical,
    QScrollBar::sub-page:vertical {
        background: none;
    }

    QMenuBar {
        background: #080F1C;
        border-bottom: 1px solid #1B2B43;
        color: #C1D4E4;
        padding: 7px 10px;
        font-weight: 600;
    }

    QMenuBar::item {
        padding: 7px 12px;
        background: transparent;
        border-radius: 6px;
    }

    QMenuBar::item:selected {
        background: #172A41;
        color: #FFFFFF;
    }

    QMenu {
        background: #111F32;
        border: 1px solid #2B4359;
        color: #EAF3FC;
        padding: 6px;
        border-radius: 8px;
    }

    QMenu::item {
        padding: 8px 26px;
        border-radius: 4px;
    }

    QMenu::item:selected {
        background: #24455A;
        color: #56EAD7;
    }

    QStatusBar {
        background: #0B1728;
        color: #90AABF;
        border-top: 1px solid #20344E;
        font-size: 11px;
    }

    QGroupBox {
        font-size: 12px;
        font-weight: 700;
        color: #DCECFB;
        background: #101E30;
        border: 1px solid #243952;
        border-radius: 14px;
        margin-top: 13px;
        padding: 16px 12px 12px;
    }

    QGroupBox::title {
        subcontrol-origin: margin;
        left: 15px;
        padding: 0 7px;
        color: #68E7D7;
    }

    QLabel {
        color: #B7CBDE;
        background: transparent;
    }

    QComboBox, QDoubleSpinBox {
        min-height: 32px;
        padding: 4px 10px;
        color: #EEF7FF;
        background: #192C43;
        border: 1px solid #314D69;
        border-radius: 8px;
        selection-background-color: #087F87;
    }

    QComboBox::drop-down {
        width: 26px;
        border: 0;
    }

    QComboBox QAbstractItemView {
        background: #14253A;
        color: #EAF5FF;
        selection-background-color: #246078;
        border: 1px solid #325771;
        outline: none;
        border-radius: 6px;
    }

    QComboBox:hover, QDoubleSpinBox:hover {
        border-color: #52B4C3;
        background: #1D334D;
    }

    QComboBox:focus, QDoubleSpinBox:focus {
        border: 1px solid #36DDCC;
    }

    QPushButton {
        color: #E6F3FC;
        background: #193046;
        border: 1px solid #35516A;
        border-radius: 8px;
        padding: 9px 16px;
        font-weight: 600;
    }

    QPushButton:hover {
        background: #234761;
        border-color: #54B5C7;
    }

    QPushButton:pressed {
        background: #10273F;
    }

    QPushButton#primaryAction {
        background: #0BCEBE;
        border: 1px solid #0BCEBE;
        color: #051C27;
        font-weight: 700;
    }

    QPushButton#primaryAction:hover {
        background: #73F3E5;
        border-color: #73F3E5;
    }

    QPushButton#photoButton {
        background: #1A3047;
        color: #D1F3F0;
        border: 1px solid #45657B;
        border-radius: 8px;
        padding: 8px 12px;
    }

    QPushButton#photoButton:hover {
        background: #24475A;
        color: #FFFFFF;
        border-color: #4FDACB;
    }

    QFrame#appTopBar {
        background: #101D31;
        border: 1px solid #273D56;
        border-radius: 16px;
    }

    QLabel#topTag {
        color: #56EAD7;
        font-weight: 800;
        font-size: 11px;
        letter-spacing: 1px;
    }

    QLabel#topHeadline {
        color: #F2F9FF;
        font-size: 22px;
        font-weight: 700;
    }

    QLabel#topSub {
        color: #90ACC0;
        font-size: 11px;
    }

    QLabel#topPill {
        color: #9CF5E7;
        font-weight: 700;
        background: #124047;
        border: 1px solid #1B6868;
        border-radius: 12px;
        padding: 8px 14px;
        font-size: 11px;
    }

    QFrame#heroPanel {
        background: #0B182B;
        border: 1px solid #2A445C;
        border-radius: 16px;
    }

    QLabel#heroEyebrow {
        color: #48E5D5;
        font-size: 11px;
        font-weight: 800;
        letter-spacing: 1px;
    }

    QLabel#heroName {
        color: #FFFFFF;
        font-size: 24px;
        font-weight: 800;
    }

    QLabel#heroNote {
        color: #91AFC7;
        font-size: 12px;
    }

    QLabel#renderNote {
        color: #7F9BB2;
        font-size: 10px;
    }

    QFrame#chartPanel {
        background: #111F32;
        border: 1px solid #283F57;
        border-radius: 14px;
    }

    QLabel#chartTitle {
        color: #C7D9EB;
        font-weight: 800;
        font-size: 11px;
        letter-spacing: 0.5px;
    }

    QFrame#card {
        background: #132239;
        border: 1px solid #2B435D;
        border-radius: 12px;
    }

    QFrame#card:hover {
        border-color: #3A5F83;
        background: #162842;
    }

    QLabel#cardTitle {
        color: #9CB8D1;
        font-weight: 600;
        font-size: 11px;
    }

    QLabel#cardValue {
        color: #ECF8FF;
        font-size: 22px;
        font-weight: 800;
    }

    QLabel#fieldError {
        color: #FFB5BB;
        background: #522336;
        padding: 5px 8px;
        border-radius: 6px;
        font-size: 11px;
    }

    QLabel#errorSummary {
        color: #FFD1D5;
        background: #3E2030;
        border: 1px solid #A2455A;
        padding: 11px;
        border-radius: 9px;
    }

    QLabel#warnings {
        color: #EAD2A4;
        background: #342A1C;
        border: 1px solid #72532E;
        border-radius: 9px;
        padding: 11px;
    }

    QDoubleSpinBox[invalid="true"],
    QTableWidget[invalid="true"] {
        border: 2px solid #FA6B7F;
    }

    QTableWidget {
        background: #0D1B2D;
        alternate-background-color: #14253A;
        border: 1px solid #2B4359;
        gridline-color: #20344C;
        color: #D8E7F5;
        selection-background-color: #244C60;
        border-radius: 8px;
    }

    QHeaderView::section {
        background: #192C43;
        border: 0px;
        border-bottom: 1px solid #314B61;
        padding: 8px;
        color: #B9CFDF;
        font-weight: 600;
    }

    QToolButton {
        color: #4CE0D1;
        border: 0px;
        font-weight: 700;
        padding: 4px;
    }

    QToolButton:hover {
        color: #7BF0E5;
    }

    QCheckBox {
        color: #D8E8F6;
        spacing: 8px;
    }

    QCheckBox::indicator {
        width: 17px;
        height: 17px;
        border: 1px solid #568195;
        border-radius: 4px;
        background: #172A42;
    }

    QCheckBox::indicator:checked {
        background: #1DD6C6;
        border: 1px solid #21E5D3;
    }

    QSplitter::handle {
        background: #0D1B2C;
        width: 8px;
    }

    QDialog {
        background: #0C192A;
        color: #E2EFFA;
    }

    QMessageBox {
        background: #102137;
    }
)";

QString qs(const std::string& text) { return QString::fromStdString(text); }

std::filesystem::path toPath(const QString& path) { return std::filesystem::path(path.toStdU16String()); }

void setInvalid(QWidget* widget, bool invalid) {
    if (!widget) return;
    widget->setProperty("invalid", invalid);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}

QLabel* makeCard(const QString& title, QHBoxLayout* row) {
    auto* frame = new QFrame;
    frame->setObjectName("card");
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(14, 12, 14, 12);
    auto* titleLabel = new QLabel(title, frame);
    titleLabel->setObjectName("cardTitle");
    auto* value = new QLabel(QStringLiteral("—"), frame);
    value->setObjectName("cardValue");
    value->setWordWrap(true);
    value->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    value->setAccessibleName(title);
    layout->addWidget(titleLabel);
    layout->addWidget(value);
    row->addWidget(frame, 1);
    return value;
}

bool parseLocaleNumber(const QString& text, double& value) {
    bool ok = false;
    value = QLocale().toDouble(text.trimmed(), &ok);
    if (!ok) {
        value = QLocale::c().toDouble(text.trimmed(), &ok);
    }
    return ok;
}

}  // namespace


MainWindow::MainWindow(
    ev::app::SimulationService& service,
    const IVehicleCatalog* catalog,
    QWidget* parent
): QMainWindow(parent),service_(service){

    if (catalog != nullptr) {
        vehicles_ = catalog->list();
    }

    setWindowTitle(tr("Simulador de Recarga de Veículo Elétrico"));
    setStyleSheet(kStyle);

    // =========================================================
    // PAINEL DE ENTRADA
    // =========================================================

    auto* inputPanel = buildInputPanel();

    // =========================================================
    // ÁREA PRINCIPAL DIREITA
    // =========================================================

    auto* right = new QWidget(this);
    right->setObjectName("rightArea");

    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(5, 3, 5, 12);
    rightLayout->setSpacing(14);

    premiumVisuals_ = new PremiumVisuals(right);

    rightLayout->addWidget(premiumVisuals_);
    rightLayout->addWidget(buildResultPanel());
    rightLayout->addWidget(buildComparisonPanel());
    rightLayout->addStretch();

    auto* rightScroll = new QScrollArea(this);
    rightScroll->setWidgetResizable(true);
    rightScroll->setFrameShape(QFrame::NoFrame);
    rightScroll->setWidget(right);

    // =========================================================
    // SPLITTER PRINCIPAL
    // =========================================================

    // IMPORTANTE: inicializar antes de configurar suas propriedades.
    mainSplitter_ = new QSplitter(Qt::Horizontal, this);

    mainSplitter_->setChildrenCollapsible(false);
    mainSplitter_->setCollapsible(0, false);
    mainSplitter_->setCollapsible(1, false);

    mainSplitter_->setStretchFactor(0, 0);
    mainSplitter_->setStretchFactor(1, 1);

    mainSplitter_->setHandleWidth(1);

    mainSplitter_->addWidget(inputPanel);
    mainSplitter_->addWidget(rightScroll);

    // Define o estado inicial depois que o layout for calculado.
    QTimer::singleShot(0, this, [this]() {
        if (!mainSplitter_)
            return;

        const int totalWidth = mainSplitter_->width();

        mainSplitter_->setSizes({
            50,
            totalWidth - 50 - mainSplitter_->handleWidth()
        });
    });

    // =========================================================
    // TOP BAR
    // =========================================================

    auto* topBar = new QFrame(this);
    topBar->setObjectName("appTopBar");

    auto* header = new QHBoxLayout(topBar);
    header->setContentsMargins(23, 14, 21, 14);

    auto* headerLeft = new QVBoxLayout;

    auto* brand = new QLabel(
        tr("EV CHARGE  /  STUDIO 3D"),
        topBar
    );
    brand->setObjectName("topTag");

    auto* heading = new QLabel(
        tr("Simulador de Recarga Elétrica"),
        topBar
    );
    heading->setObjectName("topHeadline");

    auto* subtitle = new QLabel(
        tr("Estúdio de veículos em 3D • Simulação dinâmica • Resultados rastreáveis"),
        topBar
    );
    subtitle->setObjectName("topSub");

    headerLeft->addWidget(brand);
    headerLeft->addWidget(heading);
    headerLeft->addWidget(subtitle);

    header->addLayout(headerLeft, 1);

    auto* pill = new QLabel(
        tr("●  MODO SIMULAÇÃO"),
        topBar
    );
    pill->setObjectName("topPill");

    header->addWidget(pill, 0, Qt::AlignVCenter);

    // =========================================================
    // ROOT / CENTRAL WIDGET
    // =========================================================

    auto* root = new QWidget(this);
    root->setObjectName("rootBackground");

    auto* rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(15, 13, 15, 12);
    rootLayout->setSpacing(12);

    rootLayout->addWidget(topBar);
    rootLayout->addWidget(mainSplitter_, 1);

    setCentralWidget(root);

    // =========================================================
    // MENU E ATALHOS
    // =========================================================

    buildMenu();

    for (const auto key : {Qt::Key_Return, Qt::Key_Enter}) {
        auto* shortcut = new QShortcut(QKeySequence(key), this);

        connect(
            shortcut,
            &QShortcut::activated,
            this,
            &MainWindow::calculate
        );
    }

    // =========================================================
    // INICIALIZAÇÃO DO FORMULÁRIO
    // =========================================================

    writeForm(service_.activeScenario());

    // =========================================================
    // ATUALIZAÇÃO AUTOMÁTICA
    // =========================================================

    liveTimer_ = new QTimer(this);
    liveTimer_->setSingleShot(true);
    liveTimer_->setInterval(380);

    connect(
        liveTimer_,
        &QTimer::timeout,
        this,
        &MainWindow::calculate
    );

    for (auto* spin : {
        capacity_,
        vehiclePower_,
        consumption_,
        ratedRange_,
        chargerPower_,
        installationLimit_,
        currentSoc_,
        targetSoc_,
        efficiency_,
        energyRate_,
        fixedFee_,
        minuteRate_
    }) {
        connect(
            spin,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            [this](double) {
                liveTimer_->start();
            }
        );
    }

    connect(
        vehicleCombo_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) {
            liveTimer_->start();
        }
    );

    connect(
        chargerType_,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int) {
            liveTimer_->start();
        }
    );

    connect(
        useCurve_,
        &QCheckBox::toggled,
        this,
        [this](bool) {
            liveTimer_->start();
        }
    );

    connect(
        curveTable_,
        &QTableWidget::itemChanged,
        this,
        [this](QTableWidgetItem*) {
            liveTimer_->start();
        }
    );

    // =========================================================
    // FINALIZAÇÃO
    // =========================================================

    statusBar()->showMessage(
        tr("Altere os parâmetros: os gráficos são atualizados automaticamente.")
    );

    resize(1600, 980);

    calculate();
}


void MainWindow::buildMenu() {
    auto* fileMenu = menuBar()->addMenu(tr("&Arquivo"));

    auto* open = new QAction(tr("&Abrir cenário..."), this);
    open->setShortcut(QKeySequence::Open);
    connect(open, &QAction::triggered, this, &MainWindow::openScenario);
    fileMenu->addAction(open);

    auto* save = new QAction(tr("&Salvar cenário..."), this);
    save->setShortcut(QKeySequence::Save);
    connect(save, &QAction::triggered, this, &MainWindow::saveScenario);
    fileMenu->addAction(save);

    auto* copy = new QAction(tr("&Copiar resumo"), this);
    copy->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+C")));
    connect(copy, &QAction::triggered, this, &MainWindow::copySummary);
    fileMenu->addAction(copy);

    fileMenu->addSeparator();
    auto* quit = new QAction(tr("Sai&r"), this);
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(quit);

    auto* helpMenu = menuBar()->addMenu(tr("A&juda"));
    auto* about = new QAction(tr("&Sobre"), this);
    connect(about, &QAction::triggered, this, [this] {
        QMessageBox::about(this, tr("Sobre"),
                           tr("Simulador de Recarga de Veículo Elétrico (ERS-VE-001)\n\n") +
                               qs(ev::app::kEstimateDisclaimer));
    });
    helpMenu->addAction(about);

    auto* toolsMenu = menuBar()->addMenu(tr("&Ferramentas"));
    auto* validation = new QAction(tr("Painel de &validação de testes..."), this);
    validation->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+T")));
    connect(validation, &QAction::triggered, this, [this] {
        ValidationDashboard dashboard(this);
        dashboard.exec();
    });
    toolsMenu->addAction(validation);
}

QLabel* MainWindow::addErrorLabel(const char* fieldId) {
    auto* label = new QLabel;
    label->setObjectName("fieldError");
    label->setWordWrap(true);
    label->hide();
    fieldErrors_[fieldId] = label;
    return label;
}

QDoubleSpinBox* MainWindow::addField(QFormLayout* form, const QString& label, const char* fieldId, double min,
                                     double max, int decimals, const QString& suffix, const QString& specialText) {
    auto* spin = new QDoubleSpinBox;
    spin->setRange(min, max);
    spin->setDecimals(decimals);
    spin->setSuffix(suffix);
    spin->setAccessibleName(label);
    if (!specialText.isEmpty()) {
        spin->setSpecialValueText(specialText);
    }
    auto* container = new QWidget;
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->addWidget(spin);
    layout->addWidget(addErrorLabel(fieldId));

    auto* caption = new QLabel(label + ":");
    caption->setStyleSheet("color: #B7CBDE; background: transparent; font-weight: 600;");
    caption->setBuddy(spin);
    form->addRow(caption, container);
    fieldWidgets_[fieldId] = spin;
    return spin;
}

QWidget* MainWindow::buildInputPanel() {
    auto* mainContainer = new QWidget(this);
    mainContainer->setObjectName("inputPanel");
    mainContainer->setMinimumWidth(58);
    mainContainer->setSizePolicy(
        QSizePolicy::Preferred,
        QSizePolicy::Expanding
    );
    
    auto* mainLayout = new QHBoxLayout(mainContainer);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 1. Barra lateral de ícones
    auto* sidebar = new QWidget(mainContainer);
    sidebar->setObjectName("iconSidebar");
    sidebar->setFixedWidth(50);
    sidebar->setStyleSheet("QWidget#iconSidebar { background: #0A1322; border-right: 1px solid #1B2B43; }");
    auto* sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(4, 8, 4, 8);
    sideLayout->setSpacing(8);

    // Instancia o stackWidget_
    stackWidget_ = new QStackedWidget(mainContainer);
    stackWidget_->setStyleSheet("QStackedWidget { background: #0D172A; border-right: 1px solid #1B2B43; }");
    stackWidget_->setMinimumWidth(0);
    stackWidget_->setSizePolicy(
        QSizePolicy::Ignored,
        QSizePolicy::Expanding
    );

    auto addSectionPage = [&](const QString& iconText, const QString& tooltip, QWidget* contentWidget) {
        auto* btn = new QToolButton(sidebar);
        btn->setText(iconText);
        btn->setToolTip(tooltip);
        btn->setAutoExclusive(false);
        btn->setMinimumSize(42, 42);
        btn->setStyleSheet(R"(
            QToolButton { background: #132238; color: #8BAAC2; border: 1px solid #233A54; border-radius: 8px; font-weight: bold; font-size: 14px; }
            QToolButton:hover { background: #1D3552; color: #00C9BA; border-color: #00C9BA; }
            QToolButton:checked { background: #00C9BA; color: #071425; border-color: #00C9BA; }
        )");
        
        int index = stackWidget_->addWidget(contentWidget);

        connect(btn, &QToolButton::clicked, this, [this, index]() {

            // Painel recolhido: expande e seleciona a seção clicada.
            if (!sidebarExpanded_) {
                switchSection(index);
                toggleSidebar();
                return;
            }

            // Outra seção: apenas troca a página.
            if (stackWidget_->currentIndex() != index) {
                switchSection(index);
                return;
            }

            // Mesma seção: recolhe o painel.
            toggleSidebar();

        });
        
        navButtons_.append(btn);
        sideLayout->addWidget(btn);
    };

    errorSummary_ = new QLabel(mainContainer);
    errorSummary_->setObjectName("errorSummary");
    errorSummary_->setWordWrap(true);
    errorSummary_->setAccessibleName(tr("Resumo de erros"));
    errorSummary_->hide();

    // --- SEÇÃO 1: VEÍCULO ---
    auto* vehicleScroll = new QScrollArea(mainContainer);
    vehicleScroll->setWidgetResizable(true);
    vehicleScroll->setStyleSheet("QScrollArea { background: #080F1C; border: none; }");
    auto* vehiclePage = new QWidget(vehicleScroll);
    vehiclePage->setStyleSheet("background: #080F1C; color: #E8F2FC;");
    auto* vehicleFormLayout = new QFormLayout(vehiclePage);
    
    vehicleCombo_ = new QComboBox(vehiclePage);
    vehicleCombo_->view()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vehicleCombo_->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    vehicleCombo_->addItem(tr("Entrada manual"));
    for (const auto& v : vehicles_) { vehicleCombo_->addItem(qs(v.name)); }
    
    auto* lblCatalog = new QLabel(tr("Catálogo:"), vehiclePage);
    lblCatalog->setStyleSheet("color: #B7CBDE; background: transparent;");
    vehicleFormLayout->addRow(lblCatalog, vehicleCombo_);
    
    if (!vehicles_.empty()) {
        auto* note = new QLabel(tr("⚠ Dados do catálogo são aproximados."), vehiclePage);
        note->setStyleSheet("color: #EAD2A4; background: transparent; font-size: 11px;");
        note->setWordWrap(true);
        vehicleFormLayout->addRow(note);
    }
    capacity_ = addField(vehicleFormLayout, tr("Capacidade útil"), fields::kUsableCapacity, 0, 1000, 2, " kWh");
    vehiclePower_ = addField(vehicleFormLayout, tr("Potência máx. do veículo"), fields::kVehicleMaxPower, 0, 1000, 2, " kW");
    consumption_ = addField(vehicleFormLayout, tr("Consumo médio"), fields::kConsumption, 0, 100, 2, " kWh/100 km");
    ratedRange_ = addField(vehicleFormLayout, tr("Autonomia nominal"), fields::kRatedRange, 0, 5000, 0, " km", tr("Não informado"));
    vehicleScroll->setWidget(vehiclePage);
    addSectionPage("🚗", tr("Veículo"), vehicleScroll);

    // --- SEÇÃO 2: CARREGADOR ---
    auto* chargerScroll = new QScrollArea(mainContainer);
    chargerScroll->setWidgetResizable(true);
    chargerScroll->setStyleSheet("QScrollArea { background: #080F1C; border: none; }");
    auto* chargerPage = new QWidget(chargerScroll);
    chargerPage->setStyleSheet("background: #080F1C; color: #E8F2FC;");
    auto* chargerFormLayout = new QFormLayout(chargerPage);
    
    chargerType_ = new QComboBox(chargerPage);
    chargerType_->addItems({QStringLiteral("AC"), QStringLiteral("DC")});
    
    auto* lblChargerType = new QLabel(tr("Tipo:"), chargerPage);
    lblChargerType->setStyleSheet("color: #B7CBDE; background: transparent;");
    chargerFormLayout->addRow(lblChargerType, chargerType_);
    
    chargerPower_ = addField(chargerFormLayout, tr("Potência do carregador"), fields::kChargerPower, 0, 1000, 2, " kW");
    installationLimit_ = addField(chargerFormLayout, tr("Limite da instalação"), fields::kInstallationLimit, 0, 1000, 2, " kW", tr("Não informado"));
    chargerScroll->setWidget(chargerPage);
    addSectionPage("⚡", tr("Carregador"), chargerScroll);

    // --- SEÇÃO 3: BATERIA ---
    auto* batteryScroll = new QScrollArea(mainContainer);
    batteryScroll->setWidgetResizable(true);
    batteryScroll->setStyleSheet("QScrollArea { background: #080F1C; border: none; }");
    auto* batteryPage = new QWidget(batteryScroll);
    batteryPage->setStyleSheet("background: #080F1C; color: #E8F2FC;");
    auto* batteryFormLayout = new QFormLayout(batteryPage);
    
    currentSoc_ = addField(batteryFormLayout, tr("SoC atual"), fields::kCurrentSoc, 0, 100, 1, " %");
    targetSoc_ = addField(batteryFormLayout, tr("SoC desejado"), fields::kTargetSoc, 0, 100, 1, " %");
    efficiency_ = addField(batteryFormLayout, tr("Eficiência"), fields::kEfficiency, 0, 100, 1, " %");
    batteryScroll->setWidget(batteryPage);
    addSectionPage("🔋", tr("Bateria e Eficiência"), batteryScroll);

    // --- SEÇÃO 4: TARIFA ---
    auto* tariffScroll = new QScrollArea(mainContainer);
    tariffScroll->setWidgetResizable(true);
    tariffScroll->setStyleSheet("QScrollArea { background: #080F1C; border: none; }");
    auto* tariffPage = new QWidget(tariffScroll);
    tariffPage->setStyleSheet("background: #080F1C; color: #E8F2FC;");
    auto* tariffFormLayout = new QFormLayout(tariffPage);
    
    energyRate_ = addField(tariffFormLayout, tr("Tarifa de energia"), fields::kEnergyRate, 0, 100, 2, " R$/kWh");
    fixedFee_ = addField(tariffFormLayout, tr("Taxa fixa"), fields::kFixedFee, 0, 10000, 2, " R$");
    minuteRate_ = addField(tariffFormLayout, tr("Tarifa por minuto"), fields::kMinuteRate, 0, 100, 2, " R$/min");
    tariffScroll->setWidget(tariffPage);
    addSectionPage("💰", tr("Tarifas"), tariffScroll);

    // --- SEÇÃO 5: CURVA DE CARGA ---
    auto* curveScroll = new QScrollArea(mainContainer);
    curveScroll->setWidgetResizable(true);
    curveScroll->setStyleSheet("QScrollArea { background: #080F1C; border: none; }");
    auto* curvePage = new QWidget(curveScroll);
    curvePage->setStyleSheet("background: #080F1C; color: #E8F2FC;");
    auto* curveLayout = new QVBoxLayout(curvePage);
    
    useCurve_ = new QCheckBox(tr("Usar curva segmentada por faixa de SoC"), curvePage);
    curveTable_ = new QTableWidget(0, 3, curvePage);
    curveTable_->setHorizontalHeaderLabels({tr("De %"), tr("Até %"), tr("kW máx.")});
    curveTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    curveTable_->setMinimumHeight(140);
    
    auto* curveButtons = new QHBoxLayout;
    auto* addRow = new QPushButton(tr("+ Faixa"), curvePage);
    auto* removeRow = new QPushButton(tr("- Remover"), curvePage);
    curveButtons->addWidget(addRow);
    curveButtons->addWidget(removeRow);
    curveLayout->addWidget(useCurve_);
    curveLayout->addWidget(curveTable_);
    curveLayout->addWidget(addErrorLabel(fields::kCurve));
    curveLayout->addLayout(curveButtons);
    curveScroll->setWidget(curvePage);
    addSectionPage("📈", tr("Curva de Carga"), curveScroll);

    mapPage_ = new MapPage(stackWidget_);

    addSectionPage(
        "🗺",
        tr("Mapa de Alcance"),
        mapPage_
    );

    // Atualiza os dados do veículo exibidos no mapa.
    auto updateMapVehicleData = [this]() {
        if (!mapPage_ ||
            !capacity_ ||
            !currentSoc_ ||
            !consumption_ ||
            !ratedRange_) {
            return;
        }

        mapPage_->setVehicleData(
            capacity_->value(),
            currentSoc_->value(),
            consumption_->value(),
            ratedRange_->value()
        );
    };

    // Atualiza quando qualquer dado relevante mudar.
    connect(capacity_, &QDoubleSpinBox::valueChanged, this,
        [updateMapVehicleData](double) {
            updateMapVehicleData();
        });

    connect(currentSoc_, &QDoubleSpinBox::valueChanged, this,
        [updateMapVehicleData](double) {
            updateMapVehicleData();
        });

    connect(consumption_, &QDoubleSpinBox::valueChanged, this,
        [updateMapVehicleData](double) {
            updateMapVehicleData();
        });

    connect(ratedRange_, &QDoubleSpinBox::valueChanged, this,
        [updateMapVehicleData](double) {
            updateMapVehicleData();
        });

    // Envia os valores iniciais.
    updateMapVehicleData();

    sideLayout->addStretch();

    // Botão de Adicionar Comparação (CONECTADO CORRETAMENTE)
    auto* compareBtn = new QToolButton(sidebar);
    compareBtn->setText("⚖");
    compareBtn->setToolTip(tr("Adicionar à comparação"));
    compareBtn->setMinimumSize(42, 42);
    compareBtn->setStyleSheet(R"(
        QToolButton { background: #132238; color: #0BCEBE; border: 1px solid #233A54; border-radius: 8px; font-weight: bold; font-size: 16px; }
        QToolButton:hover { background: #1D3552; color: #73F3E5; border-color: #0BCEBE; }
    )");
    connect(compareBtn, &QToolButton::clicked, this, &MainWindow::addToComparison);
    sideLayout->addWidget(compareBtn);

    // Botão de Ação Rápida (Calcular)
    auto* calcIconButton = new QToolButton(sidebar);
    calcIconButton->setText("▶");
    calcIconButton->setToolTip(tr("Calcular Recarga"));
    calcIconButton->setMinimumSize(42, 42);
    calcIconButton->setStyleSheet("QToolButton { background: #0BCEBE; color: #051C27; border-radius: 8px; font-weight: bold; font-size: 16px; } QToolButton:hover { background: #73F3E5; }");
    connect(calcIconButton, &QToolButton::clicked, this, &MainWindow::calculate);
    sideLayout->addWidget(calcIconButton);

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(stackWidget_, 1);

    if (!navButtons_.isEmpty()) {
        navButtons_[0]->setChecked(true);
        stackWidget_->setCurrentIndex(0);
    }

    connect(vehicleCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onVehicleSelected);
    connect(chargerType_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onChargerTypeChanged);
    connect(addRow, &QPushButton::clicked, this, &MainWindow::addCurveRow);
    connect(removeRow, &QPushButton::clicked, this, &MainWindow::removeCurveRow);

    return mainContainer;
}

void MainWindow::switchSection(int index) {
    if (!stackWidget_) return;
    stackWidget_->setCurrentIndex(index);
    for (int i = 0; i < navButtons_.size(); ++i) {
        navButtons_[i]->setChecked(i == index);
    }
}


void MainWindow::toggleSidebar()
{
    if (!mainSplitter_ || mainSplitter_->count() < 2)
        return;

    constexpr int expandedWidth = 425;
    constexpr int collapsedWidth = 50;

    const int totalWidth = mainSplitter_->width();

    if (totalWidth <= 0)
        return;

    // Interrompe uma animação anterior, caso exista.
    if (sidebarAnimation_) {
        sidebarAnimation_->stop();
        sidebarAnimation_->deleteLater();
        sidebarAnimation_ = nullptr;
    }

    const bool expanding = !sidebarExpanded_;

    const int startWidth = mainSplitter_->sizes().value(0);
    const int targetWidth = expanding
        ? expandedWidth
        : collapsedWidth;

    auto* animation = new QVariantAnimation(this);

    sidebarAnimation_ = animation;

    animation->setDuration(280);
    animation->setStartValue(startWidth);
    animation->setEndValue(targetWidth);
    animation->setEasingCurve(QEasingCurve::InOutCubic);

    connect(
        animation,
        &QVariantAnimation::valueChanged,
        this,
        [this](const QVariant& value)
        {
            if (!mainSplitter_)
                return;

            const int leftWidth = value.toInt();
            const int rightWidth =
                qMax(0, mainSplitter_->width() - leftWidth);

            mainSplitter_->setSizes({
                leftWidth,
                rightWidth
            });
        }
    );

    connect(
        animation,
        &QVariantAnimation::finished,
        this,
        [this, animation]()
        {
            if (sidebarAnimation_ == animation)
                sidebarAnimation_ = nullptr;

            animation->deleteLater();
        }
    );

    sidebarExpanded_ = expanding;

    animation->start();
}


QWidget* MainWindow::buildResultPanel() {
    auto* box = new QGroupBox(tr("Resultado"));
    auto* layout = new QVBoxLayout(box);

    auto* cards = new QHBoxLayout;
    timeCard_ = makeCard(tr("Tempo estimado"), cards);
    costCard_ = makeCard(tr("Custo total"), cards);
    rangeCard_ = makeCard(tr("Autonomia adicionada"), cards);
    layout->addLayout(cards);

    auto* validationButton = new QPushButton(tr("Abrir painel de validação dos testes"), box);
    validationButton->setToolTip(tr("Executar cenários UT-001 a UT-014 sem alterar sua simulação"));
    connect(validationButton, &QPushButton::clicked, this, [this] {
        ValidationDashboard dashboard(this);
        dashboard.exec();
    });
    layout->addWidget(validationButton, 0, Qt::AlignRight);

    detailsToggle_ = new QToolButton(box);
    detailsToggle_->setCheckable(true);
    detailsToggle_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    detailsToggle_->setArrowType(Qt::RightArrow);
    detailsToggle_->setText(tr("Detalhes e premissas"));
    details_ = new QWidget(box);
    auto* detailsLayout = new QVBoxLayout(details_);
    detailsText_ = new QLabel(details_);
    detailsText_->setTextFormat(Qt::PlainText);
    detailsText_->setWordWrap(true);
    detailsText_->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    detailsLayout->addWidget(detailsText_);
    details_->hide();
    connect(detailsToggle_, &QToolButton::toggled, this, [this](bool checked) {
        details_->setVisible(checked);
        detailsToggle_->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
    });
    layout->addWidget(detailsToggle_);
    layout->addWidget(details_);

    warnings_ = new QLabel(box);
    warnings_->setObjectName("warnings");
    warnings_->setWordWrap(true);
    warnings_->setTextFormat(Qt::PlainText);
    warnings_->setText(QStringLiteral("ℹ ") + qs(ev::app::kEstimateDisclaimer));
    layout->addWidget(warnings_);
    return box;
}


QWidget* MainWindow::buildComparisonPanel() {
    auto* box = new QGroupBox(tr("Comparação de cenários"));
    auto* layout = new QVBoxLayout(box);

    comparisonChart_ = new ComparisonChart(box);
    comparisonChart_->setMaximumHeight(245);

    comparisonTable_ = new QTableWidget(5, 0, box);
    comparisonTable_->setObjectName("comparisonTable");

    comparisonTable_->setMaximumHeight(190);
    comparisonTable_->setMinimumHeight(190);

    comparisonTable_->setVerticalHeaderLabels({
        tr("Tempo"),
        tr("Custo"),
        tr("Autonomia adicionada"),
        tr("Energia da rede"),
        tr("Potência efetiva")
    });

    comparisonTable_->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    comparisonTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    comparisonTable_->setSelectionBehavior(QAbstractItemView::SelectItems);
    comparisonTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    comparisonTable_->setAlternatingRowColors(true);
    comparisonTable_->setWordWrap(true);
    comparisonTable_->setCornerButtonEnabled(false);

    // Cabeçalho horizontal: Cenário 1, Cenário 2...
    auto* horizontalHeader = comparisonTable_->horizontalHeader();
    horizontalHeader->setSectionResizeMode(QHeaderView::Stretch);
    horizontalHeader->setDefaultAlignment(Qt::AlignCenter);
    horizontalHeader->setMinimumHeight(38);
    horizontalHeader->setVisible(false);

    // Cabeçalho vertical: Tempo, Custo...
    auto* verticalHeader = comparisonTable_->verticalHeader();

    verticalHeader->setVisible(true);
    verticalHeader->setSectionResizeMode(QHeaderView::Stretch);
    verticalHeader->setMinimumSectionSize(0);
    verticalHeader->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    comparisonTable_->setAccessibleName(
        tr("Tabela de comparação de cenários")
    );

    comparisonTable_->setStyleSheet(R"(
        QTableWidget#comparisonTable {
            background-color: #101D2F;
            alternate-background-color: #14263B;
            color: #DBEAF6;
            border: 1px solid #2D475F;
            border-radius: 8px;
            gridline-color: #263A4F;
            selection-background-color: #246078;
            selection-color: #FFFFFF;
        }

        QTableWidget#comparisonTable::item {
            padding: 6px;
            border: none;
        }

        QTableWidget#comparisonTable::item:selected {
            background-color: #246078;
            color: #FFFFFF;
        }

        QTableWidget#comparisonTable QHeaderView::section {
            background-color: #1E334D;
            color: #C7DEEA;
            padding: 8px;
            font-weight: 600;
            border: none;
            border-bottom: 1px solid #355270;
        }

        QTableWidget#comparisonTable QTableCornerButton::section {
            background-color: #1E334D;
            border: none;
        }
        QTableWidget#comparisonTable::viewport {
            background-color: #101D2F;
        }

    )");

    auto* note = new QLabel(
        tr("★ indica menor custo ou menor tempo; os cenários não são necessariamente equivalentes."),
        box
    );
    note->setWordWrap(true);

    auto* clear = new QPushButton(tr("Limpar comparação"), box);

    connect(clear, &QPushButton::clicked,
            this, &MainWindow::clearComparison);

    layout->addWidget(comparisonChart_);
    layout->addWidget(comparisonTable_);
    layout->addWidget(note);
    layout->addWidget(clear, 0, Qt::AlignLeft);

    return box;
}

std::optional<ChargeRequest> MainWindow::readForm() {
    ChargeRequest r = service_.activeScenario();
    const int vehicleIndex = vehicleCombo_->currentIndex();
    if (vehicleIndex > 0 && vehicleIndex - 1 < static_cast<int>(vehicles_.size())) {
        const Vehicle& v = vehicles_[static_cast<std::size_t>(vehicleIndex - 1)];
        r.vehicle = {v.id, v.name};
    } else {
        for (const auto& v : vehicles_) {
            if (v.id == r.vehicle.id) {
                r.vehicle = VehicleInfo{};
                break;
            }
        }
    }
    r.battery = {KilowattHours{capacity_->value()}, Percent{currentSoc_->value()}, Percent{targetSoc_->value()},
                 efficiency_->value() / 100.0};
    r.vehicleMaxPower = Kilowatts{vehiclePower_->value()};
    r.efficiency.consumptionKwhPer100Km = consumption_->value();
    r.efficiency.ratedRange =
        ratedRange_->value() > 0.0 ? std::optional<Kilometers>(Kilometers{ratedRange_->value()}) : std::nullopt;
    r.charger.type = chargerType_->currentIndex() == 1 ? ChargerType::DC : ChargerType::AC;
    r.charger.ratedPower = Kilowatts{chargerPower_->value()};
    r.charger.installationLimit = installationLimit_->value() > 0.0
                                      ? std::optional<Kilowatts>(Kilowatts{installationLimit_->value()})
                                      : std::nullopt;
    r.tariff.currency = "BRL";
    r.tariff.energyRatePerKwh = energyRate_->value();
    r.tariff.fixedFee = Brl{fixedFee_->value()};
    r.tariff.ratePerMinute = minuteRate_->value();

    r.curve.segments.clear();
    if (useCurve_->isChecked()) {
        const QSignalBlocker blocker(curveTable_);
        for (int row = 0; row < curveTable_->rowCount(); ++row) {
            double values[3] = {0.0, 0.0, 0.0};
            for (int col = 0; col < 3; ++col) {
                const QTableWidgetItem* item = curveTable_->item(row, col);
                if (item == nullptr || item->text().trimmed().isEmpty() || !parseLocaleNumber(item->text(), values[col])) {
                    clearResult();
                    showErrors({makeError(codes::kInvalidCurve, {fields::kCurve},
                                          "linha " + std::to_string(row + 1) + ": número inválido")});
                    return std::nullopt;
                }
            }
            r.curve.segments.push_back({Percent{values[0]}, Percent{values[1]}, Kilowatts{values[2]}});
        }
    }
    return r;
}

void MainWindow::writeForm(const ChargeRequest& r) {
    {
        const QSignalBlocker blockVehicle(vehicleCombo_);
        const QSignalBlocker blockType(chargerType_);
        int index = 0;
        for (std::size_t i = 0; i < vehicles_.size(); ++i) {
            if (vehicles_[i].id == r.vehicle.id) {
                index = static_cast<int>(i) + 1;
            }
        }
        vehicleCombo_->setCurrentIndex(index);
        chargerType_->setCurrentIndex(r.charger.type == ChargerType::DC ? 1 : 0);
    }
    capacity_->setValue(r.battery.usableCapacity.value());
    vehiclePower_->setValue(r.vehicleMaxPower.value());
    consumption_->setValue(r.efficiency.consumptionKwhPer100Km);
    ratedRange_->setValue(r.efficiency.ratedRange ? r.efficiency.ratedRange->value() : 0.0);
    chargerPower_->setValue(r.charger.ratedPower.value());
    installationLimit_->setValue(r.charger.installationLimit ? r.charger.installationLimit->value() : 0.0);
    currentSoc_->setValue(r.battery.currentSoc.value());
    targetSoc_->setValue(r.battery.targetSoc.value());
    efficiency_->setValue(r.battery.efficiency * 100.0);
    energyRate_->setValue(r.tariff.energyRatePerKwh);
    fixedFee_->setValue(r.tariff.fixedFee.value());
    minuteRate_->setValue(r.tariff.ratePerMinute);

    const QLocale locale;
    const QSignalBlocker blocker(curveTable_);
    curveTable_->setRowCount(0);
    for (const auto& segment : r.curve.segments) {
        const int row = curveTable_->rowCount();
        curveTable_->insertRow(row);
        curveTable_->setItem(row, 0, new QTableWidgetItem(locale.toString(segment.fromSoc.value(), 'g', 10)));
        curveTable_->setItem(row, 1, new QTableWidgetItem(locale.toString(segment.toSoc.value(), 'g', 10)));
        curveTable_->setItem(row, 2, new QTableWidgetItem(locale.toString(segment.maxPower.value(), 'g', 10)));
    }
    useCurve_->setChecked(r.curve.enabled());
}

void MainWindow::clearErrors() {
    errorSummary_->hide();
    for (auto& [id, label] : fieldErrors_) {
        label->clear();
        label->hide();
    }
    for (auto& [id, widget] : fieldWidgets_) {
        setInvalid(widget, false);
    }
}

void MainWindow::clearResult() {
    last_.reset();
    timeCard_->setText(QStringLiteral("—"));
    costCard_->setText(QStringLiteral("—"));
    rangeCard_->setText(QStringLiteral("—"));
    detailsText_->clear();
    if (premiumVisuals_) premiumVisuals_->reset();
    warnings_->setText(QStringLiteral("ℹ ") + qs(ev::app::kEstimateDisclaimer));
}

void MainWindow::showErrors(const std::vector<ValidationError>& errors) {
    clearErrors();
    QStringList lines;
    QWidget* firstInvalid = nullptr;
    for (const auto& e : errors) {
        QString line = QStringLiteral("• ") + qs(e.code) + ": " + qs(e.message);
        if (!e.detail.empty()) {
            line += " (" + qs(e.detail) + ")";
        }
        lines << line;
        for (const auto& field : e.fields) {
            if (auto it = fieldWidgets_.find(field); it != fieldWidgets_.end()) {
                setInvalid(it->second, true);
                if (firstInvalid == nullptr) {
                    firstInvalid = it->second;
                }
            }
            if (auto it = fieldErrors_.find(field); it != fieldErrors_.end()) {
                it->second->setText(QStringLiteral("✖ ") + qs(e.message));
                it->second->show();
            }
        }
    }
    errorSummary_->setText(tr("✖ Não foi possível calcular. Corrija os campos destacados:") + "\n" +
                           lines.join("\n"));
    errorSummary_->show();
    if (firstInvalid != nullptr) {
        firstInvalid->setFocus();
    }
}

void MainWindow::showResult(const ChargeRequest& request, const ChargeResult& result) {
    const ev::app::ResultView v = ev::app::presentResult(request, result);
    timeCard_->setText(qs(v.duration) + "\n" + qs(v.durationDecimal));
    costCard_->setText(qs(v.totalCost));
    rangeCard_->setText(qs(v.addedRange) + "\n" + tr("Final: ") + qs(v.finalRange));
    if (premiumVisuals_) premiumVisuals_->setSimulation(request, result);

    QStringList details;
    details << tr("Energia na bateria: ") + qs(v.batteryEnergy) << tr("Energia da rede: ") + qs(v.gridEnergy)
            << tr("Potência efetiva (média): ") + qs(v.effectivePower) << tr("Eficiência: ") + qs(v.efficiency)
            << tr("Modelo de potência: ") + qs(v.curveModel)
            << tr("Custo: energia ") + qs(v.energyCost) + tr(" | tempo ") + qs(v.timeCost) + tr(" | taxa fixa ") +
                   qs(v.fixedCost)
            << tr("Autonomia atual estimada: ") + qs(v.currentRange) << QString() << tr("Premissas e fórmulas:");
    for (const auto& a : v.assumptions) {
        details << QStringLiteral("• ") + qs(a);
    }
    detailsText_->setText(details.join("\n"));

    QStringList notes;
    for (const auto& w : v.warnings) {
        notes << QStringLiteral("⚠ ") + qs(w);
    }
    notes << QStringLiteral("ℹ ") + qs(ev::app::kEstimateDisclaimer);
    warnings_->setText(notes.join("\n"));
}

void MainWindow::calculate() {
    const auto request = readForm();
    if (!request) {
        return;
    }
    service_.setActiveScenario(*request);
    const auto outcome = service_.simulate(*request);
    if (!outcome.ok()) {
        clearResult();
        showErrors(outcome.errors);
        statusBar()->showMessage(tr("Corrija os campos destacados."));
        return;
    }
    clearErrors();
    showResult(*request, *outcome.result);
    last_ = std::make_pair(*request, *outcome.result);
    statusBar()->showMessage(tr("Simulação concluída."), 5000);
}

void MainWindow::onVehicleSelected(int index) {
    if (index <= 0 || index - 1 >= static_cast<int>(vehicles_.size())) {
        return;
    }
    const Vehicle& v = vehicles_[static_cast<std::size_t>(index - 1)];
    const ChargerType type = chargerType_->currentIndex() == 1 ? ChargerType::DC : ChargerType::AC;
    capacity_->setValue(v.usableCapacity.value());
    vehiclePower_->setValue(acceptedPowerFor(v, type).value());
    consumption_->setValue(v.consumptionKwhPer100Km);
    ratedRange_->setValue(v.ratedRange.value());
}

void MainWindow::onChargerTypeChanged(int /*index*/) { onVehicleSelected(vehicleCombo_->currentIndex()); }

void MainWindow::addToComparison() {
    const auto request = readForm();
    if (!request) {
        return;
    }
    const QString label = tr("Cenário %1").arg(service_.comparison().size() + 1);
    const auto outcome = service_.addToComparison(label.toStdString(), *request);
    if (!outcome.ok()) {
        showErrors(outcome.errors);
        return;
    }
    clearErrors();
    refreshComparison();
    statusBar()->showMessage(tr("Cenário adicionado à comparação."), 5000);
}

void MainWindow::clearComparison() {
    service_.clearComparison();
    refreshComparison();
}

void MainWindow::refreshComparison() {
    const auto& entries = service_.comparison();

    const bool hasScenarios = !entries.empty();

    comparisonTable_->horizontalHeader()->setVisible(hasScenarios);
    comparisonTable_->setColumnCount(static_cast<int>(entries.size()));

    const auto highlights = ev::app::highlight(entries);

    QVector<QString> comparisonNames;
    QVector<double> comparisonHours;
    QVector<double> comparisonCosts;

    for (const auto& e : entries) {
        comparisonNames.append(qs(e.label));
        comparisonHours.append(e.result.duration.value());
        comparisonCosts.append(e.result.cost.total.value());
    }

    if (comparisonChart_) {
        comparisonChart_->setData(
            comparisonNames,
            comparisonHours,
            comparisonCosts
        );
    }

    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto v = ev::app::presentResult(
            entries[i].request,
            entries[i].result
        );

        const int col = static_cast<int>(i);

        // Nome do modelo do veículo no cabeçalho
        const QString vehicleName = qs(entries[i].request.vehicle.name);

        auto* header = new QTableWidgetItem(
            tr("%1: %2").arg(qs(entries[i].label), vehicleName)
        );

        // Mantém a identificação original do cenário no tooltip
        header->setToolTip(
            tr("Cenário: %1").arg(qs(entries[i].label))
        );

        comparisonTable_->setHorizontalHeaderItem(col, header);

        QString time = qs(v.duration);

        if (highlights.fastest == i) {
            time += tr("  ★ menor tempo");
        }

        QString cost = qs(v.totalCost);

        if (highlights.lowestCost == i) {
            cost += tr("  ★ menor custo");
        }

        comparisonTable_->setItem(0, col, new QTableWidgetItem(time));
        comparisonTable_->setItem(1, col, new QTableWidgetItem(cost));
        comparisonTable_->setItem(2, col, new QTableWidgetItem(qs(v.addedRange)));
        comparisonTable_->setItem(3, col, new QTableWidgetItem(qs(v.gridEnergy)));
        comparisonTable_->setItem(4, col, new QTableWidgetItem(qs(v.effectivePower)));
    }
}

void MainWindow::openScenario() {
    const QString file =
        QFileDialog::getOpenFileName(this, tr("Abrir cenário"), QString(), tr("Cenário XML (*.xml)"));
    if (file.isEmpty()) {
        return;
    }
    const auto result = service_.openScenario(toPath(file));
    if (!result.ok()) {
        QStringList lines;
        for (const auto& e : result.errors) {
            QString line = qs(e.code) + ": " + qs(e.message);
            if (!e.fields.empty() || !e.detail.empty()) {
                line += "\n   " + (e.fields.empty() ? QString() : qs(e.fields.front()) + " - ") + qs(e.detail);
            }
            lines << line;
        }
        QMessageBox::warning(this, tr("Cenário não aberto"),
                             tr("O cenário atual foi mantido.") + "\n\n" + lines.join("\n"));
        return;
    }
    writeForm(service_.activeScenario());
    clearErrors();
    clearResult();
    if (!result.warnings.empty()) {
        QStringList lines;
        for (const auto& w : result.warnings) {
            lines << qs(w.code) + ": " + qs(w.message);
        }
        QMessageBox::information(this, tr("Cenário aberto com avisos"), lines.join("\n"));
    }
    statusBar()->showMessage(tr("Cenário aberto. Pressione Enter para calcular."), 5000);
}

void MainWindow::saveScenario() {
    const auto request = readForm();
    if (!request) {
        return;
    }
    QString file = QFileDialog::getSaveFileName(this, tr("Salvar cenário"), QStringLiteral("cenario.xml"),
                                                tr("Cenário XML (*.xml)"));
    if (file.isEmpty()) {
        return;
    }
    if (!file.endsWith(QStringLiteral(".xml"), Qt::CaseInsensitive)) {
        file += QStringLiteral(".xml");
    }
    service_.setActiveScenario(*request);
    const auto errors = service_.saveActiveScenario(toPath(file));
    if (!errors.empty()) {
        if (errors.front().code == codes::kSaveFailed) {
            QMessageBox::warning(this, tr("Cenário não salvo"), qs(errors.front().message));
        } else {
            showErrors(errors);
        }
        return;
    }
    statusBar()->showMessage(tr("Cenário salvo em %1").arg(file), 5000);
}

void MainWindow::copySummary() {
    if (!last_) {
        statusBar()->showMessage(tr("Calcule uma simulação antes de copiar o resumo."), 5000);
        return;
    }
    QGuiApplication::clipboard()->setText(qs(ev::app::buildSummary(last_->first, last_->second)));
    statusBar()->showMessage(tr("Resumo copiado para a área de transferência."), 5000);
}

void MainWindow::addCurveRow() {
    const QSignalBlocker blocker(curveTable_);
    const int row = curveTable_->rowCount();
    curveTable_->insertRow(row);
    for (int col = 0; col < 3; ++col) {
        curveTable_->setItem(row, col, new QTableWidgetItem(QString()));
    }
    curveTable_->setCurrentCell(row, 0);
    curveTable_->editItem(curveTable_->item(row, 0));
}

void MainWindow::removeCurveRow() {
    const int row = curveTable_->currentRow() >= 0 ? curveTable_->currentRow() : curveTable_->rowCount() - 1;
    if (row >= 0) {
        const QSignalBlocker blocker(curveTable_);
        curveTable_->removeRow(row);
        liveTimer_->start();
    }
}