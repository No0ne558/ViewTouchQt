// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/PropertyDialog.h — Dialog for editing UI element properties.

#ifndef VT_PROPERTY_DIALOG_H
#define VT_PROPERTY_DIALOG_H

#include "../layout/UiElement.h"

#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QColor>

namespace vt {

/// A modal dialog that allows editing the visual properties of a UiElement.
class PropertyDialog : public QDialog {
    Q_OBJECT
public:
    explicit PropertyDialog(UiElement *element, QWidget *parent = nullptr);

private slots:
    void chooseColor(QPushButton *btn, QColor &target);
    void applyChanges();

private:
    void setupUi();
    void setButtonColor(QPushButton *btn, const QColor &c);

    UiElement *m_element;

    QLineEdit *m_idEdit       = nullptr;
    QLineEdit *m_labelEdit    = nullptr;
    QSpinBox  *m_fontSizeBox  = nullptr;
    QSpinBox  *m_xBox         = nullptr;
    QSpinBox  *m_yBox         = nullptr;
    QSpinBox  *m_wBox         = nullptr;
    QSpinBox  *m_hBox         = nullptr;
    QSpinBox  *m_radiusBox    = nullptr;

    QPushButton *m_bgColorBtn   = nullptr;
    QPushButton *m_textColorBtn = nullptr;

    QColor m_bgColor;
    QColor m_textColor;

    // Type-specific widgets
    QLineEdit *m_keyValueEdit    = nullptr;   // KeypadButton
    QComboBox *m_actionTypeCombo = nullptr;   // ActionButton
    QCheckBox *m_maskedCheck     = nullptr;   // PinEntry
    QSpinBox  *m_maxLengthBox    = nullptr;   // PinEntry
};

} // namespace vt

#endif // VT_PROPERTY_DIALOG_H
