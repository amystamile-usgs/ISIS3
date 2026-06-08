#include "DashboardWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QPixmap>
#include <QFileInfo>
#include <QEvent>
#include <QMouseEvent>

namespace Isis {

  DashboardWidget::DashboardWidget(QWidget *parent) : QWidget(parent) {
    m_activeViewWidget = nullptr;
    m_viewContainer = nullptr;
    m_targetBodyCard = nullptr;
    m_spacecraftCard = nullptr;
    m_imageCount = 0;
    m_controlNetworkCount = 0;
    setupUI();
  }

  DashboardWidget::~DashboardWidget() {
  }

  bool DashboardWidget::eventFilter(QObject *obj, QEvent *event) {
    // Handle clicks on image labels
    if (event->type() == QEvent::MouseButtonPress) {
      QLabel *label = qobject_cast<QLabel*>(obj);
      if (label && label->property("isImageLabel").toBool()) {
        QString imagePath = label->property("imagePath").toString();
        if (!imagePath.isEmpty()) {
          emit imageClicked(imagePath);
          return true;
        }
      }
    }
    return QWidget::eventFilter(obj, event);
  }

  void DashboardWidget::setupUI() {
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("DashboardWidget { background: transparent; }");

    m_mainLayout = new QGridLayout(this);
    m_mainLayout->setSpacing(16);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);

    QFrame *iconCard = createDiagramCard();
    m_mainLayout->addWidget(iconCard, 0, 0, 2, 1);

    QFrame *welcomeCard = createWelcomeCard();
    m_mainLayout->addWidget(welcomeCard, 0, 1, 2, 1);

    QPushButton *closeButton = new QPushButton("×");
    closeButton->setStyleSheet(
      "QPushButton { "
      "  background: transparent; "
      "  color: #8090b0; "
      "  border: none; "
      "  font-size: 24px; "
      "  padding: 4px; "
      "  min-width: 30px; "
      "  max-width: 30px; "
      "  min-height: 30px; "
      "  max-height: 30px; "
      "}"
      "QPushButton:hover { "
      "  background: rgba(255, 100, 100, 0.3); "
      "  color: #ff6666; "
      "}"
    );
    closeButton->setCursor(Qt::PointingHandCursor);
    connect(closeButton, &QPushButton::clicked, [welcomeCard, iconCard]() {
      welcomeCard->hide();
      iconCard->hide();
    });

    closeButton->setParent(welcomeCard);
    closeButton->move(welcomeCard->width() - 35, 5);

    m_imagesCard = createImagesCard();
    m_mainLayout->addWidget(m_imagesCard, 2, 0);

    QFrame *shapesCard = createStatusCard("Shapes", "0", "Items", "status-card");
    m_mainLayout->addWidget(shapesCard, 2, 1);

    m_controlNetworksCard = createControlNetworksCard();
    m_mainLayout->addWidget(m_controlNetworksCard, 2, 2);

    m_targetBodyCard = createTargetBodyCard();
    m_mainLayout->addWidget(m_targetBodyCard, 3, 0);

    m_spacecraftCard = createSpacecraftCard();
    m_mainLayout->addWidget(m_spacecraftCard, 3, 1);

    QFrame *resultsCard = createStatusCard("Results", "0", "Items", "status-card");
    m_mainLayout->addWidget(resultsCard, 3, 2);

    int row = 4;
    m_viewContainer = new QFrame();
    m_viewContainer->setProperty("class", "view-container");
    m_viewContainer->setMinimumHeight(400);
    m_viewContainer->setStyleSheet(
      "QFrame { "
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
      "              stop:0 rgba(20, 20, 40, 0.6), stop:1 rgba(15, 15, 30, 0.6)); "
      "  border: 1px solid rgba(100, 80, 180, 0.4); "
      "  border-radius: 18px; "
      "}"
    );
    QVBoxLayout *viewLayout = new QVBoxLayout(m_viewContainer);
    viewLayout->setContentsMargins(0, 0, 0, 0);
    m_viewContainer->hide(); // Hidden by default until a view is opened
    m_mainLayout->addWidget(m_viewContainer, row, 0, 1, 3);

    m_mainLayout->setColumnStretch(0, 1);
    m_mainLayout->setColumnStretch(1, 2);
    m_mainLayout->setColumnStretch(2, 1);
  }

  QFrame* DashboardWidget::createStatusCard(const QString &title, const QString &value,
                                            const QString &label, const QString &colorClass) {
    QFrame *card = new QFrame();
    card->setProperty("class", colorClass);
    card->setMinimumSize(200, 180);
    card->setMaximumHeight(220);
    card->setStyleSheet(
      "QFrame { "
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
      "              stop:0 rgba(30, 50, 60, 0.7), stop:1 rgba(25, 40, 48, 0.7)); "
      "  border: 1px solid rgba(60, 120, 140, 0.5); "
      "  border-radius: 18px; "
      "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(4);

    QLabel *titleLabel = new QLabel(title);
    titleLabel->setStyleSheet(
      "font-size: 11px; "
      "color: #90b0c0; "
      "font-weight: 700; "
      "text-transform: uppercase; "
      "letter-spacing: 1px; "
      "background: transparent;"
    );
    layout->addWidget(titleLabel);

    layout->addSpacing(8);

    // Content area - this is where we'll add images/info/lists
    QWidget *contentArea = new QWidget();
    contentArea->setStyleSheet("background: transparent;");
    QVBoxLayout *contentLayout = new QVBoxLayout(contentArea);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(4);

    QLabel *emptyLabel = new QLabel("No " + title.toLower());
    emptyLabel->setStyleSheet(
      "color: #607080;"
      "font-size: 12px; "
      "font-style: italic; "
      "background: transparent;"
    );
    emptyLabel->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(emptyLabel);

    layout->addWidget(contentArea, 1);

    return card;
  }

  QFrame* DashboardWidget::createWelcomeCard() {
    QFrame *card = new QFrame();
    card->setProperty("class", "status-card");
    card->setMinimumSize(400, 300);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(28, 28, 28, 28);
    layout->setSpacing(12);

    // Header
    QLabel *headerLabel = new QLabel("SYSTEM STATUS");
    headerLabel->setProperty("class", "system-header");
    headerLabel->setStyleSheet("font-size: 10px; color: #7c8ba0; font-weight: 700; "
                              "text-transform: uppercase; letter-spacing: 2px;");
    layout->addWidget(headerLabel);

    QLabel *titleLabel = new QLabel("Welcome to Astroset");
    titleLabel->setProperty("class", "welcome-title");
    titleLabel->setStyleSheet("font-size: 32px; font-weight: 800; color: #ffffff; "
                             "letter-spacing: -1px; margin: 8px 0;");
    layout->addWidget(titleLabel);

    layout->addSpacing(16);

    // Instructions
    QLabel *instructionsLabel = new QLabel(
      "<div style='line-height: 1.8;'>"
      "<p style='margin: 8px 0;'><span style='color: #00ff88;'>1.</span> <b>Import images:</b> File → Import → Import Images</p>"
      "<p style='margin: 8px 0;'><span style='color: #00ff88;'>2.</span> <b>Explore data:</b> Use the tree on the left to browse imported images</p>"
      "<p style='margin: 8px 0;'><span style='color: #00ff88;'>3.</span> <b>Open views:</b> Right-click on images to open viewers (DN view, footprint view, etc.)</p>"
      "<p style='margin: 8px 0;'><span style='color: #00ff88;'>4.</span> <b>Save your work:</b> File → Save Project (optional, saves state for later)</p>"
      "</div>"
    );
    instructionsLabel->setWordWrap(true);
    instructionsLabel->setTextFormat(Qt::RichText);
    instructionsLabel->setStyleSheet("font-size: 14px; color: #c0cce0; line-height: 1.8;");
    layout->addWidget(instructionsLabel);

    layout->addStretch();

    return card;
  }

  QFrame* DashboardWidget::createDiagramCard() {
    QFrame *card = new QFrame();
    card->setProperty("class", "status-card");
    card->setMinimumSize(280, 280);
    card->setMaximumSize(320, 320);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignCenter);

    // Satellite/spacecraft icon placeholder
    QLabel *iconLabel = new QLabel();
    iconLabel->setFixedSize(220, 220);
    iconLabel->setStyleSheet(
      "QLabel {"
      "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
      "              stop:0 rgba(80, 60, 160, 0.15), stop:1 rgba(100, 80, 180, 0.1));"
      "  border: 2px solid rgba(120, 100, 220, 0.3);"
      "  border-radius: 110px;"
      "}"
    );
    iconLabel->setAlignment(Qt::AlignCenter);

    QLabel *innerIcon = new QLabel("🛰️", iconLabel);
    innerIcon->setGeometry(0, 0, 220, 220);
    innerIcon->setAlignment(Qt::AlignCenter);
    innerIcon->setStyleSheet("font-size: 80px; background: transparent; border: none;");

    layout->addWidget(iconLabel);

    return card;
  }

  QFrame* DashboardWidget::createImagesCard() {
    QFrame *card = new QFrame();
    card->setProperty("class", "status-card");
    card->setMinimumSize(200, 180);
    card->setMaximumHeight(220);

    card->setStyleSheet(
      "QFrame { "
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
      "              stop:0 rgba(30, 50, 60, 0.7), stop:1 rgba(25, 40, 48, 0.7)); "
      "  border: 1px solid rgba(60, 120, 140, 0.5); "
      "  border-radius: 18px; "
      "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(4);

    QLabel *titleLabel = new QLabel("IMAGES");
    titleLabel->setStyleSheet(
      "font-size: 11px; "
      "color: #8090b0; "
      "font-weight: 700; "
      "text-transform: uppercase; "
      "letter-spacing: 1px; "
      "background: transparent;"
    );
    layout->addWidget(titleLabel);
    layout->addSpacing(8);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(
      "QScrollArea { background: transparent; border: none; }"
      "QScrollBar:vertical { background: rgba(30, 30, 50, 0.5); width: 6px; border-radius: 3px; }"
      "QScrollBar::handle:vertical { background: rgba(100, 80, 180, 0.6); border-radius: 3px; }"
    );

    QWidget *contentWidget = new QWidget();
    contentWidget->setStyleSheet("background: transparent;");

    // Use vertical layout for simple list of filenames
    m_imagesContentLayout = new QVBoxLayout(contentWidget);
    m_imagesContentLayout->setContentsMargins(8, 8, 8, 8);
    m_imagesContentLayout->setSpacing(4);
    m_imagesContentLayout->setAlignment(Qt::AlignTop);

    QLabel *emptyLabel = new QLabel("No images imported");
    emptyLabel->setObjectName("emptyImagesLabel");
    emptyLabel->setStyleSheet(
      "color: #607080;"
      "font-size: 11px; "
      "font-style: italic; "
      "background: transparent; "
      "padding: 20px;"
    );
    emptyLabel->setAlignment(Qt::AlignCenter);
    m_imagesContentLayout->addWidget(emptyLabel);

    scrollArea->setWidget(contentWidget);
    layout->addWidget(scrollArea, 1);

    return card;
  }

  QFrame* DashboardWidget::createChartCard() {
    QFrame *card = new QFrame();
    card->setProperty("class", "chart-area");
    card->setMinimumHeight(200);
    card->setMaximumHeight(400);

    m_chartLayout = new QVBoxLayout(card);
    m_chartLayout->setContentsMargins(24, 24, 24, 24);

    // Header
    QLabel *headerLabel = new QLabel("OPERATION LOG");
    headerLabel->setStyleSheet("font-size: 11px; color: #7c8ba0; font-weight: 700; "
                              "text-transform: uppercase; letter-spacing: 2px;");
    m_chartLayout->addWidget(headerLabel);

    m_chartLayout->addSpacing(12);

    QLabel *placeholderLabel = new QLabel("Operation history will appear here as you work");
    placeholderLabel->setStyleSheet("color: #7c8ba0; font-size: 13px; padding: 40px;");
    placeholderLabel->setAlignment(Qt::AlignCenter);
    m_chartLayout->addWidget(placeholderLabel);

    return card;
  }

  void DashboardWidget::updateSystemStatus(const QString &status) {
    if (m_systemStatusLabel) {
      m_systemStatusLabel->setText(status);
    }
  }

  void DashboardWidget::updateImageCount(int count) {
    if (m_imageCountLabel) {
      m_imageCountLabel->setText(QString::number(count));
    }
  }

  void DashboardWidget::updateControlNetworkCount(int count) {
    m_controlNetworkCount = count;
  }

  QFrame* DashboardWidget::createControlNetworksCard() {
    QFrame *card = new QFrame();
    card->setProperty("class", "status-card");
    card->setMinimumSize(200, 180);
    card->setMaximumHeight(220);

    card->setStyleSheet(
      "QFrame { "
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
      "              stop:0 rgba(30, 50, 60, 0.7), stop:1 rgba(25, 40, 48, 0.7)); "
      "  border: 1px solid rgba(60, 120, 140, 0.5); "
      "  border-radius: 18px; "
      "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(4);

    // Title row with add button
    QHBoxLayout *titleLayout = new QHBoxLayout();

    QLabel *titleLabel = new QLabel("CONTROL NETWORKS");
    titleLabel->setStyleSheet(
      "font-size: 11px; "
      "color: #8090b0; "
      "font-weight: 700; "
      "text-transform: uppercase; "
      "letter-spacing: 1px; "
      "background: transparent;"
    );
    titleLayout->addWidget(titleLabel);

    titleLayout->addStretch();

    // Add button
    QPushButton *addButton = new QPushButton("+");
    addButton->setStyleSheet(
      "QPushButton { "
      "  background: rgba(96, 216, 220, 0.3); "
      "  color: #60d8dc; "
      "  border: 1px solid rgba(96, 216, 220, 0.5); "
      "  border-radius: 10px; "
      "  font-size: 16px; "
      "  font-weight: bold; "
      "  padding: 0px; "
      "  min-width: 20px; "
      "  max-width: 20px; "
      "  min-height: 20px; "
      "  max-height: 20px; "
      "}"
      "QPushButton:hover { "
      "  background: rgba(96, 216, 220, 0.5); "
      "  color: white; "
      "}"
    );
    addButton->setCursor(Qt::PointingHandCursor);
    addButton->setToolTip("Create a new control network");
    connect(addButton, &QPushButton::clicked, this, &DashboardWidget::createControlNetworkRequested);
    titleLayout->addWidget(addButton);

    layout->addLayout(titleLayout);
    layout->addSpacing(8);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(
      "QScrollArea { background: transparent; border: none; }"
      "QScrollBar:vertical { background: rgba(30, 30, 50, 0.5); width: 6px; border-radius: 3px; }"
      "QScrollBar::handle:vertical { background: rgba(100, 80, 180, 0.6); border-radius: 3px; }"
    );

    QWidget *contentWidget = new QWidget();
    contentWidget->setStyleSheet("background: transparent;");

    m_controlNetworksLayout = new QVBoxLayout(contentWidget);
    m_controlNetworksLayout->setContentsMargins(0, 0, 0, 0);
    m_controlNetworksLayout->setSpacing(6);

    QLabel *emptyLabel = new QLabel("No control networks");
    emptyLabel->setObjectName("emptyControlNetworksLabel");
    emptyLabel->setStyleSheet(
      "color: #607080;"
      "font-size: 11px; "
      "font-style: italic; "
      "background: transparent; "
      "padding: 20px;"
    );
    emptyLabel->setAlignment(Qt::AlignCenter);
    m_controlNetworksLayout->addWidget(emptyLabel);

    scrollArea->setWidget(contentWidget);
    layout->addWidget(scrollArea, 1);

    return card;
  }

  void DashboardWidget::addControlNetworkToCard(const QString &networkName, int numPoints, int numMeasures) {
    if (!m_controlNetworksLayout) return;

    try {
      // Remove empty label if it exists
      if (m_controlNetworksLayout->parentWidget()) {
        QWidget *emptyLabel = m_controlNetworksLayout->parentWidget()->findChild<QWidget*>("emptyControlNetworksLabel");
        if (emptyLabel) {
          delete emptyLabel;
        }
      }

      m_controlNetworkCount++;

      // Create a compact info card for the control network
      QFrame *cnetFrame = new QFrame();
      if (!cnetFrame) return;

      cnetFrame->setStyleSheet(
        "QFrame { "
        "  background: rgba(60, 80, 100, 0.4); "
        "  border: 1px solid rgba(80, 140, 160, 0.5); "
        "  border-radius: 8px; "
        "  padding: 8px; "
        "}"
      );

      QVBoxLayout *cnetLayout = new QVBoxLayout(cnetFrame);
      cnetLayout->setContentsMargins(8, 8, 8, 8);
      cnetLayout->setSpacing(4);

      // Network name
      QLabel *nameLabel = new QLabel(networkName);
      nameLabel->setStyleSheet(
        "color: #60d8dc; "
        "font-size: 13px; "
        "font-weight: 700; "
        "background: transparent;"
      );
      nameLabel->setWordWrap(true);
      cnetLayout->addWidget(nameLabel);

      // Stats
      QLabel *statsLabel = new QLabel(QString("%1 points • %2 measures").arg(numPoints).arg(numMeasures));
      statsLabel->setStyleSheet(
        "color: #8099aa; "
        "font-size: 10px; "
        "background: transparent;"
      );
      cnetLayout->addWidget(statsLabel);

      m_controlNetworksLayout->addWidget(cnetFrame);
    }
    catch (...) {
      // Silently fail to avoid crashing
      qDebug() << "Error adding control network card for:" << networkName;
    }
  }

  void DashboardWidget::setActiveView(QWidget *view) {
    if (!view || !m_viewContainer) return;

    clearActiveView();

    m_viewContainer->layout()->addWidget(view);
    m_viewContainer->show();
    m_activeViewWidget = view;
  }

  void DashboardWidget::clearActiveView() {
    if (m_activeViewWidget && m_viewContainer) {
      m_viewContainer->layout()->removeWidget(m_activeViewWidget);
      m_viewContainer->hide();
      m_activeViewWidget = nullptr;
    }
  }

  void DashboardWidget::addImageToCard(const QString &imageName, const QString &imagePath) {
    if (!m_imagesContentLayout) return;

    // Check if this image was already added (prevent duplicates)
    if (m_addedImages.contains(imagePath)) {
      return;
    }
    m_addedImages.insert(imagePath);
    m_imageCount++;

    // Remove empty label if it exists (only on first image)
    if (m_imageCount == 1) {
      QWidget *emptyLabel = m_imagesContentLayout->parentWidget()->findChild<QWidget*>("emptyImagesLabel");
      if (emptyLabel) {
        m_imagesContentLayout->removeWidget(emptyLabel);
        delete emptyLabel;
      }
    }

    // Extract just the filename from the full path
    QString fileName = QFileInfo(imagePath).fileName();

    // Create a clickable label for the image
    QLabel *imageLabel = new QLabel(fileName);
    imageLabel->setStyleSheet(
      "QLabel { "
      "  color: #60d8dc; "
      "  font-size: 11px; "
      "  background: transparent; "
      "  padding: 4px 8px; "
      "  border-radius: 4px; "
      "}"
      "QLabel:hover { "
      "  background: rgba(96, 216, 220, 0.2); "
      "  text-decoration: underline; "
      "}"
    );
    imageLabel->setCursor(Qt::PointingHandCursor);
    imageLabel->setToolTip(imagePath);  // Show full path on hover
    imageLabel->setProperty("imagePath", imagePath);

    // Make it clickable
    imageLabel->installEventFilter(this);

    // Store the label so we can handle clicks
    imageLabel->setProperty("isImageLabel", true);

    m_imagesContentLayout->addWidget(imageLabel);
  }

  QFrame* DashboardWidget::createTargetBodyCard() {
    QFrame *card = new QFrame();
    card->setProperty("class", "status-card");
    card->setMinimumSize(200, 180);
    card->setMaximumHeight(220);

    card->setStyleSheet(
      "QFrame { "
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
      "              stop:0 rgba(30, 50, 60, 0.7), stop:1 rgba(25, 40, 48, 0.7)); "
      "  border: 1px solid rgba(60, 120, 140, 0.5); "
      "  border-radius: 18px; "
      "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(4);

    QLabel *titleLabel = new QLabel("TARGET BODY");
    titleLabel->setStyleSheet(
      "font-size: 11px; "
      "color: #8090b0; "
      "font-weight: 700; "
      "text-transform: uppercase; "
      "letter-spacing: 1px; "
      "background: transparent;"
    );
    layout->addWidget(titleLabel);

    layout->addSpacing(8);

    // Content area
    m_targetBodyContentLayout = new QVBoxLayout();
    m_targetBodyContentLayout->setContentsMargins(0, 0, 0, 0);
    m_targetBodyContentLayout->setSpacing(4);

    QLabel *emptyLabel = new QLabel("No target body");
    emptyLabel->setStyleSheet(
      "color: #607080;"
      "font-size: 11px; "
      "font-style: italic; "
      "background: transparent; "
      "padding: 20px;"
    );
    emptyLabel->setAlignment(Qt::AlignCenter);
    m_targetBodyContentLayout->addWidget(emptyLabel);

    layout->addLayout(m_targetBodyContentLayout, 1);

    return card;
  }

  void DashboardWidget::setTargetBodyInfo(const QString &targetName, const QPixmap &targetImage,
                                          const QString &systemName, const QString &centerLon, const QString &centerLat) {
    if (!m_targetBodyContentLayout) return;

    QLayoutItem *item;
    while ((item = m_targetBodyContentLayout->takeAt(0)) != nullptr) {
      delete item->widget();
      delete item;
    }

    // Horizontal layout for image and info
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(16);

    // Target image with border
    if (!targetImage.isNull()) {
      QLabel *imageLabel = new QLabel();
      imageLabel->setPixmap(targetImage.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
      imageLabel->setStyleSheet(
        "border: 2px solid rgba(96, 216, 220, 0.4); "
        "border-radius: 50px; "
        "background: transparent;"
      );
      contentLayout->addWidget(imageLabel);
    }

    // Info section
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);

    QLabel *nameLabel = new QLabel(targetName);
    nameLabel->setStyleSheet(
      "color: #60d8dc; "
      "font-size: 20px; "
      "font-weight: 700; "
      "background: transparent;"
    );
    infoLayout->addWidget(nameLabel);

    // System name as subtitle
    if (!systemName.isEmpty()) {
      QLabel *systemLabel = new QLabel(systemName);
      systemLabel->setStyleSheet(
        "color: #8099aa; "
        "font-size: 13px; "
        "background: transparent;"
      );
      infoLayout->addWidget(systemLabel);
    }

    infoLayout->addSpacing(8);

    // Coordinates - show as separate lines for readability
    if (!centerLat.isEmpty()) {
      QLabel *latLabel = new QLabel("Lat: " + centerLat);
      latLabel->setStyleSheet(
        "color: #7080a0; "
        "font-size: 11px; "
        "background: transparent;"
      );
      infoLayout->addWidget(latLabel);
    }

    if (!centerLon.isEmpty()) {
      QLabel *lonLabel = new QLabel("Lon: " + centerLon);
      lonLabel->setStyleSheet(
        "color: #7080a0; "
        "font-size: 11px; "
        "background: transparent;"
      );
      infoLayout->addWidget(lonLabel);
    }

    infoLayout->addStretch();
    contentLayout->addLayout(infoLayout, 1);

    m_targetBodyContentLayout->addLayout(contentLayout);
    m_targetBodyContentLayout->addStretch();
  }

  QFrame* DashboardWidget::createSpacecraftCard() {
    QFrame *card = new QFrame();
    card->setProperty("class", "status-card");
    card->setMinimumSize(200, 180);
    card->setMaximumHeight(220);

    card->setStyleSheet(
      "QFrame { "
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
      "              stop:0 rgba(30, 50, 60, 0.7), stop:1 rgba(25, 40, 48, 0.7)); "
      "  border: 1px solid rgba(60, 120, 140, 0.5); "
      "  border-radius: 18px; "
      "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(4);

    QLabel *titleLabel = new QLabel("SPACECRAFT");
    titleLabel->setStyleSheet(
      "font-size: 11px; "
      "color: #8090b0; "
      "font-weight: 700; "
      "text-transform: uppercase; "
      "letter-spacing: 1px; "
      "background: transparent;"
    );
    layout->addWidget(titleLabel);

    layout->addSpacing(8);

    m_spacecraftContentLayout = new QVBoxLayout();
    m_spacecraftContentLayout->setContentsMargins(0, 0, 0, 0);
    m_spacecraftContentLayout->setSpacing(8);

    QLabel *emptyLabel = new QLabel("No spacecraft");
    emptyLabel->setStyleSheet(
      "color: #607080;"
      "font-size: 11px; "
      "font-style: italic; "
      "background: transparent; "
      "padding: 20px;"
    );
    emptyLabel->setAlignment(Qt::AlignCenter);
    m_spacecraftContentLayout->addWidget(emptyLabel);

    layout->addLayout(m_spacecraftContentLayout, 1);

    return card;
  }

  void DashboardWidget::setSpacecraftInfo(const QString &spacecraftName, const QString &instrumentName,
                                          const QString &startTime, const QString &exposureDuration, const QString &filter) {
    if (!m_spacecraftContentLayout) return;

    QLayoutItem *item;
    while ((item = m_spacecraftContentLayout->takeAt(0)) != nullptr) {
      delete item->widget();
      delete item;
    }

    // Create horizontal layout for icon and info
    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->setSpacing(16);

    // Add spacecraft icon/visual on the right
    QLabel *iconLabel = new QLabel();
    iconLabel->setFixedSize(100, 100);
    iconLabel->setStyleSheet(
      "QLabel {"
      "  background: qradialgradient(cx:0.5, cy:0.5, radius:0.5, "
      "              fx:0.5, fy:0.5, stop:0 rgba(255, 102, 68, 0.3), "
      "              stop:0.5 rgba(255, 102, 68, 0.1), stop:1 transparent); "
      "  border: 2px solid rgba(255, 102, 68, 0.4); "
      "  border-radius: 50px;"
      "}"
    );

    // Info section on the left
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);

    // Spacecraft name
    QLabel *nameLabel = new QLabel(spacecraftName);
    nameLabel->setStyleSheet(
      "color: #ff6644; "
      "font-size: 18px; "
      "font-weight: 700; "
      "background: transparent;"
    );
    nameLabel->setWordWrap(true);
    infoLayout->addWidget(nameLabel);

    // Instrument
    if (!instrumentName.isEmpty()) {
      QLabel *instrumentValue = new QLabel(instrumentName);
      instrumentValue->setStyleSheet(
        "color: #9099aa; "
        "font-size: 13px; "
        "background: transparent;"
      );
      infoLayout->addWidget(instrumentValue);
    }

    infoLayout->addSpacing(8);

    // Start Time
    if (!startTime.isEmpty()) {
      QString formattedTime = startTime;
      if (startTime.length() > 19) {
        formattedTime = startTime.left(19).replace("T", " ");
      }

      QLabel *timeValue = new QLabel(formattedTime);
      timeValue->setStyleSheet(
        "color: #7080a0; "
        "font-size: 11px; "
        "background: transparent;"
      );
      timeValue->setWordWrap(true);
      infoLayout->addWidget(timeValue);
    }

    // Compact details line
    QStringList details;
    if (!exposureDuration.isEmpty()) {
      details << "Exp: " + exposureDuration + "ms";
    }
    if (!filter.isEmpty()) {
      details << "Filter: " + filter;
    }

    if (!details.isEmpty()) {
      QLabel *detailsLabel = new QLabel(details.join(" • "));
      detailsLabel->setStyleSheet(
        "color: #7080a0; "
        "font-size: 11px; "
        "background: transparent;"
      );
      detailsLabel->setWordWrap(true);
      infoLayout->addWidget(detailsLabel);
    }

    infoLayout->addStretch();

    mainLayout->addLayout(infoLayout, 1);
    mainLayout->addWidget(iconLabel);

    m_spacecraftContentLayout->addLayout(mainLayout);
    m_spacecraftContentLayout->addStretch();
  }
}
