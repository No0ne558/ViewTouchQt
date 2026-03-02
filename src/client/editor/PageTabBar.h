// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/PageTabBar.h — Bottom tab bar showing all pages for quick switching.

#ifndef VT_PAGE_TAB_BAR_H
#define VT_PAGE_TAB_BAR_H

#include "../layout/LayoutEngine.h"

#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QPushButton>
#include <QWidget>
#include <QHBoxLayout>
#include <QList>

namespace vt {

/// A horizontal bar rendered at the bottom of the 1920×1080 design scene
/// showing one tab per page.  Clicking a tab switches to that page.
class PageTabBar : public QObject {
    Q_OBJECT
public:
    explicit PageTabBar(LayoutEngine *engine, QGraphicsScene *scene,
                        QObject *parent = nullptr);
    ~PageTabBar() override;

    /// Rebuild the tab buttons from the current engine page list.
    void refresh();

    /// Show / hide the tab bar in the scene.
    void setVisible(bool visible);
    bool isVisible() const { return m_proxy != nullptr; }

signals:
    /// Emitted when user clicks a page tab.
    void pageSelected(const QString &pageName);

private:
    void buildWidget();
    void destroyWidget();

    LayoutEngine   *m_engine = nullptr;
    QGraphicsScene *m_scene  = nullptr;

    QWidget              *m_bar   = nullptr;
    QGraphicsProxyWidget *m_proxy = nullptr;
    QList<QPushButton *>  m_tabs;
};

} // namespace vt

#endif // VT_PAGE_TAB_BAR_H
