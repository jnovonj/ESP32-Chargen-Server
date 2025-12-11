#include <Arduino.h>

#include <WiFi.h>
#include <AsyncTCP.h>

// --- Configuration ---
constexpr size_t MAX_CLIENTS = 5;
constexpr size_t CHARGEN_PORT = 19;
constexpr size_t LINE_LENGTH = 72;

// Replace with your WiFi credentials
const char *Hostname = "ChargenServer";
const char *ssid = "YourSSID";
const char *password = "YourPassword";

// Full pattern of printable ASCII characters (95 characters)
const char CHARGEN_PATTERN_FULL[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
const size_t PATTERN_LENGTH_FULL = 95;

// Main asynchronous server object
AsyncServer *AsyncServerChargen = nullptr;

// Structure for the client state and static array
struct ClientState {
  AsyncClient *client = nullptr;           // Pointer to the AsyncClient object. nullptr means free slot.
  size_t startIndex = 0;                      // Tracks the index within the 95-character string where the next 72-character line to be sent should start.
};
ClientState AsyncClientChargen[MAX_CLIENTS];     // Static array to manage connections

// 1. Finds the state associated with a given client.
ClientState *findClientState(AsyncClient* client) {
  for (size_t i = 0; i < MAX_CLIENTS; i++) {
    if (AsyncClientChargen[i].client == client) {
      return &AsyncClientChargen[i];
    }
  }
  return nullptr; // Not found
}

// 2. Function that constructs and sends a 72-character line with rotation
void makeAndSendLine(AsyncClient *client) {
  // Connection status check before sending
  if (!client || client->disconnected()) return;

  ClientState *state = findClientState(client);
  if (!state) return; 

  int currentStartIndex = state->startIndex;

  // Buffer for the line (72 characters + \r\n)
  char lineBuffer[LINE_LENGTH + 2];

  // 1. Construct the 72-character line using the rotating pattern
  for (size_t i = 0; i < LINE_LENGTH; i++) {
    lineBuffer[i] = CHARGEN_PATTERN_FULL[(currentStartIndex + i) % PATTERN_LENGTH_FULL];
  }

  // 2. Advance the starting index for the next line (rotation)
  // The RFC dictates rotation after each line, which is why it is incremented here.
  state->startIndex = (currentStartIndex + 1) % PATTERN_LENGTH_FULL;

  // 3. Add the standard CHARGEN line terminator (\r\n)
  lineBuffer[LINE_LENGTH] = '\r';
  lineBuffer[LINE_LENGTH + 1] = '\n';

  // 4. Write data to the socket. write() is safer for a single packet.
  size_t bytes_to_send = LINE_LENGTH + 2;
  // Only write if there is enough space in the buffer.
  // This double check is a good defensive programming practice.
  if (client->canSend() && client->space() >= bytes_to_send) {
    client->write(lineBuffer, bytes_to_send);
  } else {
    // If there is no space, the next call to handleClientAck or onPoll will try again.
  }
}

// --- Callback Functions ---

// 1. Handles the Acknowledgement (ACK) event
void handleClientonAck(void *arg, AsyncClient *client, size_t len, uint32_t time) {
  // Main engine: If there is space in the buffer (and not disconnected), send the next line.
  if (!client->disconnected() && client->space() >= (LINE_LENGTH + 2)) {
    makeAndSendLine(client);
  }
}

// Called periodically while the client is connected.
void handleClientonPoll(void* arg, AsyncClient* client) {
  // We can reuse the same logic as the ACK handler.
  // Just try to send more data if there's space.
  handleClientonAck(arg, client, 0, 0);
}

void handleClientonError(void* arg, AsyncClient* client, int error) {
  // Log the error with the client's IP for better diagnostics.
  Serial.printf("Client error from %s! Code: %d, Message: %s\n",
                client->remoteIP().toString().c_str(),
                error,
                client->errorToString(error));
  // We don't need to clean up here. Closing the client will trigger the
  // onDisconnect callback, which is the single, correct place for cleanup.
  client->close();
}

// 2. Handles the disconnection event
void handleClientonDisconnect(void *a, AsyncClient *client) {
  Serial.println("Client disconnected.");

  // Search for the state in the array and free the slot
  for (size_t i = 0; i < MAX_CLIENTS; i++) {
    if (AsyncClientChargen[i].client == client) {
      AsyncClientChargen[i].client = nullptr;     // Mark as free slot
      AsyncClientChargen[i].startIndex = 0;       // Optional: reset the index
      break;
    }
  }
  delete client; // ⚠️ Important: free the memory of the AsyncClient object
}

// 3. Handles the new client connection event
void handleNewClient(void *arg, AsyncClient *client) {
  // Search for the first free slot in our static array.
  size_t freeIndex = MAX_CLIENTS;
  for (size_t i = 0; i < MAX_CLIENTS; i++) {
    if (AsyncClientChargen[i].client == nullptr) {
      freeIndex = i;
      break;
    }
  }

  if (freeIndex == MAX_CLIENTS) {
    // No free slot, reject the connection
    Serial.println("Connection rejected. Maximum clients reached.");
    client->close();
    delete client;
    return;
  }

  Serial.printf("Client connected from %s (Slot %d)\n", client->remoteIP().toString().c_str(), freeIndex);

  // Store the new client in the free slot
  AsyncClientChargen[freeIndex].client = client;
  AsyncClientChargen[freeIndex].startIndex = 0;        // Start the rotation

  // Called when previously sent data is acknowledged by the client. 
  // This is the core engine for continuous data transmission (Chargen).
  client->onAck(handleClientonAck, nullptr);

  // Called periodically by the AsyncTCP task. 
  // Serves as a backup to resume transmission if the buffer was full and the ACK wasn't received.
  client->onPoll(handleClientonPoll, nullptr);

  // Called when a communication error (e.g., protocol failure or timeout) occurs.
  // Essential for cleaning up the global client pointer and preventing resource leaks.
  client->onError(handleClientonError, nullptr);

  // Called when the client actively closes the connection or if a fatal error occurs.
  // Responsible for resetting the global client pointer and freeing memory.
  client->onDisconnect(handleClientonDisconnect, nullptr);

  // Initiate CHARGEN transmission with the first line
  makeAndSendLine(client);
}

void setup() {
  Serial.begin(115200);
  // 1. WiFi Configuration
  
  // Setting the Hostname before starting the connection is important
  // so the device is identified with a clear name on the network (e.g., ChargenServer.local).
  WiFi.setHostname(Hostname);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  // Loop waiting for the connection to be established
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP of the ESP32: ");
  Serial.println(WiFi.localIP());

  // 2. Initialize and start the asynchronous server
  AsyncServerChargen = new AsyncServer(CHARGEN_PORT);
  AsyncServerChargen->onClient(&handleNewClient, nullptr);
  AsyncServerChargen->begin();
  Serial.printf("CHARGEN server started on port %d\n", CHARGEN_PORT);
}


// Global variable to control the printing frequency
unsigned long lastPrintTime = 0;
const long interval = 5000; // Print every 5000 milliseconds (5 seconds)

void loop() {
  unsigned long currentMillis = millis();

  // Check if 5 seconds have passed since the last print
  if (currentMillis - lastPrintTime >= interval) {
    lastPrintTime = currentMillis;

    Serial.println("--- Chargen Connection Status (Port 19) ---");
    size_t activeClients = 0;

    // Iterate over the client state array
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
      if (AsyncClientChargen[i].client != nullptr) {
        // An active client was found in this slot
        activeClients++;
        AsyncClient *client = AsyncClientChargen[i].client;

        // Show client details
        Serial.printf ("  Slot %d: ACTIVE. IP: %s, Rotation Index: %d\n",
                      i, 
                      client->remoteIP().toString().c_str(),
                      AsyncClientChargen[i].startIndex);
      } else {
        // The slot is free
        Serial.printf("  Slot %d: Free\n", i);
      }
    }
    Serial.printf("Total Active Clients: %d/%d\n", activeClients, MAX_CLIENTS);
    Serial.println("-------------------------------------------------");
  }
}