#include "ValidationDashboard.h"

#include <QAbstractItemView>
#include <QByteArray>
#include <QColor>
#include <QFont>
#include <QLocale>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <algorithm>

namespace {


constexpr const char* kDashboardStyle = R"(
QDialog#validationDashboard {
    background: #0A1524;
    color: #E8F1FA;
}

QFrame#summaryTile {
    background: #15263C;
    border: 1px solid #2B455F;
    border-radius: 12px;
}

QLabel#tileLabel {
    color: #A1BACD;
    font-size: 10pt;
}

QLabel#tileValue {
    color: #E9F7FF;
    font-size: 23pt;
    font-weight: 700;
}

QLabel#heading {
    color: #F1FAFF;
    font-size: 18pt;
    font-weight: 700;
}

QLabel#subheading {
    color: #99B6C9;
    font-size: 10pt;
}

QLabel#note {
    background: #182A42;
    color: #B4D0E2;
    border: 1px solid #304D67;
    border-radius: 8px;
    padding: 11px;
}

/* Tabela */
QTableWidget {
    background: #101D2F;
    alternate-background-color: #14263B;
    color: #DBEAF6;
    border: 1px solid #2D475F;
    gridline-color: #263A4F;
    selection-background-color: #246078;
    border-radius: 8px;
}

/* Cabeçalho horizontal e vertical */
QHeaderView::section {
    background: #1E334D;
    color: #C7DEEA;
    padding: 9px;
    font-weight: 600;
    border: none;
    border-bottom: 1px solid #355270;
}

/* CORREÇÃO: canto superior esquerdo da tabela */
QTableCornerButton::section {
    background: #1E334D;
    border: 1px solid #2D475F;
}

/* Botões */
QPushButton {
    padding: 9px 15px;
    border-radius: 8px;
    border: 1px solid #34536A;
    background: #1B3247;
    color: #EAF6FF;
    font-weight: 600;
}

QPushButton#primaryButton {
    background: #0CCABF;
    color: #04252D;
    border: 1px solid #0CCABF;
    font-weight: 700;
}

QPushButton#primaryButton:hover {
    background: #73F3E5;
    border-color: #73F3E5;
}

QPushButton:focus {
    border: 2px solid #25E6D7;
}

/* Barra de progresso */
QProgressBar {
    height: 16px;
    border-radius: 6px;
    border: 1px solid #30495F;
    background: #182A3F;
    text-align: center;
    color: #F0FAFF;
    font-weight: 600;
}

QProgressBar::chunk {
    background: #13CDBC;
    border-radius: 5px;
}
)";


QLabel* addTile(QHBoxLayout* row, const QString& name, QWidget* parent) {
    auto* frame = new QFrame(parent);
    frame->setObjectName("summaryTile");
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(17, 12, 17, 13);
    auto* label = new QLabel(name, frame);
    label->setObjectName("tileLabel");
    auto* value = new QLabel(QStringLiteral("—"), frame);
    value->setObjectName("tileValue");
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(label);
    layout->addWidget(value);
    row->addWidget(frame, 1);
    return value;
}

QString text(const std::string& s) { return QString::fromStdString(s); }

QTableWidgetItem* cell(const QString& value) {
    auto* item = new QTableWidgetItem(value);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

}  // namespace

ValidationDashboard::ValidationDashboard(QWidget* parent) : QDialog(parent) {
    setObjectName(QStringLiteral("validationDashboard"));
    setWindowTitle(tr("Painel de Validação | EV Charge Simulator"));
    resize(1220, 700);
    setMinimumSize(920, 560);
    setStyleSheet(kDashboardStyle);

    auto* root = new QVBoxLayout(this);
    root->setSpacing(14);
    root->setContentsMargins(24, 22, 24, 22);

    auto* heading = new QLabel(tr("Validação do motor de cálculo"), this);
    heading->setObjectName("heading");
    auto* subtitle = new QLabel(tr("Casos de verificação rápida baseados na ERS-VE-001, seção 8.2"), this);
    subtitle->setObjectName("subheading");
    root->addWidget(heading);
    root->addWidget(subtitle);

    auto* tiles = new QHBoxLayout;
    totalValue_ = addTile(tiles, tr("Casos executados"), this);
    passedValue_ = addTile(tiles, tr("Aprovados"), this);
    failedValue_ = addTile(tiles, tr("Reprovados"), this);
    root->addLayout(tiles);

    progress_ = new QProgressBar(this);
    progress_->setMinimum(0);
    progress_->setMaximum(1);
    progress_->setValue(0);
    progress_->setFormat(tr("%p% de cenários aprovados"));
    progress_->setAccessibleName(tr("Percentual de cenários aprovados"));
    root->addWidget(progress_);

    table_ = new QTableWidget(0, 7, this);
    table_->setHorizontalHeaderLabels(
        {tr("ID"), tr("Requisito"), tr("Categoria"), tr("Cenário"),
         tr("Resultado esperado"), tr("Resultado obtido"), tr("Situação")});
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setAlternatingRowColors(true);
    table_->setWordWrap(true);
    table_->verticalHeader()->setVisible(false);
    auto* header = table_->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::Stretch);
    header->setSectionResizeMode(4, QHeaderView::Stretch);
    header->setSectionResizeMode(5, QHeaderView::Stretch);
    header->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    table_->setAccessibleName(tr("Resultados dos testes: esperado, obtido e aprovação"));
    root->addWidget(table_, 1);

    auto* note = new QLabel(tr(
        "Estes são testes rápidos executados no motor real do simulador. A taxa exibida NÃO é cobertura "
        "de linhas de código. A suíte completa Google Test / CTest e o CI devem ser executados separadamente."), this);
    note->setObjectName("note");
    note->setWordWrap(true);
    root->addWidget(note);

    auto* footer = new QHBoxLayout;
    runTime_ = new QLabel(tr("Ainda não executado"), this);
    runTime_->setObjectName("subheading");
    footer->addWidget(runTime_);
    footer->addStretch();
    auto* rerun = new QPushButton(tr("Executar novamente"), this);
    rerun->setObjectName("primaryButton");
    auto* exportButton = new QPushButton(tr("Exportar CSV"), this);
    auto* close = new QPushButton(tr("Fechar"), this);
    footer->addWidget(rerun);
    footer->addWidget(exportButton);
    footer->addWidget(close);
    root->addLayout(footer);

    connect(rerun, &QPushButton::clicked, this, [this] { runChecks(); });
    connect(exportButton, &QPushButton::clicked, this, [this] { exportCsv(); });
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    runChecks();
}

void ValidationDashboard::runChecks() {
    checks_ = ev::validation::runQuickChecks();
    const int total = static_cast<int>(checks_.size());
    const int passed = static_cast<int>(std::count_if(checks_.begin(), checks_.end(),
                                                    [](const auto& c) { return c.passed; }));
    totalValue_->setText(QString::number(total));
    passedValue_->setText(QString::number(passed));
    failedValue_->setText(QString::number(total - passed));
    failedValue_->setStyleSheet(total == passed ? QStringLiteral("color: #4FF2C1;")
                                               : QStringLiteral("color: #FF8D9A;"));
    passedValue_->setStyleSheet(QStringLiteral("color: #4FF2C1;"));
    progress_->setMaximum(std::max(total, 1));
    progress_->setValue(passed);
    table_->setRowCount(total);
    for (int i = 0; i < total; ++i) {
        const auto& check = checks_[static_cast<std::size_t>(i)];
        const QString columns[] = {text(check.id), text(check.requirement), text(check.category),
                                   text(check.description), text(check.expected), text(check.observed),
                                   check.passed ? tr("APROVADO") : tr("REPROVADO")};
        for (int c = 0; c < 7; ++c) {
            auto* item = cell(columns[c]);
            item->setToolTip(columns[c]);
            if (c == 6) {
                item->setForeground(check.passed ? QColor("#4FF2C1") : QColor("#FF8D9A"));
                QFont font = item->font();
                font.setBold(true);
                item->setFont(font);
            }
            table_->setItem(i, c, item);
        }
        table_->resizeRowToContents(i);
    }
    runTime_->setText(tr("Executado em %1 | ERS-VE-001")
                          .arg(QLocale(QLocale::Portuguese, QLocale::Brazil)
                                   .toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
}

void ValidationDashboard::exportCsv() {
    if (checks_.empty()) return;
    const QString filename = QFileDialog::getSaveFileName(this, tr("Exportar evidência dos testes"),
        QStringLiteral("EVCharge_Validacao_UT001-UT014.csv"), tr("CSV (*.csv)"));
    if (filename.isEmpty()) return;
    const QString path = filename.endsWith(QStringLiteral(".csv"), Qt::CaseInsensitive)
                             ? filename : filename + QStringLiteral(".csv");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("Erro ao exportar"), file.errorString());
        return;
    }
    const QByteArray csv = QByteArray::fromStdString(ev::validation::reportAsCsv(checks_));
    const QByteArray utf8Bom("\xEF\xBB\xBF", 3);
    const bool success = file.write(utf8Bom) == utf8Bom.size() && file.write(csv) == csv.size();
    file.close();
    if (!success) {
        QMessageBox::warning(this, tr("Erro ao exportar"), tr("Falha ao gravar o relatório."));
        return;
    }
    QMessageBox::information(this, tr("Relatório exportado"),
                             tr("CSV salvo em:\n%1").arg(path));
}