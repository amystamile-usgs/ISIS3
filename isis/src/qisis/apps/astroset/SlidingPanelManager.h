#ifndef SlidingPanelManager_H
#define SlidingPanelManager_H

#include <QWidget>
#include <QMap>
#include <QString>

class QVBoxLayout;
class QPushButton;
class QPropertyAnimation;
class QFrame;

namespace Isis {

  /**
   * @brief Manages sliding panels that overlay the dashboard
   *
   * Handles multiple tool panels (QView, QNet, etc.) that slide in from the right
   * with vertical tabs for switching between them.
   */
  class SlidingPanelManager : public QWidget {
    Q_OBJECT

  public:
    SlidingPanelManager(QWidget *parent = 0);
    ~SlidingPanelManager();

    void addPanel(const QString &id, QWidget *panel, const QString &tabLabel);
    void showPanel(const QString &id);
    void hidePanel();  // Hide current panel but keep tab
    void closePanel(const QString &id);  // Remove panel and tab completely
    void closeAllPanels();
    bool hasOpenPanels() const;
    bool hasPanel(const QString &id) const;
    QString currentPanelId() const;

  signals:
    void allPanelsClosed();
    void panelOpened(const QString &id);
    void panelClosed(const QString &id);

  private:
    void createTabBar();
    void updateTabBar();
    void slideIn();
    void slideOut();

    struct PanelInfo {
      QWidget *widget;
      QString tabLabel;
      QPushButton *tabButton;
    };

    QFrame *m_panelContainer;          // Container for the active panel
    QFrame *m_tabBar;                   // Vertical tab bar on the right
    QVBoxLayout *m_tabBarLayout;
    QMap<QString, PanelInfo> m_panels;
    QString m_currentPanelId;
    QPropertyAnimation *m_slideAnimation;
  };
}

#endif
