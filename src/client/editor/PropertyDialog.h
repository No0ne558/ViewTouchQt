// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/PropertyDialog.h — Dialog for editing UI element properties.

#ifndef VT_PROPERTY_DIALOG_H
#define VT_PROPERTY_DIALOG_H

#include "../layout/UiElement.h"

#include <QCheckBox>
#include <QDialog>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QColor>
#include <QLabel>

namespace vt {

/// A modal dialog that allows editing the visual properties of a UiElement.
class PropertyDialog : public QDialog {
    Q_OBJECT
public:
    explicit PropertyDialog(UiElement *element, QWidget *parent = nullptr);

    /// Provide a list of page names for the Navigation target combo.
    void setPageNames(const QStringList &names);

    /// Whether the user changed the element type.
    bool typeChanged() const { return m_originalType != m_newType; }

    /// The element type selected by the user (may differ from original).
    ElementType newType() const { return m_newType; }

    /// Image-specific getters
    QString imagePathValue() const;
    bool imageOnlyValue() const;

    // ── Type-specific property getters (only Button supported now)
    int     actionTypeValue() const;
    bool    inheritable() const;

private slots:
    void chooseColor(QPushButton *btn, QColor &target);
    void applyChanges();
    void onTypeChanged(int index);

private:
    void setupUi();
    void setButtonColor(QPushButton *btn, const QColor &c);

    UiElement *m_element;
    ElementType m_originalType;
    ElementType m_newType;

    QLineEdit *m_idEdit       = nullptr;
    QLineEdit *m_labelEdit    = nullptr;
    QGroupBox *m_textGroup    = nullptr;
    QSpinBox  *m_fontSizeBox  = nullptr;
    QComboBox *m_fontFamilyCombo = nullptr;
    QCheckBox *m_fontBoldChk  = nullptr;
    QSpinBox  *m_xBox         = nullptr;
    QSpinBox  *m_yBox         = nullptr;
    QSpinBox  *m_wBox         = nullptr;
    QSpinBox  *m_hBox         = nullptr;
    QComboBox *m_edgeStyleCombo = nullptr;
    QComboBox *m_shadowCombo = nullptr;

    QPushButton *m_bgColorBtn   = nullptr;
    QPushButton *m_textColorBtn = nullptr;
    QCheckBox   *m_bgTransChk   = nullptr;

    QColor m_bgColor;
    QColor m_textColor;

    // Type selector
    QComboBox *m_typeCombo = nullptr;

    // Type-specific widgets (no longer used in simplified editor)
    // (action-type UI removed)

    // Image property
    QComboBox *m_imageCombo = nullptr;
    QLabel *m_imagePreview = nullptr;
    QCheckBox *m_imageOnlyChk = nullptr;

    // Common flag
    QCheckBox *m_inheritableChk  = nullptr;
    // Button behaviour selector
    QComboBox *m_behaviorCombo = nullptr;
    QSpinBox  *m_layerBox = nullptr;
};

} // namespace vt

#endif // VT_PROPERTY_DIALOG_H
