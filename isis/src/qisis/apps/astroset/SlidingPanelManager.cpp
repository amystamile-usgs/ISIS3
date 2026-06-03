#include "SlidingPanelManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QFrame>
#include <QLabel>

namespace Isis {

  SlidingPanelManager::SlidingPanelManager(QWidget *parent) : QWidget(parent) {
    // Main layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Panel container (holds the active panel)
    m_panelContainer = new QFrame();
    m_panelContainer->setStyleSheet(
      "QFrame { "
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
      "              stop:0 #0a0814, stop:1 #0d0a1a); "
      "  border: none; "
      "}"
    );
    QVBoxLayout *containerLayout = new QVBoxLayout(m_panelContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);

    // Add "Back to Dashboard" button at top
    QPushButton *backButton = new QPushButton("🏠 Dashboard");
    backButton->setFixedHeight(40);
    backButton->setStyleSheet(
      "QPushButton { "
      "  background: rgba(30, 25, 45, 0.8); "
      "  border: none; "
      "  border-bottom: 1px solid rgba(100, 80, 180, 0.3); "
      "  color: #8090b0; "
      "  font-size: 13px; "
      "  font-weight: 600; "
      "  text-align: left; "
      "  padding-left: 15px; "
      "}"
      "QPushButton:hover { "
      "  background: rgba(60, 50, 120, 0.6); "
      "  color: #00ff88; "
      "}"
    );
    connect(backButton, &QPushButton::clicked, this, &SlidingPanelManager::hidePanel);
    containerLayout->addWidget(backButton);

    // Tab bar (vertical tabs on right edge)
    createTabBar();

    mainLayout->addWidget(m_panelContainer);
    mainLayout->addWidget(m_tabBar);

    // Initially hidden
    hide();

    m_slideAnimation = nullptr;
  }

  SlidingPanelManager::~SlidingPanelManager() {
  }

  void SlidingPanelManager::createTabBar() {
    m_tabBar = new QFrame();
    m_tabBar->setFixedWidth(40);
    m_tabBar->setStyleSheet(
      "QFrame { "
      "  background: rgba(15, 10, 25, 0.95); "
      "  border-left: 1px solid rgba(100, 80, 180, 0.3); "
      "}"
    );

    m_tabBarLayout = new QVBoxLayout(m_tabBar);
    m_tabBarLayout->setContentsMargins(0, 10, 0, 10);
    m_tabBarLayout->setSpacing(8);
    m_tabBarLayout->addStretch();
  }

  void SlidingPanelManager::addPanel(const QString &id, QWidget *panel, const QString &tabLabel) {
    // Create tab container with button and close
    QWidget *tabContainer = new QWidget();
    tabContainer->setFixedSize(36, 70);

    QVBoxLayout *tabLayout = new QVBoxLayout(tabContainer);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->setSpacing(2);

    // Tab button
    QPushButton *tabButton = new QPushButton(tabLabel.left(1)); // First letter only
    tabButton->setFixedSize(36, 50);
    tabButton->setToolTip(tabLabel);
    tabButton->setStyleSheet(
      "QPushButton { "
      "  background: rgba(30, 25, 45, 0.8); "
      "  border: 1px solid rgba(100, 80, 180, 0.3); "
      "  border-radius: 6px; "
      "  color: #8090b0; "
      "  font-size: 14px; "
      "  font-weight: 700; "
      "}"
      "QPushButton:hover { "
      "  background: rgba(60, 50, 120, 0.6); "
      "  border: 1px solid rgba(0, 255, 136, 0.5); "
      "  color: #00ff88; "
      "}"
      "QPushButton:checked { "
      "  background: rgba(0, 255, 136, 0.2); "
      "  border: 1px solid #00ff88; "
      "  color: #00ff88; "
      "}"
    );
    tabButton->setCheckable(true);

    // Close button
    QPushButton *closeButton = new QPushButton("×");
    closeButton->setFixedSize(36, 18);
    closeButton->setToolTip("Close " + tabLabel);
    closeButton->setStyleSheet(
      "QPushButton { "
      "  background: transparent; "
      "  border: none; "
      "  color: #8090b0; "
      "  font-size: 16px; "
      "}"
      "QPushButton:hover { "
      "  background: rgba(255, 100, 100, 0.3); "
      "  color: #ff6666; "
      "}"
    );

    tabLayout->addWidget(tabButton);
    tabLayout->addWidget(closeButton);

    // Store panel info
    PanelInfo info;
    info.widget = panel;
    info.tabLabel = tabLabel;
    info.tabButton = tabButton;
    m_panels[id] = info;

    // Add tab container
    m_tabBarLayout->insertWidget(m_tabBarLayout->count() - 1, tabContainer);

    // Connect click to show panel
    connect(tabButton, &QPushButton::clicked, this, [this, id]() {
      showPanel(id);
    });

    // Connect close button
    connect(closeButton, &QPushButton::clicked, this, [this, id]() {
      closePanel(id);
    });

    // Hide panel initially
    panel->hide();
  }

  void SlidingPanelManager::showPanel(const QString &id) {
    if (!m_panels.contains(id)) return;

    // If same panel, do nothing
    if (m_currentPanelId == id && isVisible()) return;

    // Hide current panel
    if (!m_currentPanelId.isEmpty() && m_panels.contains(m_currentPanelId)) {
      m_panels[m_currentPanelId].widget->hide();
      m_panels[m_currentPanelId].tabButton->setChecked(false);
    }

    // Remove current panel from layout
    QLayoutItem *item;
    while ((item = m_panelContainer->layout()->takeAt(0)) != nullptr) {
      // Don't delete the widget, just remove from layout
    }

    // Show new panel
    m_currentPanelId = id;
    PanelInfo &info = m_panels[id];
    info.tabButton->setChecked(true);
    m_panelContainer->layout()->addWidget(info.widget);

    // Show the widget and ensure it gets properly sized
    info.widget->show();
    info.widget->raise();
    info.widget->activateWindow();

    // Show the panel container and the manager
    m_panelContainer->show();
    if (!isVisible()) {
      show();
      emit panelOpened(id);
    }
  }

  void SlidingPanelManager::hidePanel() {
    // Hide current panel but keep tabs
    if (!m_currentPanelId.isEmpty() && m_panels.contains(m_currentPanelId)) {
      m_panels[m_currentPanelId].widget->hide();
      m_panels[m_currentPanelId].tabButton->setChecked(false);
    }

    m_currentPanelId.clear();

    // Hide the panel container
    m_panelContainer->hide();

    // Go back to dashboard but keep tab bar loaded (it will be hidden when stack switches)
    emit allPanelsClosed();
  }

  void SlidingPanelManager::closePanel(const QString &id) {
    if (!m_panels.contains(id)) return;

    PanelInfo &info = m_panels[id];

    // Remove the tab button from the layout and delete it
    if (info.tabButton) {
      QWidget *tabContainer = info.tabButton->parentWidget();
      if (tabContainer) {
        m_tabBarLayout->removeWidget(tabContainer);
        tabContainer->deleteLater();
      }
    }

    // Remove the panel widget from layout if it's there
    if (m_panelContainer->layout()) {
      m_panelContainer->layout()->removeWidget(info.widget);
    }

    // Remove from panels map
    m_panels.remove(id);

    // If this was the current panel, hide the manager or switch to another
    if (m_currentPanelId == id) {
      m_currentPanelId.clear();

      // Check if there are other panels with tabs still available
      if (!m_panels.isEmpty()) {
        // Show the first available panel
        showPanel(m_panels.begin().key());
      } else {
        // No more panels/tabs, go back to dashboard
        m_panelContainer->hide();
        emit allPanelsClosed();
      }
    }

    emit panelClosed(id);
  }

  void SlidingPanelManager::closeAllPanels() {
    // Close all panels
    for (auto it = m_panels.begin(); it != m_panels.end(); ++it) {
      it.value().widget->hide();
      it.value().tabButton->setChecked(false);
    }

    m_currentPanelId.clear();
    hide();
    emit allPanelsClosed();
  }

  bool SlidingPanelManager::hasOpenPanels() const {
    return isVisible();
  }

  bool SlidingPanelManager::hasPanel(const QString &id) const {
    return m_panels.contains(id);
  }

  QString SlidingPanelManager::currentPanelId() const {
    return m_currentPanelId;
  }

  void SlidingPanelManager::updateTabBar() {
    // Update tab button states
    for (auto it = m_panels.begin(); it != m_panels.end(); ++it) {
      it.value().tabButton->setChecked(it.key() == m_currentPanelId);
    }
  }
}
