# TamaPetchi Companion (React Native)

A mobile companion app for the ESP32 TamaPetchi project, enabling remote pet monitoring and control from iOS and Android devices.

## Features

- **Device Discovery**: Automatic mDNS scan for TamaPetchi devices on local network
- **Real-time Stats**: Live pet status via WebSocket connection
- **Pet Sprite Display**: Animated pet visualization matching the device display
- **Quick Actions**: Feed, Play, Clean, Sleep buttons (POST to device API)
- **Notifications**: Background fetch for pet alerts (every 15 min)
- **Settings**: Device IP configuration, notification preferences, theme

## Project Structure

```
TamaPetchiCompanion/
├── src/
│   ├── screens/
│   │   ├── HomeScreen.tsx        # Main pet status display
│   │   ├── ActionsScreen.tsx     # Feed/play/clean/sleep buttons
│   │   ├── StatsScreen.tsx       # Detailed stats and analytics
│   │   └── SettingsScreen.tsx    # Device IP, notifications, theme
│   ├── services/
│   │   ├── DiscoveryService.ts   # mDNS device discovery
│   │   ├── ApiClient.ts          # HTTP API client for device
│   │   ├── WebSocketClient.ts    # WebSocket real-time updates
│   │   └── NotificationService.ts # Push notification handler
│   ├── components/
│   │   ├── PetSprite.tsx         # Animated pet component
│   │   ├── StatBar.tsx           # Stat bar with animation
│   │   └── ActionButton.tsx      # Action button with icon
│   └── types/
│       └── index.ts              # TypeScript type definitions
├── App.tsx                       # Main app entry
├── package.json
└── README.md
```

## API Integration

The companion app communicates with the TamaPetchi device via:

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/pet` | GET | Get current pet status |
| `/feed` | POST | Feed the pet |
| `/play` | POST | Play with the pet |
| `/clean` | POST | Clean the pet |
| `/sleep` | POST | Put pet to sleep |
| `/api/voice/status` | GET | Get voice-friendly status |
| `/api/analytics/predictions` | GET | Get health predictions |
| WebSocket `:81` | WS | Real-time stat updates |

## Setup

```bash
npx react-native init TamaPetchiCompanion
cd TamaPetchiCompanion
npm install @react-navigation/native @react-navigation/bottom-tabs
npm install react-native-zeroconf  # For mDNS discovery
npm install @react-native-async-storage/async-storage
```

## Building

```bash
# iOS
cd ios && pod install && cd ..
npx react-native run-ios

# Android
npx react-native run-android
```

## Device Discovery

The app uses mDNS/Bonjour to discover TamaPetchi devices advertising `_http._tcp` on the local network. The device hostname follows the pattern `tamapetchi-XXXX.local`.

## License

MIT License — Part of the ESP32-TamaPetchi project
