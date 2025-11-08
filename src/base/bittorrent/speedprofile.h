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

#pragma once

#include <QString>
#include <QTime>

#include "base/preferences.h"

namespace SpeedSchedule
{
    /**
     * @brief Represents a named speed limit profile
     *
     * A profile defines specific upload/download speed limits that can be
     * applied during scheduled time periods. Multiple profiles can be created
     * and referenced by schedule entries.
     */
    struct SpeedProfile
    {
        QString name;           // Unique identifier for the profile (e.g., "Night", "Peak Hours", "Normal")
        int downloadLimit;      // Download speed limit in bytes/second (-1 for unlimited)
        int uploadLimit;        // Upload speed limit in bytes/second (-1 for unlimited)

        // TODO: Add equality operators for comparison
        // TODO: Add serialization methods (toJson/fromJson) for Preferences storage
        // TODO: Add validation methods (e.g., isValid(), validateLimits())
    };

    /**
     * @brief Represents a scheduled time period for applying a speed profile
     *
     * A schedule entry defines when a specific speed profile should be active.
     * Multiple entries can exist, and the scheduler will evaluate them to
     * determine which profile to apply at any given time.
     */
    struct ScheduleEntry
    {
        QTime startTime;        // Start time of the schedule (e.g., 04:00)
        QTime endTime;          // End time of the schedule (e.g., 06:00)
        Scheduler::Days days;   // Days when this schedule is active
        QString profileName;    // Name of the SpeedProfile to apply during this time

        // TODO: Add equality operators for comparison
        // TODO: Add serialization methods (toJson/fromJson) for Preferences storage
        // TODO: Add validation methods (e.g., isValid(), doesOverlap())
        // TODO: Add method to check if a given QDateTime falls within this schedule
    };

    // TODO: Add helper functions for schedule conflict detection
    // TODO: Add helper functions for finding the active profile at a given time
    // TODO: Consider adding priority field to ScheduleEntry for conflict resolution
}
