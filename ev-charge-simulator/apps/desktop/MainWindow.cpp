#include "MainWindow.h"

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
#include <QStatusBar>
#include <QStyle>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <filesystem>

#include "ev/app/ResultPresenter.h"
#include "ev/domain/ValidationError.h"

using namespace ev::domain;

namespace {

constexpr const char* kStyle = R"(
QFrame#card { background: #f4f7fb; border: 1px solid #1b3a5c; border-radius: 6px; }
QLabel#cardTitle { color: #1b3a5c; font-weight: bold; }
QLabel#cardValue { color: #0b1f33; font-size: 18pt; font-weight: bold; }
QLabel#fieldError { color: #b00020; background: #fdecee; padding: 2px 4px; }
QLabel#errorSummary { color: #b00020; background: #fdecee; border: 1px solid #b00020; padding: 6px; }
QLabel#warnings { color: #4a3200; background: #fff4d6; border: 1px solid #8a6d00; padding: 6px; }
QDoubleSpinBox[invalid="true"], QTableWidget[invalid="true"] { border: 2px solid #b00020; }
)";

QString qs(const std::string& text) { return QString::fromStdString(text); }

std::filesystem::path toPath(const QString& path) { return std::filesystem::path(path.toStdU16String()); }

void setInvalid(QWidget* widget, bool invalid) {
    widget->setProperty("invalid", invalid);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}

QLabel* makeCard(const QString& title, QHBoxLayout* row) {
    auto* frame = new QFrame;
    frame->setObjectName("card");
    auto* layout = new QVBoxLayout(frame);
    auto* titleLabel = new QLabel(title);
    titleLabel->setObjectName("cardTitle");
    auto* value = new QLabel(QStringLiteral("—"));
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

MainWindow::MainWindow(ev::app::SimulationService& service, const IVehicleCatalog* catalog, QWidget* parent)
    : QMainWindow(parent), service_(service) {
    if (catalog != nullptr) {
        vehicles_ = catalog->list();
    }
    setWindowTitle(tr("Simulador de Recarga de Veículo Elétrico"));
    setStyleSheet(kStyle);

    auto* inputScroll = new QScrollArea;
    inputScroll->setWidgetResizable(true);
    inputScroll->setWidget(buildInputPanel());

    auto* right = new QWidget;
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->addWidget(buildResultPanel());
    rightLayout->addWidget(buildComparisonPanel(), 1);
    auto* rightScroll = new QScrollArea;
    rightScroll->setWidgetResizable(true);
    rightScroll->setWidget(right);

    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(inputScroll);
    splitter->addWidget(rightScroll);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);

    buildMenu();
    for (const auto key : {Qt::Key_Return, Qt::Key_Enter}) {
        auto* shortcut = new QShortcut(QKeySequence(key), this);
        connect(shortcut, &QShortcut::activated, this, &MainWindow::calculate);
    }

    writeForm(service_.activeScenario());
    statusBar()->showMessage(tr("Preencha os dados e pressione Enter ou \"Calcular\"."));
    resize(1200, 820);
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
        spin->setSpecialValueText(specialText);  // shown when value == minimum (0 = not informed)
    }
    auto* container = new QWidget;
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->addWidget(spin);
    layout->addWidget(addErrorLabel(fieldId));

    auto* caption = new QLabel(label + ":");
    caption->setBuddy(spin);
    form->addRow(caption, container);
    fieldWidgets_[fieldId] = spin;
    return spin;
}

QWidget* MainWindow::buildInputPanel() {
    auto* panel = new QWidget;
    auto* layout = new QVBoxLayout(panel);

    errorSummary_ = new QLabel;
    errorSummary_->setObjectName("errorSummary");
    errorSummary_->setWordWrap(true);
    errorSummary_->setAccessibleName(tr("Resumo de erros"));
    errorSummary_->hide();
    layout->addWidget(errorSummary_);

    auto* vehicleBox = new QGroupBox(tr("1. Veículo"));
    auto* vehicleForm = new QFormLayout(vehicleBox);
    vehicleCombo_ = new QComboBox;
    vehicleCombo_->addItem(tr("Entrada manual"));
    for (const auto& v : vehicles_) {
        vehicleCombo_->addItem(qs(v.name));
    }
    vehicleCombo_->setAccessibleName(tr("Veículo do catálogo"));
    auto* catalogLabel = new QLabel(tr("Catálogo:"));
    catalogLabel->setBuddy(vehicleCombo_);
    vehicleForm->addRow(catalogLabel, vehicleCombo_);
    if (!vehicles_.empty()) {
        auto* note = new QLabel(tr("⚠ Dados do catálogo são aproximados e não verificados. Confira com o fabricante."));
        note->setWordWrap(true);
        vehicleForm->addRow(note);
    }
    capacity_ = addField(vehicleForm, tr("Capacidade útil"), fields::kUsableCapacity, 0, 1000, 2, " kWh");
    vehiclePower_ =
        addField(vehicleForm, tr("Potência máx. aceita pelo veículo"), fields::kVehicleMaxPower, 0, 1000, 2, " kW");
    consumption_ = addField(vehicleForm, tr("Consumo médio"), fields::kConsumption, 0, 100, 2, " kWh/100 km");
    ratedRange_ = addField(vehicleForm, tr("Autonomia nominal (100%)"), fields::kRatedRange, 0, 5000, 0, " km",
                           tr("Não informado"));
    layout->addWidget(vehicleBox);

    auto* chargerBox = new QGroupBox(tr("2. Carregador"));
    auto* chargerForm = new QFormLayout(chargerBox);
    chargerType_ = new QComboBox;
    chargerType_->addItems({QStringLiteral("AC"), QStringLiteral("DC")});
    chargerType_->setAccessibleName(tr("Tipo de carregador"));
    auto* typeLabel = new QLabel(tr("Tipo:"));
    typeLabel->setBuddy(chargerType_);
    chargerForm->addRow(typeLabel, chargerType_);
    chargerPower_ = addField(chargerForm, tr("Potência do carregador"), fields::kChargerPower, 0, 1000, 2, " kW");
    installationLimit_ = addField(chargerForm, tr("Limite da instalação"), fields::kInstallationLimit, 0, 1000, 2,
                                  " kW", tr("Não informado"));
    layout->addWidget(chargerBox);

    auto* batteryBox = new QGroupBox(tr("3. Bateria e eficiência"));
    auto* batteryForm = new QFormLayout(batteryBox);
    currentSoc_ = addField(batteryForm, tr("SoC atual"), fields::kCurrentSoc, 0, 100, 1, " %");
    targetSoc_ = addField(batteryForm, tr("SoC desejado"), fields::kTargetSoc, 0, 100, 1, " %");
    efficiency_ = addField(batteryForm, tr("Eficiência de recarga"), fields::kEfficiency, 0, 100, 1, " %");
    layout->addWidget(batteryBox);

    auto* tariffBox = new QGroupBox(tr("4. Tarifa (BRL)"));
    auto* tariffForm = new QFormLayout(tariffBox);
    energyRate_ = addField(tariffForm, tr("Tarifa de energia"), fields::kEnergyRate, 0, 100, 2, " R$/kWh");
    fixedFee_ = addField(tariffForm, tr("Taxa fixa"), fields::kFixedFee, 0, 10000, 2, " R$");
    minuteRate_ = addField(tariffForm, tr("Tarifa por minuto"), fields::kMinuteRate, 0, 100, 2, " R$/min");
    layout->addWidget(tariffBox);

    auto* curveBox = new QGroupBox(tr("5. Curva de carga (opcional)"));
    auto* curveLayout = new QVBoxLayout(curveBox);
    useCurve_ = new QCheckBox(tr("Usar curva segmentada por faixa de SoC"));
    curveTable_ = new QTableWidget(0, 3);
    curveTable_->setHorizontalHeaderLabels({tr("De SoC (%)"), tr("Até SoC (%)"), tr("Potência máx. (kW)")});
    curveTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    curveTable_->setAccessibleName(tr("Faixas da curva de carga"));
    curveTable_->setMinimumHeight(120);
    auto* curveHelp = new QLabel(tr("Fora das faixas usa-se a potência máxima do veículo. Faixas: [de, até)."));
    curveHelp->setWordWrap(true);
    auto* curveButtons = new QHBoxLayout;
    auto* addRow = new QPushButton(tr("Adicionar faixa"));
    auto* removeRow = new QPushButton(tr("Remover faixa"));
    curveButtons->addWidget(addRow);
    curveButtons->addWidget(removeRow);
    curveButtons->addStretch();
    curveLayout->addWidget(useCurve_);
    curveLayout->addWidget(curveHelp);
    curveLayout->addWidget(curveTable_);
    curveLayout->addWidget(addErrorLabel(fields::kCurve));
    curveLayout->addLayout(curveButtons);
    fieldWidgets_[fields::kCurve] = curveTable_;
    for (QWidget* w : {static_cast<QWidget*>(curveTable_), static_cast<QWidget*>(addRow),
                       static_cast<QWidget*>(removeRow)}) {
        w->setEnabled(false);
        connect(useCurve_, &QCheckBox::toggled, w, &QWidget::setEnabled);
    }
    connect(addRow, &QPushButton::clicked, this, &MainWindow::addCurveRow);
    connect(removeRow, &QPushButton::clicked, this, &MainWindow::removeCurveRow);
    layout->addWidget(curveBox);

    auto* actions = new QHBoxLayout;
    auto* calculateButton = new QPushButton(tr("Calcular (Enter)"));
    calculateButton->setDefault(true);
    auto* compareButton = new QPushButton(tr("Adicionar à comparação"));
    actions->addWidget(calculateButton);
    actions->addWidget(compareButton);
    layout->addLayout(actions);
    layout->addStretch();

    connect(calculateButton, &QPushButton::clicked, this, &MainWindow::calculate);
    connect(compareButton, &QPushButton::clicked, this, &MainWindow::addToComparison);
    connect(vehicleCombo_, &QComboBox::currentIndexChanged, this, &MainWindow::onVehicleSelected);
    connect(chargerType_, &QComboBox::currentIndexChanged, this, &MainWindow::onChargerTypeChanged);
    return panel;
}

QWidget* MainWindow::buildResultPanel() {
    auto* box = new QGroupBox(tr("Resultado"));
    auto* layout = new QVBoxLayout(box);

    auto* cards = new QHBoxLayout;
    timeCard_ = makeCard(tr("Tempo estimado"), cards);
    costCard_ = makeCard(tr("Custo total"), cards);
    rangeCard_ = makeCard(tr("Autonomia adicionada"), cards);
    layout->addLayout(cards);

    detailsToggle_ = new QToolButton;
    detailsToggle_->setCheckable(true);
    detailsToggle_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    detailsToggle_->setArrowType(Qt::RightArrow);
    detailsToggle_->setText(tr("Detalhes e premissas"));
    details_ = new QWidget;
    auto* detailsLayout = new QVBoxLayout(details_);
    detailsText_ = new QLabel;
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

    warnings_ = new QLabel;
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
    comparisonTable_ = new QTableWidget(5, 0);
    comparisonTable_->setVerticalHeaderLabels(
        {tr("Tempo"), tr("Custo"), tr("Autonomia adicionada"), tr("Energia da rede"), tr("Potência efetiva")});
    comparisonTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    comparisonTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    comparisonTable_->setAccessibleName(tr("Tabela de comparação de cenários"));
    auto* note = new QLabel(tr("★ indica menor custo ou menor tempo; os cenários não são necessariamente equivalentes."));
    note->setWordWrap(true);
    auto* clear = new QPushButton(tr("Limpar comparação"));
    connect(clear, &QPushButton::clicked, this, &MainWindow::clearComparison);
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
        for (int row = 0; row < curveTable_->rowCount(); ++row) {
            double values[3] = {0.0, 0.0, 0.0};
            for (int col = 0; col < 3; ++col) {
                const QTableWidgetItem* item = curveTable_->item(row, col);
                if (item == nullptr || !parseLocaleNumber(item->text(), values[col])) {
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
    rangeCard_->setText(qs(v.addedRange) + "\n" + tr("final estimada: ") + qs(v.finalRange));

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
    const auto highlights = ev::app::highlight(entries);
    comparisonTable_->setColumnCount(static_cast<int>(entries.size()));
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto v = ev::app::presentResult(entries[i].request, entries[i].result);
        auto* header = new QTableWidgetItem(qs(entries[i].label));
        header->setToolTip(qs(entries[i].request.vehicle.name));
        comparisonTable_->setHorizontalHeaderItem(static_cast<int>(i), header);
        QString time = qs(v.duration);
        if (highlights.fastest == i) {
            time += tr("  ★ menor tempo");
        }
        QString cost = qs(v.totalCost);
        if (highlights.lowestCost == i) {
            cost += tr("  ★ menor custo");
        }
        const int col = static_cast<int>(i);
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
        curveTable_->removeRow(row);
    }
}
