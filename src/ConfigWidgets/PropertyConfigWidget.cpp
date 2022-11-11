// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ConfigWidgets/PropertyConfigWidget.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>
#include <QWidget>
#include <string_view>
#include <variant>

namespace orbit_config_widgets {

static QString QStringFromStringView(std::string_view sv) {
  return QString::fromUtf8(sv.data(), sv.size());
}

// Makes an object name that kind of resembles a C identifier.
// The output is guaranteed to only contain lower case letters and underscores.
static QString MakeObjectName(std::string_view input) {
  auto output = QStringFromStringView(input).trimmed().toLower();

  for (auto& c : output) {
    if (!c.isLetter()) c = '_';
  }

  return output;
}

PropertyConfigWidget::PropertyConfigWidget(QWidget* parent) : QWidget(parent) {
  layout_ = new QGridLayout(this);  // NOLINT(cppcoreguidelines-owning-memory)
}

void PropertyConfigWidget::AddWidgetForProperty(FloatProperty* property) {
  int row = layout_->rowCount();

  auto* name_label = new QLabel(QStringFromStringView(property->definition().label), this);
  layout_->addWidget(name_label, row, 0, Qt::AlignRight);
  name_label->setObjectName(
      QString{"name_label_%1"}.arg(MakeObjectName(property->definition().label)));

  auto* slider = new QSlider(Qt::Horizontal, this);  // NOLINT(cppcoreguidelines-owning-memory)
  property->value_ = property->definition().initial_value;
  slider->setMinimum(0);
  slider->setMaximum(static_cast<int>((property->definition().max - property->definition().min) /
                                      property->definition().step));
  slider->setValue(static_cast<int>((property->value_ - property->definition().min) /
                                    property->definition().step));
  slider->setSingleStep(1);
  slider->setObjectName(QString{"slider_%1"}.arg(MakeObjectName(property->definition().label)));
  layout_->addWidget(slider, row, 1);

  const auto get_value_string = [property](float val) {
    return QString{"%1%2"}
        .arg(val, 0, 'f', 2)
        .arg(QStringFromStringView(property->definition().suffix));
  };

  auto* value_label = new QLabel(get_value_string(property->definition_.initial_value), this);
  layout_->addWidget(value_label, row, 2, Qt::AlignRight);
  value_label->setObjectName(
      QString{"value_label_%1"}.arg(MakeObjectName(property->definition_.label)));

  QObject::connect(slider, &QSlider::valueChanged, value_label,
                   [property, get_value_string, value_label](int value) {
                     const auto new_value = property->definition().min +
                                            static_cast<float>(value) * property->definition_.step;
                     if (property->value_ == new_value) return;
                     property->value_ = new_value;
                     value_label->setText(get_value_string(property->value_));
                   });

  property->setter_ = [slider](int value) { slider->setValue(value); };
}

void PropertyConfigWidget::AddWidgetForProperty(IntProperty* property) {
  int row = layout_->rowCount();

  auto* name_label = new QLabel(
      QString::fromUtf8(property->definition_.label.data(), property->definition_.label.size()),
      this);
  layout_->addWidget(name_label, row, 0, Qt::AlignRight);
  name_label->setObjectName(
      QString{"name_label_%1"}.arg(MakeObjectName(property->definition_.label)));

  auto* slider = new QSlider(Qt::Horizontal, this);  // NOLINT(cppcoreguidelines-owning-memory)
  property->value_ = property->definition_.initial_value;
  slider->setMinimum(property->definition().min);
  slider->setMaximum(property->definition().max);
  slider->setValue(property->definition_.initial_value);
  slider->setSingleStep(property->definition_.step);
  slider->setObjectName(QString{"slider_%1"}.arg(MakeObjectName(property->definition_.label)));
  layout_->addWidget(slider, row, 1);

  const auto get_value_string = [property](int val) {
    return QString{"%1%2"}.arg(val).arg(QString::fromUtf8(property->definition_.suffix.data(),
                                                          property->definition_.suffix.size()));
  };

  auto* value_label = new QLabel(get_value_string(property->definition_.initial_value), this);
  layout_->addWidget(value_label, row, 2, Qt::AlignRight);
  value_label->setObjectName(
      QString{"value_label_%1"}.arg(MakeObjectName(property->definition_.label)));

  QObject::connect(slider, &QSlider::valueChanged, value_label,
                   [property, get_value_string, value_label](int value) {
                     if (value == property->value_) return;
                     property->value_ = value;
                     value_label->setText(get_value_string(property->value_));
                   });

  property->setter_ = [slider](int value) { slider->setValue(value); };
}

void PropertyConfigWidget::AddWidgetForProperty(BoolProperty* property) {
  int row = layout_->rowCount();

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  auto* checkbox = new QCheckBox(QStringFromStringView(property->definition().label), this);
  checkbox->setObjectName(QString{"checkbox_%1"}.arg(MakeObjectName(property->definition().label)));
  property->value_ = property->definition().initial_value;
  checkbox->setChecked(property->value());
  layout_->addWidget(checkbox, row, 1);

  QObject::connect(checkbox, &QCheckBox::stateChanged, this, [property](int check_state) {
    bool new_value = check_state == Qt::Checked;
    if (property->value() == new_value) return;
    property->value_ = new_value;
  });

  property->setter_ = [checkbox](bool checked) { checkbox->setChecked(checked); };
}

}  // namespace orbit_config_widgets
