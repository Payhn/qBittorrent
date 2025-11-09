/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2025  qBittorrent project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link this program with the OpenSSL project's "OpenSSL" library (or with
 * modified versions of it that use the same license as the "OpenSSL" library),
 * and distribute the linked executables. You must obey the GNU General Public
 * License in all respects for all of the code used other than "OpenSSL".  If you
 * modify file(s), you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete this
 * exception statement from your version.
 */

#include "scheduleentrydialog.h"

#include <QMessageBox>

#include "base/preferences.h"
#include "ui_scheduleentrydialog.h"

ScheduleEntryDialog::ScheduleEntryDialog(QWidget *parent, const QStringList &availableProfiles,
                                          const SpeedSchedule::ScheduleEntry *entry)
    : QDialog(parent)
    , m_ui(new Ui::ScheduleEntryDialog)
{
    m_ui->setupUi(this);

    // Populate profile combo box
    m_ui->comboBoxProfile->addItems(availableProfiles);

    if (entry)
    {
        // Editing existing entry
        m_ui->timeEditStart->setTime(entry->startTime);
        m_ui->timeEditEnd->setTime(entry->endTime);
        m_ui->comboBoxDays->setCurrentIndex(static_cast<int>(entry->days));

        const int profileIndex = m_ui->comboBoxProfile->findText(entry->profileName);
        if (profileIndex >= 0)
            m_ui->comboBoxProfile->setCurrentIndex(profileIndex);

        setWindowTitle(tr("Edit Schedule Entry"));
    }
    else
    {
        // Creating new entry
        m_ui->comboBoxDays->setCurrentIndex(static_cast<int>(Scheduler::Days::EveryDay));
        setWindowTitle(tr("Add Schedule Entry"));
    }

    setupValidation();
}

ScheduleEntryDialog::~ScheduleEntryDialog()
{
    delete m_ui;
}

void ScheduleEntryDialog::setupValidation()
{
    // Enable/disable OK button based on profile selection
    connect(m_ui->comboBoxProfile, &QComboBox::currentIndexChanged, this, [this](int index)
    {
        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(index >= 0);
    });

    // Initial state
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_ui->comboBoxProfile->count() > 0);
}

SpeedSchedule::ScheduleEntry ScheduleEntryDialog::getEntry() const
{
    SpeedSchedule::ScheduleEntry entry;
    entry.startTime = m_ui->timeEditStart->time();
    entry.endTime = m_ui->timeEditEnd->time();
    entry.days = static_cast<Scheduler::Days>(m_ui->comboBoxDays->currentIndex());
    entry.profileName = m_ui->comboBoxProfile->currentText();
    return entry;
}

void ScheduleEntryDialog::accept()
{
    if (m_ui->comboBoxProfile->currentIndex() < 0)
    {
        QMessageBox::warning(this, tr("Invalid Input"), tr("Please select a speed profile."));
        return;
    }

    QDialog::accept();
}
