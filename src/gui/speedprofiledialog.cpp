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

#include "speedprofiledialog.h"

#include <QMessageBox>

#include "ui_speedprofiledialog.h"

SpeedProfileDialog::SpeedProfileDialog(QWidget *parent, const SpeedSchedule::SpeedProfile *profile)
    : QDialog(parent)
    , m_ui(new Ui::SpeedProfileDialog)
{
    m_ui->setupUi(this);

    if (profile)
    {
        // Editing existing profile
        m_ui->lineEditName->setText(profile->name);
        m_ui->spinDownloadLimit->setValue(profile->downloadLimit / 1024); // Convert from bytes/s to KiB/s
        m_ui->spinUploadLimit->setValue(profile->uploadLimit / 1024);
        m_originalName = profile->name;
        setWindowTitle(tr("Edit Speed Profile"));
    }
    else
    {
        // Creating new profile
        setWindowTitle(tr("Add Speed Profile"));
    }

    setupValidation();
}

SpeedProfileDialog::~SpeedProfileDialog()
{
    delete m_ui;
}

void SpeedProfileDialog::setupValidation()
{
    // Enable/disable OK button based on name field
    connect(m_ui->lineEditName, &QLineEdit::textChanged, this, [this](const QString &text)
    {
        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!text.trimmed().isEmpty());
    });

    // Initial state
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!m_ui->lineEditName->text().trimmed().isEmpty());
}

SpeedSchedule::SpeedProfile SpeedProfileDialog::getProfile() const
{
    SpeedSchedule::SpeedProfile profile;
    profile.name = m_ui->lineEditName->text().trimmed();
    profile.downloadLimit = m_ui->spinDownloadLimit->value() * 1024; // Convert from KiB/s to bytes/s
    profile.uploadLimit = m_ui->spinUploadLimit->value() * 1024;
    return profile;
}

void SpeedProfileDialog::accept()
{
    const QString name = m_ui->lineEditName->text().trimmed();

    if (name.isEmpty())
    {
        QMessageBox::warning(this, tr("Invalid Input"), tr("Profile name cannot be empty."));
        return;
    }

    QDialog::accept();
}
