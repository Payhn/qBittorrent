# DHT (Distributed Hash Table) Implementation

## Overview

DHT (Distributed Hash Table) is a decentralized peer discovery mechanism that allows BitTorrent clients to find peers without relying on centralized trackers. qBittorrent implements DHT through the libtorrent library, providing a robust trackerless peer discovery system.

### What is DHT?

DHT is based on the Kademlia protocol and works by:
1. **Distributing peer information** across a network of nodes
2. **Storing info-hash → peer mappings** in a distributed manner
3. **Enabling peer discovery** without requiring tracker servers
4. **Providing redundancy** when trackers are unavailable or blocked

Each DHT node maintains a routing table of other nodes and can look up peers for any torrent by its info-hash.

## Architecture

### Component Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                         qBittorrent DHT Architecture                 │
├─────────────────────────────────────────────────────────────────────┤
│                                                                       │
│  ┌──────────────┐    ┌──────────────────┐    ┌──────────────────┐   │
│  │   GUI Layer  │    │   Session Layer   │    │  libtorrent      │   │
│  │              │    │                   │    │  DHT Engine      │   │
│  │ - StatusBar  │◄──►│ - SessionImpl    │◄──►│                  │   │
│  │ - Options    │    │ - SessionStatus  │    │ - Kademlia DHT   │   │
│  │ - TrackerList│    │ - TorrentImpl    │    │ - Routing Table  │   │
│  └──────────────┘    └──────────────────┘    │ - Bootstrap      │   │
│                                               └──────────────────┘   │
│                                                                       │
└─────────────────────────────────────────────────────────────────────┘
```

### Key Files

| Component | File | Purpose |
|-----------|------|---------|
| Session Management | `src/base/bittorrent/sessionimpl.cpp` | DHT enable/disable, bootstrap configuration |
| Session Interface | `src/base/bittorrent/session.h` | Public DHT API |
| Session Status | `src/base/bittorrent/sessionstatus.h` | DHT statistics structure |
| Torrent Control | `src/base/bittorrent/torrentimpl.cpp` | Per-torrent DHT control |
| Peer Info | `src/base/bittorrent/peerinfo.cpp` | Identify DHT-sourced peers |
| Status Bar | `src/gui/statusbar.cpp` | DHT node count display |
| Options Dialog | `src/gui/optionsdialog.cpp` | DHT enable/disable UI |
| Advanced Settings | `src/gui/advancedsettings.cpp` | Bootstrap nodes configuration |
| Tracker List | `src/gui/trackerlist/trackerlistmodel.cpp` | DHT status in tracker list |

## Implementation Details

### Default Bootstrap Nodes

qBittorrent uses three default DHT bootstrap nodes to join the network:

```cpp
// src/base/bittorrent/sessionimpl.cpp:129
const QString DEFAULT_DHT_BOOTSTRAP_NODES =
    u"dht.libtorrent.org:25401, dht.transmissionbt.com:6881, router.bittorrent.com:6881"_s;
```

These nodes are well-known entry points to the DHT network maintained by:
- **dht.libtorrent.org:25401** - Libtorrent project
- **dht.transmissionbt.com:6881** - Transmission project
- **router.bittorrent.com:6881** - BitTorrent Inc.

### Session-Level Configuration

#### Enable/Disable DHT

```cpp
// src/base/bittorrent/sessionimpl.cpp:768-780
bool SessionImpl::isDHTEnabled() const
{
    return m_isDHTEnabled;
}

void SessionImpl::setDHTEnabled(bool enabled)
{
    if (enabled != m_isDHTEnabled)
    {
        m_isDHTEnabled = enabled;
        configureDeferred();
        LogMsg(tr("Distributed Hash Table (DHT) support: %1")
            .arg(enabled ? tr("ON") : tr("OFF")), Log::INFO);
    }
}
```

#### Bootstrap Nodes Management

```cpp
// src/base/bittorrent/sessionimpl.cpp:753-766
QString SessionImpl::getDHTBootstrapNodes() const
{
    const QString nodes = m_DHTBootstrapNodes;
    return !nodes.isEmpty() ? nodes : DEFAULT_DHT_BOOTSTRAP_NODES;
}

void SessionImpl::setDHTBootstrapNodes(const QString &nodes)
{
    if (nodes == m_DHTBootstrapNodes)
        return;

    m_DHTBootstrapNodes = nodes;
    configureDeferred();
}
```

### Libtorrent Integration

DHT is configured through libtorrent's settings pack:

```cpp
// src/base/bittorrent/sessionimpl.cpp:2129-2130
settingsPack.set_str(lt::settings_pack::dht_bootstrap_nodes,
                     getDHTBootstrapNodes().toStdString());
settingsPack.set_bool(lt::settings_pack::enable_dht, isDHTEnabled());

// Additional DHT settings
pack.set_bool(lt::settings_pack::use_dht_as_fallback, false);  // Line 1712
settingsPack.set_int(lt::settings_pack::active_dht_limit, -1); // Line 2051 (unlimited)
```

Key libtorrent settings:
- `enable_dht`: Master switch for DHT functionality
- `dht_bootstrap_nodes`: Comma-separated list of `host:port` entries
- `use_dht_as_fallback`: When `false`, DHT runs alongside trackers (not just as fallback)
- `active_dht_limit`: Set to `-1` for unlimited DHT announcements

### Per-Torrent DHT Control

Individual torrents can have DHT disabled:

```cpp
// src/base/bittorrent/torrentimpl.cpp:1475-1478
bool TorrentImpl::isDHTDisabled() const
{
    return static_cast<bool>(m_nativeStatus.flags & lt::torrent_flags::disable_dht);
}

// src/base/bittorrent/torrentimpl.cpp:2688-2699
void TorrentImpl::setDHTDisabled(const bool disable)
{
    if (disable == isDHTDisabled())
        return;

    if (disable)
        m_nativeHandle.set_flags(lt::torrent_flags::disable_dht);
    else
        m_nativeHandle.unset_flags(lt::torrent_flags::disable_dht);

    deferredRequestResumeData();
}

// Force immediate DHT announce
// src/base/bittorrent/torrentimpl.cpp:1628-1631
void TorrentImpl::forceDHTAnnounce()
{
    m_nativeHandle.force_dht_announce();
}
```

### Private Torrents

Private torrents automatically disable DHT per BitTorrent specification:

```cpp
// src/gui/trackerlist/trackerlistmodel.cpp:76-85
QString statusDHT(const BitTorrent::Torrent *torrent)
{
    if (!torrent->session()->isDHTEnabled())
        return TrackerListModel::tr("Disabled");

    if (torrent->isPrivate() || torrent->isDHTDisabled())
        return TrackerListModel::tr("Disabled for this torrent");

    return TrackerListModel::tr("Working");
}
```

## Statistics and Monitoring

### SessionStatus Structure

```cpp
// src/base/bittorrent/sessionstatus.h:35-76
struct SessionStatus
{
    // DHT-specific metrics
    qint64 dhtUploadRate = 0;      // Current DHT upload rate (bytes/sec)
    qint64 dhtDownloadRate = 0;    // Current DHT download rate (bytes/sec)
    qint64 dhtUpload = 0;          // Total DHT bytes uploaded
    qint64 dhtDownload = 0;        // Total DHT bytes downloaded
    qint64 dhtNodes = 0;           // Number of DHT nodes in routing table
    // ... other fields
};
```

### Metrics Collection

DHT statistics are collected from libtorrent's internal metrics:

```cpp
// src/base/bittorrent/sessionimpl.h:115-120
struct MetricIndices
{
    struct {
        int dhtBytesIn = -1;   // Metric index for incoming DHT bytes
        int dhtBytesOut = -1;  // Metric index for outgoing DHT bytes
        int dhtNodes = -1;     // Metric index for DHT node count
    } dht;
};

// src/base/bittorrent/sessionimpl.cpp:1825-1829
.dht =
{
    .dhtBytesIn = findMetricIndex("dht.dht_bytes_in"),
    .dhtBytesOut = findMetricIndex("dht.dht_bytes_out"),
    .dhtNodes = findMetricIndex("dht.dht_nodes")
}
```

### Rate Calculation

```cpp
// src/base/bittorrent/sessionimpl.cpp:6222-6253
const int64_t dhtDownload = stats[m_metricIndices.dht.dhtBytesIn];
const int64_t dhtUpload = stats[m_metricIndices.dht.dhtBytesOut];

const auto calcRate = [interval](const qint64 previous, const qint64 current) -> qint64
{
    return (((current - previous) * lt::microseconds(1s).count()) / interval);
};

m_status.dhtDownloadRate = calcRate(m_status.dhtDownload, dhtDownload);
m_status.dhtUploadRate = calcRate(m_status.dhtUpload, dhtUpload);
m_status.dhtDownload = dhtDownload;
m_status.dhtUpload = dhtUpload;
m_status.dhtNodes = stats[m_metricIndices.dht.dhtNodes];
```

## User Interface

### Status Bar

The status bar displays the current DHT node count:

```cpp
// src/gui/statusbar.cpp:111-113
m_DHTLbl = new QLabel(tr("DHT: %1 nodes").arg(0), this);
m_DHTLbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
m_DHTSeparator = createSeparator(m_DHTLbl);

// src/gui/statusbar.cpp:223-236
void StatusBar::updateDHTNodesNumber()
{
    if (BitTorrent::Session::instance()->isDHTEnabled())
    {
        m_DHTLbl->setVisible(true);
        m_DHTSeparator->setVisible(true);
        m_DHTLbl->setText(tr("DHT: %1 nodes")
            .arg(BitTorrent::Session::instance()->status().dhtNodes));
    }
    else
    {
        m_DHTLbl->setVisible(false);
        m_DHTSeparator->setVisible(false);
    }
}
```

### Preferences Dialog

Located in **Preferences > BitTorrent**:
- Checkbox: "Enable DHT (decentralized network) to find more peers"
- Tooltip: "Find peers on the DHT network"

### Advanced Settings

Located in **Preferences > Advanced**:
- **DHT bootstrap nodes**: Text field for custom bootstrap nodes
- Placeholder: "Resets to default if empty"
- Links to libtorrent documentation

### Per-Torrent Options

Located in **Torrent Properties > Options**:
- Checkbox: "Disable DHT for this torrent"
- Automatically disabled for private torrents with tooltip: "Not applicable to private torrents"

### Tracker List

DHT appears in the tracker list as `** [DHT] **` showing:
- Status (Working/Disabled/Disabled for this torrent)
- Peer count from DHT source
- Seed count from DHT source

## Peer Identification

Peers discovered through DHT are marked with flag **'H'**:

```cpp
// src/base/bittorrent/peerinfo.cpp:48-51
bool PeerInfo::fromDHT() const
{
    return static_cast<bool>(m_nativeInfo.source & lt::peer_info::dht);
}

// src/base/bittorrent/peerinfo.cpp:346-348
if (fromDHT())
    updateFlags(u'H', tr("Peer from DHT"));
```

## WebUI API

### Get Settings

```
GET /api/v2/app/preferences
```

Response includes:
```json
{
    "dht": true,
    "dht_bootstrap_nodes": "dht.libtorrent.org:25401, ..."
}
```

### Set Settings

```
POST /api/v2/app/setPreferences
Content-Type: application/x-www-form-urlencoded

json={"dht": true, "dht_bootstrap_nodes": "..."}
```

## Configuration Storage

| Setting | Storage Key | Default |
|---------|-------------|---------|
| DHT Enabled | `Preferences/Bittorrent/DHT` | `true` |
| Bootstrap Nodes | `Preferences/Bittorrent/DHTBootstrapNodes` | (empty, uses default) |

Legacy key `BitTorrent/Session/DHTEnabled` is migrated during upgrades.

## Data Flow

```
┌──────────────────────────────────────────────────────────────────┐
│                        DHT Data Flow                              │
└──────────────────────────────────────────────────────────────────┘

1. BOOTSTRAP PROCESS
   User starts qBittorrent
          │
          ▼
   SessionImpl::loadSettings()
          │
          ▼
   Apply lt::settings_pack with:
   - enable_dht = true
   - dht_bootstrap_nodes = "dht.libtorrent.org:25401, ..."
          │
          ▼
   libtorrent contacts bootstrap nodes
          │
          ▼
   DHT routing table populated
          │
          ▼
   Status bar shows "DHT: N nodes"

2. PEER DISCOVERY
   Torrent added
          │
          ▼
   libtorrent announces to DHT (if not private & DHT enabled)
          │
          ▼
   DHT network returns peers
          │
          ▼
   Peers marked with 'H' flag (fromDHT)
          │
          ▼
   Tracker list shows DHT peer count

3. STATISTICS UPDATE CYCLE
   Timer triggers (every second)
          │
          ▼
   Read libtorrent metrics:
   - dht.dht_bytes_in
   - dht.dht_bytes_out
   - dht.dht_nodes
          │
          ▼
   Calculate rates and update SessionStatus
          │
          ▼
   UI components read SessionStatus
          │
          ▼
   Status bar, statistics dialog updated
```

## Troubleshooting

### DHT Shows 0 Nodes

1. Check if DHT is enabled in Preferences > BitTorrent
2. Verify firewall allows UDP traffic on listening port
3. Check bootstrap nodes are reachable
4. Wait a few minutes for the routing table to populate

### No Peers Found via DHT

1. Verify the torrent is not marked as private
2. Check if DHT is disabled for the specific torrent
3. Ensure session-level DHT is enabled
4. Check if the torrent has been announced to DHT

### High DHT Traffic

DHT traffic is typically minimal. If excessive:
1. Check `dhtUploadRate` and `dhtDownloadRate` in status
2. Consider if many torrents are being added simultaneously
3. DHT traffic increases with number of active torrents

## Related Systems

- **PeX (Peer Exchange)**: Complements DHT by exchanging peer lists with connected peers
- **LSD (Local Service Discovery)**: Discovers peers on local network
- **Trackers**: Traditional centralized peer discovery

## References

- [libtorrent DHT Documentation](https://www.libtorrent.org/dht_extensions.html)
- [libtorrent Settings Reference](https://www.libtorrent.org/reference-Settings.html#dht_bootstrap_nodes)
- [BEP 5: DHT Protocol](http://www.bittorrent.org/beps/bep_0005.html)
- [Kademlia Paper](https://pdos.csail.mit.edu/~petar/papers/maymounkov-kademlia-lncs.pdf)
