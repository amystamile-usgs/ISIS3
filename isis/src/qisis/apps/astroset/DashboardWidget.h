#ifndef DashboardWidget_H
#define DashboardWidget_H

#include <QWidget>
#include <QGridLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>

namespace Isis {

  /**
   * @brief Mission control style dashboard for Astroset
   *
   * Creates a futuristic dashboard with status cards, technical diagrams,
   * and system metrics inspired by aerospace mission control interfaces.
   */
  class DashboardWidget : public QWidget {
    Q_OBJECT

  public:
    DashboardWidget(QWidget *parent = 0);
    ~DashboardWidget();

    void updateSystemStatus(const QString &status);
    void updateImageCount(int count);
    void updateControlNetworkCount(int count);

    // Add/remove active view as a card
    void setActiveView(QWidget *view);
    void clearActiveView();

    // Populate cards with actual data
    void addImageToCard(const QString &imageName, const QString &imagePath);
    void setTargetBodyInfo(const QString &targetName, const QPixmap &targetImage,
                           const QString &systemName, const QString &centerLon, const QString &centerLat);
    void setSpacecraftInfo(const QString &spacecraftName, const QString &instrumentName,
                           const QString &startTime, const QString &exposureDuration, const QString &filter);

  signals:
    void imageClicked(const QString &imagePath);

  private:
    void setupUI();
    QFrame* createStatusCard(const QString &title, const QString &value,
                             const QString &label, const QString &colorClass);
    QFrame* createWelcomeCard();
    QFrame* createDiagramCard();
    QFrame* createChartCard();
    QFrame* createImagesCard();
    QFrame* createTargetBodyCard();
    QFrame* createSpacecraftCard();

    QGridLayout *m_mainLayout;
    QLabel *m_systemStatusLabel;
    QLabel *m_imageCountLabel;
    QLabel *m_networkCountLabel;
    QWidget *m_activeViewWidget;
    QFrame *m_viewContainer;
    QFrame *m_chartCard;
    QVBoxLayout *m_chartLayout;

    QFrame *m_imagesCard;
    QVBoxLayout *m_imagesContentLayout;
    QGridLayout *m_imagesGridLayout;
    QSet<QString> m_addedImages;
    int m_imageCount;
    QFrame *m_targetBodyCard;
    QVBoxLayout *m_targetBodyContentLayout;
    QFrame *m_spacecraftCard;
    QVBoxLayout *m_spacecraftContentLayout;
  };
}

#endif
