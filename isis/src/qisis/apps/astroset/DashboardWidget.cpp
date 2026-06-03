#include "DashboardWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QPixmap>

namespace Isis {

  DashboardWidget::DashboardWidget(QWidget *parent) : QWidget(parent) {
    m_activeViewWidget = nullptr;
    m_viewContainer = nullptr;
    setupUI();
  }

  DashboardWidget::~DashboardWidget() {
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

    m_targetBodyCard = createTargetBodyCard();
    m_mainLayout->addWidget(m_targetBodyCard, 3, 0);

    QStringList cardTitles;
    cardTitles << "Shapes" << "Registrations" << "Sensors" << "Results";

    int col = 1;
    int row = 2;
    for (int i = 0; i < cardTitles.size(); i++) {
      QFrame *card = createStatusCard(cardTitles[i], "0", "Items", "status-card");
      m_mainLayout->addWidget(card, row, col);

      col++;
      if (col > 2) {
        col = 0;
        row++;
      }
    }

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
      "              stop:0 rgba(20, 20, 40, 0.6), stop:1 rgba(15, 15, 30, 0.6)); "
      "  border: 1px solid rgba(100, 80, 180, 0.4); "
      "  border-radius: 18px; "
      "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(4);

    QLabel *titleLabel = new QLabel(title);
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

    // Content area - this is where we'll add images/info/lists
    QWidget *contentArea = new QWidget();
    contentArea->setStyleSheet("background: transparent;");
    QVBoxLayout *contentLayout = new QVBoxLayout(contentArea);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(4);

    QLabel *emptyLabel = new QLabel("No " + title.toLower());
    emptyLabel->setStyleSheet(
      "color: #505870; "
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
      "              stop:0 rgba(20, 20, 40, 0.6), stop:1 rgba(15, 15, 30, 0.6)); "
      "  border: 1px solid rgba(100, 80, 180, 0.4); "
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
    m_imagesContentLayout = new QVBoxLayout(contentWidget);
    m_imagesContentLayout->setContentsMargins(0, 0, 0, 0);
    m_imagesContentLayout->setSpacing(4);

    QLabel *emptyLabel = new QLabel("No images imported");
    emptyLabel->setStyleSheet(
      "color: #505870; "
      "font-size: 11px; "
      "font-style: italic; "
      "background: transparent; "
      "padding: 20px;"
    );
    emptyLabel->setAlignment(Qt::AlignCenter);
    m_imagesContentLayout->addWidget(emptyLabel);

    m_imagesContentLayout->addStretch();
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
    if (m_networkCountLabel) {
      m_networkCountLabel->setText(QString::number(count));
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

    if (m_imagesContentLayout->count() > 0) {
      QLayoutItem *item = m_imagesContentLayout->itemAt(0);
      if (item && item->widget()) {
        QLabel *label = qobject_cast<QLabel*>(item->widget());
        if (label && label->text().contains("No images")) {
          delete label;
        }
      }
    }

    QPushButton *imageButton = new QPushButton(imageName);
    imageButton->setStyleSheet(
      "QPushButton { "
      "  background: rgba(30, 30, 50, 0.4); "
      "  color: #c0cce0; "
      "  border: 1px solid rgba(100, 80, 180, 0.3); "
      "  border-radius: 6px; "
      "  padding: 8px; "
      "  text-align: left; "
      "  font-size: 10px; "
      "}"
      "QPushButton:hover { "
      "  background: rgba(60, 50, 120, 0.5); "
      "  border: 1px solid rgba(0, 255, 136, 0.5); "
      "  color: #ffffff; "
      "}"
    );
    imageButton->setProperty("imagePath", imagePath);

    connect(imageButton, &QPushButton::clicked, this, [this, imagePath]() {
      emit imageClicked(imagePath);
    });

    m_imagesContentLayout->insertWidget(m_imagesContentLayout->count() - 1, imageButton);
  }

  QFrame* DashboardWidget::createTargetBodyCard() {
    QFrame *card = new QFrame();
    card->setProperty("class", "status-card");
    card->setMinimumSize(200, 180);
    card->setMaximumHeight(220);

    card->setStyleSheet(
      "QFrame { "
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
      "              stop:0 rgba(20, 20, 40, 0.6), stop:1 rgba(15, 15, 30, 0.6)); "
      "  border: 1px solid rgba(100, 80, 180, 0.4); "
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
      "color: #505870; "
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

  void DashboardWidget::setTargetBodyInfo(const QString &targetName, const QPixmap &targetImage) {
    if (!m_targetBodyContentLayout) return;

    QLayoutItem *item;
    while ((item = m_targetBodyContentLayout->takeAt(0)) != nullptr) {
      delete item->widget();
      delete item;
    }

    QLabel *nameLabel = new QLabel(targetName);
    nameLabel->setStyleSheet(
      "color: #00ff88; "
      "font-size: 14px; "
      "font-weight: 700; "
      "background: transparent;"
    );
    m_targetBodyContentLayout->addWidget(nameLabel);

    m_targetBodyContentLayout->addSpacing(8);

    if (!targetImage.isNull()) {
      QLabel *imageLabel = new QLabel();
      imageLabel->setPixmap(targetImage.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
      imageLabel->setAlignment(Qt::AlignCenter);
      imageLabel->setStyleSheet("background: transparent;");
      m_targetBodyContentLayout->addWidget(imageLabel);
    }

    m_targetBodyContentLayout->addStretch();
  }

  void DashboardWidget::setSpacecraftInfo(const QString &spacecraftName, const QString &instrumentName) {
    // TODO: Implement when we do Spacecraft card
  }
}
