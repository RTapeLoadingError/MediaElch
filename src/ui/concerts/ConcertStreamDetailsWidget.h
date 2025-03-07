#pragma once

#include "concerts/ConcertController.h"

#include <QLineEdit>
#include <QPointer>
#include <QVector>
#include <QWidget>
#include <chrono>
#include <memory>

namespace Ui {
class ConcertStreamDetailsWidget;
}

class ConcertStreamDetailsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConcertStreamDetailsWidget(QWidget* parent = nullptr);
    ~ConcertStreamDetailsWidget() override;

    void updateConcert(ConcertController* controller);
    /// \brief Clear the widget and remove all references to current concert.
    void clear();

signals:
    void streamDetailsChanged();
    void runtimeChanged(std::chrono::minutes);

private slots:
    void onReloadStreamDetails();

    void onStreamDetailsEdited();
    void updateStreamDetails(bool reloadFromFile = false);

private:
    std::unique_ptr<Ui::ConcertStreamDetailsWidget> ui;
    QPointer<ConcertController> m_concertController = nullptr;
    QVector<QWidget*> m_streamDetailsWidgets;
    QVector<QVector<QLineEdit*>> m_streamDetailsAudio;
    QVector<QVector<QLineEdit*>> m_streamDetailsSubtitles;
};
