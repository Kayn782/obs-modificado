#include "obs-setup-wizard.h"

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/config-file.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

#include <QApplication>
#include <QMessageBox>
#include <QFrame>
#include <QGridLayout>
#include <QScrollArea>
#include <QFont>
#include <QPalette>
#include <QStyleOption>
#include <QPainter>

// ─────────────────────────────────────────────────────────────────────────────
// Plugin entry points
// ─────────────────────────────────────────────────────────────────────────────

bool obs_module_load(void)
{
    blog(LOG_INFO, "[obs-setup-wizard] version %s loaded", PLUGIN_VERSION);

    // Add menu entry under Tools
    obs_frontend_add_tools_menu_item(
        "Setup Wizard", // visible name
        [](void *) {
            QWidget *parent = (QWidget *)obs_frontend_get_main_window();
            SetupWizardDialog *dlg = new SetupWizardDialog(parent);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            dlg->exec();
        },
        nullptr);

    // Auto-show on first run
    config_t *global = obs_frontend_get_global_config();
    bool firstRun = config_get_bool(global, PLUGIN_NAME, "first_run_done") == false;
    if (firstRun) {
        QMetaObject::invokeMethod(
            qApp,
            []() {
                QWidget *parent = (QWidget *)obs_frontend_get_main_window();
                SetupWizardDialog *dlg = new SetupWizardDialog(parent);
                dlg->setAttribute(Qt::WA_DeleteOnClose);
                dlg->exec();
            },
            Qt::QueuedConnection);
        config_set_bool(global, PLUGIN_NAME, "first_run_done", true);
        config_save(global);
    }

    return true;
}

void obs_module_unload(void) {}

// ─────────────────────────────────────────────────────────────────────────────
// SetupWizardDialog
// ─────────────────────────────────────────────────────────────────────────────

SetupWizardDialog::SetupWizardDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("OBS Setup Wizard");
    setMinimumWidth(900);
    setMinimumHeight(650);

    // ── Main layout ──────────────────────────────────────────────────────────
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(0);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    // Header
    auto *header = new QFrame(this);
    header->setObjectName("wizardHeader");
    header->setStyleSheet(
        "#wizardHeader { background: #1E1E2E; padding: 16px 24px; }");
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(24, 16, 24, 16);

    auto *titleLabel = new QLabel("🔥 MI NUEVO WIZARD 🔥", header);
    titleLabel->setStyleSheet(
        "color: #FFFFFF; font-size: 16px; font-weight: 600;");

    m_stepLabel = new QLabel("Paso 1 de 4", header);
    m_stepLabel->setStyleSheet("color: #888; font-size: 13px;");
    m_progressBar = new QProgressBar(header);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(25);
    m_progressBar->setFixedWidth(350);
    m_progressBar->setFixedHeight(20);
    m_progressBar->setStyleSheet(
    "QProgressBar {"
    "background:#101018;"
    "border:2px solid #3A96DD;"
    "border-radius:8px;"
    "color:white;"
    "font-weight:bold;"
    "text-align:center;" "}"
    "QProgressBar::chunk {" "background:#3A96DD;" "border-radius:6px;""}");
    m_progressBar->setTextVisible(true);

    m_progressBar->setStyleSheet(
    "QProgressBar {"
    "background:#1A1A24;"
    "border:1px solid #2A2A3E;"
    "border-radius:6px;"
    "text-align:center;"
    "height:12px;"
    "}"
    "QProgressBar::chunk {"
    "background:#3A96DD;"
    "border-radius:6px;"
    "}"
);

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_progressBar);
    headerLayout->addSpacing(10);
    headerLayout->addWidget(m_stepLabel);

    // Pages
    m_pages = new QStackedWidget(this);
    m_pages->addWidget(createUseCasePage());
    m_pages->addWidget(createHardwarePage());
    m_pages->addWidget(createResolutionPage());
    m_pages->addWidget(createSummaryPage());

    // Navigation footer
    auto *footer = new QFrame(this);
    footer->setObjectName("wizardFooter");
    footer->setStyleSheet(
        "#wizardFooter { background: #13131F; border-top: 1px solid #2A2A3E; "
        "padding: 12px 24px; }");
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(24, 12, 24, 12);

    m_btnBack = new QPushButton("← Atrás", footer);
    m_btnBack->setEnabled(false);
    m_btnBack->setStyleSheet(
        "QPushButton { background: transparent; border: 1px solid #444; "
        "border-radius: 6px; color: #aaa; padding: 8px 18px; font-size: 13px; }"
        "QPushButton:hover { background: #2A2A3E; }"
        "QPushButton:disabled { color: #555; border-color: #333; }");

    m_btnNext = new QPushButton("Siguiente →", footer);
    m_btnNext->setEnabled(false);
    m_btnNext->setStyleSheet(
        "QPushButton { background: #3A96DD; border: none; border-radius: 6px; "
        "color: white; padding: 8px 22px; font-size: 13px; font-weight: 600; }"
        "QPushButton:hover { background: #3C1FAF; }"
        "QPushButton:disabled { background: #333; color: #666; }");

    footerLayout->addWidget(m_btnBack);
    footerLayout->addStretch();
    footerLayout->addWidget(m_btnNext);

    rootLayout->addWidget(header);
    rootLayout->addWidget(m_pages, 1);
    rootLayout->addWidget(footer);

    // ── Connections ──────────────────────────────────────────────────────────
    connect(m_btnNext, &QPushButton::clicked, this, &SetupWizardDialog::nextStep);
    connect(m_btnBack, &QPushButton::clicked, this, &SetupWizardDialog::prevStep);

    // Global dark background
    setStyleSheet("QDialog { background: #13131F; } "
                  "QScrollArea { background: transparent; border: none; }");
}

// ─────────────────────────────────────────────────────────────────────────────
// Pages
// ─────────────────────────────────────────────────────────────────────────────

QWidget *SetupWizardDialog::createUseCasePage()
{
    auto *page       = new QWidget;
    auto *scroll     = new QScrollArea;
    auto *container  = new QWidget;
    auto *layout     = new QVBoxLayout(container);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    auto *questionLabel = new QLabel("¿Para qué vas a usar OBS?", container);
    questionLabel->setStyleSheet(
        "color: #E0DEFF; font-size: 15px; font-weight: 600; margin-bottom: 4px;");
    layout->addWidget(questionLabel);

    auto *subLabel = new QLabel(
        "Selecciona tu caso de uso principal — ajustaremos todo automáticamente.",
        container);
    subLabel->setStyleSheet("color: #888; font-size: 13px; margin-bottom: 12px;");
    subLabel->setWordWrap(true);
    layout->addWidget(subLabel);

    // Button grid
    auto *grid   = new QWidget(container);
    auto *gLayout = new QGridLayout(grid);
    gLayout->setSpacing(10);
    gLayout->setContentsMargins(0, 0, 0, 0);

    auto *group = new QButtonGroup(this);
    group->setExclusive(true);

    struct UseCaseOption {
        QString icon, title, desc;
    };
    QList<UseCaseOption> options = {
        {"📡", "Streaming multiplataforma",  "Twitch, YouTube, Kick simultáneamente"},
        {"🎮", "Gameplay alta calidad",       "Grabación 4K/1080p para YouTube"},
        {"🎙️",  "Podcasts y entrevistas",     "Video + audio multi-fuente en vivo"},
        {"📚", "Tutoriales y cursos",         "Captura de pantalla con webcam"},
        {"🎬", "Eventos en vivo",             "Producción profesional multicámara"},
        {"✂️",  "Clips y highlights",          "Grabaciones cortas de momentos"},
    };

    for (int i = 0; i < options.size(); ++i) {
        const auto &opt = options[i];
        auto *btn = makeOptionButton(
            opt.title, opt.desc, opt.icon, group, i);
        gLayout->addWidget(btn, i / 2, i % 2);
    }

    layout->addWidget(grid);
    layout->addStretch();

    connect(group,
            QOverload<int>::of(&QButtonGroup::idClicked),
            this,
            &SetupWizardDialog::onUseCaseSelected);

    scroll->setWidget(container);
    scroll->setWidgetResizable(true);

    auto *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scroll);
    return page;
}

QWidget *SetupWizardDialog::createHardwarePage()
{
    auto *page      = new QWidget;
    auto *layout    = new QVBoxLayout(page);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    auto *label = new QLabel("¿Qué tan potente es tu PC?", page);
    label->setStyleSheet(
        "color: #E0DEFF; font-size: 15px; font-weight: 600; margin-bottom: 4px;");
    layout->addWidget(label);

    auto *sub = new QLabel(
        "El encoder recomendado depende de tu GPU y CPU.", page);
    sub->setStyleSheet("color: #888; font-size: 13px; margin-bottom: 12px;");
    layout->addWidget(sub);

    auto *group = new QButtonGroup(this);

    struct HWOption { QString icon, title, desc; };
    QList<HWOption> opts = {
        {"🖥️",  "PC básica",   "4-8 GB RAM, GPU integrada o GTX 1060"},
        {"💻", "PC media",    "16 GB RAM, RTX 3060 / RX 6600"},
        {"🚀", "PC potente",  "32+ GB RAM, RTX 4070+ / RX 7800+"},
    };

    auto *grid    = new QWidget(page);
    auto *gLayout = new QHBoxLayout(grid);
    gLayout->setSpacing(10);
    gLayout->setContentsMargins(0, 0, 0, 0);

    for (int i = 0; i < opts.size(); ++i) {
        const auto &o = opts[i];
        auto *btn = makeOptionButton(o.title, o.desc, o.icon, group, i);
        gLayout->addWidget(btn);
    }

    layout->addWidget(grid);
    layout->addStretch();

    connect(group,
            QOverload<int>::of(&QButtonGroup::idClicked),
            this,
            &SetupWizardDialog::onHardwareSelected);

    return page;
}

QWidget *SetupWizardDialog::createResolutionPage()
{
    auto *page   = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    auto *label = new QLabel("¿Resolución y FPS objetivo?", page);
    label->setStyleSheet(
        "color: #E0DEFF; font-size: 15px; font-weight: 600; margin-bottom: 4px;");
    layout->addWidget(label);

    auto *sub = new QLabel(
        "Define la calidad de tu stream o grabación.", page);
    sub->setStyleSheet("color: #888; font-size: 13px; margin-bottom: 12px;");
    layout->addWidget(sub);

    auto *group = new QButtonGroup(this);

    struct ResOption { QString icon, title, desc; };
    QList<ResOption> opts = {
        {"📱", "720p / 30 FPS",   "Streaming con internet moderado"},
        {"🖥️",  "1080p / 60 FPS", "Estándar profesional de streaming"},
        {"🔲", "1440p / 60 FPS", "Alta calidad para grabación local"},
        {"🎯", "4K / 30 FPS",    "Máxima calidad para YouTube"},
    };

    auto *grid    = new QWidget(page);
    auto *gLayout = new QGridLayout(grid);
    gLayout->setSpacing(10);
    gLayout->setContentsMargins(0, 0, 0, 0);

    for (int i = 0; i < opts.size(); ++i) {
        const auto &o = opts[i];
        auto *btn = makeOptionButton(o.title, o.desc, o.icon, group, i);
        gLayout->addWidget(btn, i / 2, i % 2);
    }

    layout->addWidget(grid);
    layout->addStretch();

    connect(group,
            QOverload<int>::of(&QButtonGroup::idClicked),
            this,
            &SetupWizardDialog::onResolutionSelected);

    return page;
}

QWidget *SetupWizardDialog::createSummaryPage()
{
    auto *page   = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    auto *label = new QLabel("Tu configuración recomendada", page);
    label->setStyleSheet(
        "color: #E0DEFF; font-size: 15px; font-weight: 600; margin-bottom: 4px;");
    layout->addWidget(label);

    auto *card = new QFrame(page);
    card->setObjectName("summaryCard");
    card->setStyleSheet(
        "#summaryCard { background: #1E1E2E; border: 1px solid #2A2A3E; "
        "border-radius: 10px; padding: 16px; }");
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setSpacing(10);

    auto makeRow = [&](const QString &key, QLabel *&valLabel) {
        auto *row = new QHBoxLayout;
        auto *k   = new QLabel(key, card);
        k->setStyleSheet("color: #888; font-size: 13px;");
        valLabel  = new QLabel("—", card);
        valLabel->setStyleSheet("color: #C9B8FF; font-size: 13px; font-weight: 600;");
        row->addWidget(k);
        row->addStretch();
        row->addWidget(valLabel);
        cardLayout->addLayout(row);

        auto *sep = new QFrame(card);
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("color: #2A2A3E;");
        cardLayout->addWidget(sep);
    };

    makeRow("Caso de uso",   m_sumUseCase);
    makeRow("Hardware",      m_sumHardware);
    makeRow("Resolución",    m_sumResolution);
    makeRow("Encoder video", m_sumEncoder);
    makeRow("Bitrate",       m_sumBitrate);
    makeRow("Plugin sugerido", m_sumPlugin);

    layout->addWidget(card);

    auto *applyBtn = new QPushButton("✓  Aplicar configuración a OBS", page);
    applyBtn->setStyleSheet(
        "QPushButton { background: #1D9E75; border: none; border-radius: 8px; "
        "color: white; padding: 12px; font-size: 14px; font-weight: 600; "
        "margin-top: 8px; }"
        "QPushButton:hover { background: #17806E; }");
    connect(applyBtn, &QPushButton::clicked,
            this,     &SetupWizardDialog::applyConfiguration);
    layout->addWidget(applyBtn);
    layout->addStretch();

    return page;
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

QPushButton *SetupWizardDialog::makeOptionButton(
    const QString &title, const QString &desc,
    const QString &icon,  QButtonGroup *group, int id)
{
    auto *btn = new QPushButton;
    btn->setCheckable(true);
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    btn->setMinimumHeight(130);

    // Custom widget inside
    auto *inner   = new QWidget(btn);
    auto *iLayout = new QVBoxLayout(inner);
    iLayout->setContentsMargins(12, 10, 12, 10);
    iLayout->setSpacing(4);

    auto *iconLabel  = new QLabel(icon + "  " + title, inner);
    iconLabel->setStyleSheet(
        "font-size: 14px; font-weight: 600; color: #E0DEFF;");
    auto *descLabel  = new QLabel(desc, inner);
    descLabel->setStyleSheet("font-size: 12px; color: #888;");
    descLabel->setWordWrap(true);

    iLayout->addWidget(iconLabel);
    iLayout->addWidget(descLabel);

    auto *outerLayout = new QVBoxLayout(btn);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(inner);

    btn->setStyleSheet(
        "QPushButton { background: #1E1E2E; border: 1px solid #2A2A3E; "
        "border-radius: 8px; text-align: left; }"
        "QPushButton:hover {  border: 2px solid #3A96DD; background: #252533;}"
        "QPushButton:checked { border: 2px solid #58A6FF; background: #1F2A38;}");

    group->addButton(btn, id);
    return btn;
}

QMap<int, ProfileConfig> SetupWizardDialog::buildConfigMap() const
{
    // key = useCase * 100 + resolution
    // For simplicity we key by use-case and override by resolution
    QMap<int, ProfileConfig> map;

    // 0 = streaming multi-platform
    map[0] = {"x264", "AAC", "4500-6000 kbps", "160 kbps",
              1920, 1080, 60, "obs-multi-rtmp", ""};
    // 1 = gameplay high quality
    map[1] = {"NVENC HEVC", "FLAC", "40 Mbps (CQP 18)", "320 kbps",
              3840, 2160, 30, "obs-replay-buffer", ""};
    // 2 = podcast
    map[2] = {"x264 lossless", "AAC multi-track", "12 Mbps", "320 kbps",
              1920, 1080, 60, "obs-ndi", ""};
    // 3 = tutorials
    map[3] = {"x264 / NVENC", "AAC", "8-15 Mbps", "192 kbps",
              1920, 1080, 30, "obs-browser", ""};
    // 4 = live events
    map[4] = {"NVENC B-frames", "AAC", "15-25 Mbps", "320 kbps",
              1920, 1080, 60, "obs-websocket", ""};
    // 5 = clips
    map[5] = {"GPU lossless", "PCM 48kHz", "Lossless", "PCM",
              1920, 1080, 60, "obs-replay-buffer", ""};

    return map;
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────────────────────────────────────

void SetupWizardDialog::onUseCaseSelected(int id)
{
    m_useCase = id;
    m_btnNext->setEnabled(true);
}

void SetupWizardDialog::onHardwareSelected(int id)
{
    m_hardware = id;
    m_btnNext->setEnabled(true);
}

void SetupWizardDialog::onResolutionSelected(int id)
{
    m_resolution = id;
    m_btnNext->setEnabled(true);
}

void SetupWizardDialog::nextStep()
{
    if (m_currentStep < 3) {
        m_currentStep++;
        m_pages->setCurrentIndex(m_currentStep);
        m_btnBack->setEnabled(true);
        m_btnNext->setEnabled(false);

        QStringList steps = {"Paso 1 de 4", "Paso 2 de 4",
                             "Paso 3 de 4", "Paso 4 de 4"};
        m_stepLabel->setText(steps[m_currentStep]);
        m_progressBar->setValue((m_currentStep + 1) * 25);

        if (m_currentStep == 3) {
            updateSummaryPage();
            m_btnNext->setEnabled(false);
            m_btnNext->setText("Listo");
        }
    } else {
        accept();
    }
}

void SetupWizardDialog::prevStep()
{
    if (m_currentStep > 0) {
        m_currentStep--;
        m_pages->setCurrentIndex(m_currentStep);
        m_btnNext->setEnabled(true);
        m_btnBack->setEnabled(m_currentStep > 0);

        QStringList steps = {"Paso 1 de 4", "Paso 2 de 4",
                             "Paso 3 de 4", "Paso 4 de 4"};
        m_stepLabel->setText(steps[m_currentStep]);
        m_progressBar->setValue((m_currentStep + 1) * 25);
        m_btnNext->setText("Siguiente →");
    }
}

void SetupWizardDialog::updateSummaryPage()
{
    QStringList useCases = {"Streaming multiplataforma", "Gameplay alta calidad",
                            "Podcasts y entrevistas",   "Tutoriales y cursos",
                            "Eventos en vivo",           "Clips y highlights"};
    QStringList hardware = {"PC básica", "PC media", "PC potente"};
    QStringList resolutions = {"720p / 30 FPS", "1080p / 60 FPS",
                               "1440p / 60 FPS", "4K / 30 FPS"};

    auto map = buildConfigMap();
    ProfileConfig cfg = map.value(m_useCase);

    // Override resolution
    if (m_resolution == 0) { cfg.width = 1280; cfg.height = 720;  cfg.fps = 30; }
    if (m_resolution == 1) { cfg.width = 1920; cfg.height = 1080; cfg.fps = 60; }
    if (m_resolution == 2) { cfg.width = 2560; cfg.height = 1440; cfg.fps = 60; }
    if (m_resolution == 3) { cfg.width = 3840; cfg.height = 2160; cfg.fps = 30; }

    // Override encoder based on hardware
    if (m_hardware == 0) cfg.encoderVideo = "x264 (software)";
    if (m_hardware == 1) cfg.encoderVideo = "NVENC / AMF";
    if (m_hardware == 2) cfg.encoderVideo = "NVENC AV1 / AV1";

    m_sumUseCase->setText(m_useCase    >= 0 ? useCases[m_useCase]    : "—");
    m_sumHardware->setText(m_hardware  >= 0 ? hardware[m_hardware]   : "—");
    m_sumResolution->setText(m_resolution >= 0 ? resolutions[m_resolution] : "—");
    m_sumEncoder->setText(cfg.encoderVideo);
    m_sumBitrate->setText(cfg.bitrateVideo);
    m_sumPlugin->setText(cfg.suggestedPlugin);
}

void SetupWizardDialog::applyConfiguration()
{
    auto map = buildConfigMap();
    ProfileConfig cfg = map.value(m_useCase);

    if (m_resolution == 0) { cfg.width = 1280; cfg.height = 720;  cfg.fps = 30; }
    if (m_resolution == 1) { cfg.width = 1920; cfg.height = 1080; cfg.fps = 60; }
    if (m_resolution == 2) { cfg.width = 2560; cfg.height = 1440; cfg.fps = 60; }
    if (m_resolution == 3) { cfg.width = 3840; cfg.height = 2160; cfg.fps = 30; }

    if (m_hardware == 0) cfg.encoderVideo = "obs_x264";
    if (m_hardware == 1) cfg.encoderVideo = "jim_nvenc";
    if (m_hardware == 2) cfg.encoderVideo = "jim_av1_nvenc";

    applyOBSSettings(cfg);

    blog(LOG_INFO, "[obs-setup-wizard] Configuration applied: %s %dx%d@%d",
         cfg.encoderVideo.toUtf8().constData(), cfg.width, cfg.height, cfg.fps);

    accept();
}

void SetupWizardDialog::applyOBSSettings(const ProfileConfig &cfg)
{
    config_t *basicCfg = obs_frontend_get_profile_config();
    if (!basicCfg) return;

    // Video output
    config_set_uint(basicCfg, "Video", "BaseCX",   (uint64_t)cfg.width);
    config_set_uint(basicCfg, "Video", "BaseCY",   (uint64_t)cfg.height);
    config_set_uint(basicCfg, "Video", "OutputCX", (uint64_t)cfg.width);
    config_set_uint(basicCfg, "Video", "OutputCY", (uint64_t)cfg.height);
    config_set_uint(basicCfg, "Video", "FPSNum",   (uint64_t)cfg.fps);
    config_set_uint(basicCfg, "Video", "FPSDen",   1);

    // Encoder
    config_set_string(basicCfg, "SimpleOutput", "StreamEncoder",
                      cfg.encoderVideo.toUtf8().constData());
    config_set_string(basicCfg, "SimpleOutput", "RecEncoder",
                      cfg.encoderVideo.toUtf8().constData());

    // Save config — OBS picks up changes on next launch
    config_save(basicCfg);

    QMessageBox::information(this, "OBS Setup Wizard", "Configuracion aplicada. Reinicia OBS para que los cambios tomen efecto.");
}

