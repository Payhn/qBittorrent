/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2017  Vladimir Golovnev <glassez@yandex.ru>
 * Copyright (C) 2010  Christophe Dumez <chris@qbittorrent.org>
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

#include "bandwidthscheduler.h"

#include <chrono>
#include <utility>

#include <QDate>
#include <QTime>

#include "base/preferences.h"

using namespace std::chrono_literals;

BandwidthScheduler::BandwidthScheduler(QObject *parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &BandwidthScheduler::onTimeout);
}

void BandwidthScheduler::start()
{
    m_lastAlternative = isTimeForAlternative();
    emit bandwidthLimitRequested(m_lastAlternative);

    // Timeout regularly to accommodate for external system clock changes
    // eg from the user or from a timesync utility
    m_timer.start(30s);
}

bool BandwidthScheduler::isTimeForAlternative() const
{
    const Preferences *const pref = Preferences::instance();

    QTime start = pref->getSchedulerStartTime();
    QTime end = pref->getSchedulerEndTime();
    const QTime now = QTime::currentTime();
    const Scheduler::Days schedulerDays = pref->getSchedulerDays();
    const int day = QDate::currentDate().dayOfWeek();
    bool alternative = false;

    if (start > end)
    {
        std::swap(start, end);
        alternative = true;
    }

    if ((start <= now) && (end >= now))
    {
        switch (schedulerDays)
        {
        case Scheduler::Days::EveryDay:
            alternative = !alternative;
            break;
        case Scheduler::Days::Monday:
        case Scheduler::Days::Tuesday:
        case Scheduler::Days::Wednesday:
        case Scheduler::Days::Thursday:
        case Scheduler::Days::Friday:
        case Scheduler::Days::Saturday:
        case Scheduler::Days::Sunday:
            {
                const int offset = static_cast<int>(Scheduler::Days::Monday) - 1;
                const int dayOfWeek = static_cast<int>(schedulerDays) - offset;
                if (day == dayOfWeek)
                    alternative = !alternative;
            }
            break;
        case Scheduler::Days::Weekday:
            if ((day >= 1) && (day <= 5))
                alternative = !alternative;
            break;
        case Scheduler::Days::Weekend:
            if ((day == 6) || (day == 7))
                alternative = !alternative;
            break;
        default:
            Q_UNREACHABLE();
            break;
        }
    }

    return alternative;
}

void BandwidthScheduler::onTimeout()
{
    const bool alternative = isTimeForAlternative();

    if (alternative != m_lastAlternative)
    {
        m_lastAlternative = alternative;
        emit bandwidthLimitRequested(alternative);
    }

    // TODO: Multi-profile implementation
    // Once Preferences::getSpeedProfiles() and getScheduleEntries() are implemented,
    // replace the above with:
    //
    // const QString currentProfile = getCurrentSpeedProfile();
    // if (currentProfile != m_lastActiveProfile)
    // {
    //     m_lastActiveProfile = currentProfile;
    //     emit speedProfileRequested(currentProfile);
    // }
}

// TODO: New method for multi-profile system
//
// QString BandwidthScheduler::getCurrentSpeedProfile() const
// {
//     const Preferences *const pref = Preferences::instance();
//     const QList<SpeedSchedule::ScheduleEntry> schedules = pref->getScheduleEntries();
//     const QTime now = QTime::currentTime();
//     const int day = QDate::currentDate().dayOfWeek();
//
//     // Implementation logic:
//     // 1. Iterate through all schedule entries
//     // 2. For each entry, check if current day matches entry.days
//     // 3. Check if current time falls within entry.startTime to entry.endTime
//     //    - Handle wrap-around (e.g., 23:00 to 01:00)
//     // 4. If match found, return entry.profileName
//     // 5. If multiple matches (overlapping schedules), use first match or priority system
//     // 6. If no match, return default profile from pref->getDefaultSpeedProfile()
//     //
//     // Example implementation:
//     // for (const SpeedSchedule::ScheduleEntry &entry : schedules)
//     // {
//     //     if (!isDayMatch(entry.days, day))
//     //         continue;
//     //
//     //     bool inTimeRange = false;
//     //     if (entry.startTime <= entry.endTime)
//     //     {
//     //         // Normal range (e.g., 06:00 to 16:00)
//     //         inTimeRange = (now >= entry.startTime && now <= entry.endTime);
//     //     }
//     //     else
//     //     {
//     //         // Wrap-around range (e.g., 23:00 to 01:00)
//     //         inTimeRange = (now >= entry.startTime || now <= entry.endTime);
//     //     }
//     //
//     //     if (inTimeRange)
//     //         return entry.profileName;
//     // }
//     //
//     // return pref->getDefaultSpeedProfile();
// }
//
// Helper method to check if current day matches schedule days setting:
// bool BandwidthScheduler::isDayMatch(Scheduler::Days schedulerDays, int currentDay) const
// {
//     switch (schedulerDays)
//     {
//     case Scheduler::Days::EveryDay:
//         return true;
//     case Scheduler::Days::Weekday:
//         return (currentDay >= 1 && currentDay <= 5);
//     case Scheduler::Days::Weekend:
//         return (currentDay == 6 || currentDay == 7);
//     case Scheduler::Days::Monday:
//     case Scheduler::Days::Tuesday:
//     case Scheduler::Days::Wednesday:
//     case Scheduler::Days::Thursday:
//     case Scheduler::Days::Friday:
//     case Scheduler::Days::Saturday:
//     case Scheduler::Days::Sunday:
//         {
//             const int offset = static_cast<int>(Scheduler::Days::Monday) - 1;
//             const int dayOfWeek = static_cast<int>(schedulerDays) - offset;
//             return currentDay == dayOfWeek;
//         }
//     default:
//         return false;
//     }
// }
