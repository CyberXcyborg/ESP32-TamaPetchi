#ifndef WEBSOCKET_H
#define WEBSOCKET_H

// ============================================================
// WebSocket Configuration
// ============================================================
#define WS_PORT        81
#define WS_MAX_CLIENTS  3

#include <Arduino.h>

// ============================================================
// WebSocket Module (Phase 10.2)
// Replaces SSE with WebSocket for lower-latency real-time updates.
// Uses the Links2004/WebSockets library.
// ============================================================

// Initialize WebSocket server on the given port
void webSocketBegin(uint16_t port);

// Handle WebSocket events — call in every loop iteration
void webSocketLoop();

// Broadcast a JSON message to all connected WebSocket clients
void webSocketBroadcast(const String &data);

// Get the number of connected WebSocket clients
int webSocketConnectedClients();

// Get the WebSocket port
uint16_t webSocketPort();

// Periodic broadcast — call from loop() instead of handleSSEClients()
void handleWebSocketBroadcast();

// Notification broadcast — call when events happen (feed, play, reset, etc.)
void webSocketBroadcastNotification(const String &notificationType, const String &message);

#endif // WEBSOCKET_H
