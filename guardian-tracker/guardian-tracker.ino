
/**
 * SYNTAX:
 *
 * Firestore::Documents::createDocument(<AsyncClient>, <Firestore::Parent>, <documentPath>, <DocumentMask>, <Document>, <AsyncResultCallback>, <uid>);
 *
 * Firestore::Documents::createDocument(<AsyncClient>, <Firestore::Parent>, <collectionId>, <documentId>, <DocumentMask>, <Document>, <AsyncResultCallback>, <uid>);
 *
 * <AsyncClient> - The async client.
 * <Firestore::Parent> - The Firestore::Parent object included project Id and database Id in its constructor.
 * <documentPath> - The relative path of document to create in the collection.
 * <DocumentMask> - The fields to return. If not set, returns all fields. Use comma (,) to separate between the field names.
 * <collectionId> - The relative path of document collection id to create the document.
 * <documentId> - The document id of document to be created.
 * <Document> - The Firestore document.
 * <AsyncResultCallback> - The async result callback (AsyncResultCallback).
 * <uid> - The user specified UID of async result (optional).
 *
 * The Firebase project Id should be only the name without the firebaseio.com.
 * The Firestore database id should be (default) or empty "".
 *
 * The complete usage guidelines, please visit https://github.com/mobizt/FirebaseClient
 */

#include <WiFi.h>


#include <FirebaseClient.h>

#define WIFI_SSID "WAG KAMI MABAGAL NET!!"
#define WIFI_PASSWORD "binatog1432"

// The API key can be obtained from Firebase console > Project Overview > Project settings.
#define API_KEY "AIzaSyB0zBoo0h4sFUdc1l0rDSRAttgRk8NlAP8"

// User Email and password that already registerd or added in your project.
#define USER_EMAIL "encorotech@gmail.com"
#define USER_PASSWORD "iloverobotics143"
#define DATABASE_URL "/"

#define FIREBASE_PROJECT_ID "guardian-tracker-ee4b5"

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5
#define RST_PIN 4

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key; 
MFRC522::StatusCode status;

byte hash[16];
byte blockSize = 18;


void asyncCB(AsyncResult &aResult);

void printResult(AsyncResult &aResult);

String getTimestampString(uint64_t sec, uint32_t nano);

DefaultNetwork network; // initilize with boolean parameter to enable/disable network reconnection

UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);

FirebaseApp app;

#include <WiFiClientSecure.h>
WiFiClientSecure ssl_client;


using AsyncClient = AsyncClientClass;

AsyncClient aClient(ssl_client, getNetwork(network));

Firestore::Documents Docs;

bool taskCompleted = false;
bool isWifiAvailable = true;

int cnt = 0;

void setRFIDKey() {
   for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
}
String byteArrayToString(byte *array) {
  String result = "";
    for (int i = 0; i < sizeof(array) / sizeof(array[0]); i++) {

    result += (char)array[i];
  }
  return result;
}
String hexToText(const String &hexString) {
  String result = "";
  const unsigned int strLen = hexString.length();
  for (int i = 0; i < strLen; i += 2) {
    String hexByte = hexString.substring(i, i + 2);
    const char charByte = static_cast<char>(strtol(hexByte.c_str(), nullptr, 16));
    result += charByte;
  }

  return result;
}
void setup() {

  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  setRFIDKey();


  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
    if (millis() - startTime >= 5000) { // Check if 5 seconds have passed
      isWifiAvailable = false;
      break; // Exit the loop if WiFi connection failed
    }
  }

  if (isWifiAvailable) {
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    Serial.println("Initializing app...");

    ssl_client.setInsecure();

    initializeApp(aClient, app, getAuth(user_auth), asyncCB, "authTask");

    // Binding the FirebaseApp for authentication handler.
    // To unbind, use Docs.resetApp();
    app.getApp<Firestore::Documents>(Docs);
  } else {
    // Handle WiFi connection failure
    Serial.println("WiFi connection failed");
  }
}

void loop()
{
    // The async task handler should run inside the main loop
    // without blocking delay or bypassing with millis code blocks.

    if(isWifiAvailable){
      app.loop();

      Docs.loop();
  // add condition when a rfid is scanned upload to db
      if (app.ready() && !taskCompleted)
      {
          taskCompleted = true;

          // Note: If new document created under non-existent ancestor documents, that document will not appear in queries and snapshot
          // https://cloud.google.com/firestore/docs/using-console#non-existent_ancestor_documents.

          // We will create the document in the parent path "a0/b?
          // a0 is the collection id, b? is the document id in collection a0.

          String documentPath = "ATTENDANCE/08-26-24/present/";
          documentPath += cnt;

          String doc_path = "projects/";
          doc_path += FIREBASE_PROJECT_ID;
          doc_path += "/databases/(default)/documents/coll_id/doc_id"; // coll_id and doc_id are your collection id and document id

          // timestamp
          Values::TimestampValue tsV(getTimestampString(1712674441, 999999999));
          // stringa
          Values::StringValue strV(byteArrayToString(hash));
          Values::StringValue lateV("LATE");

          Document<Values::Value> doc("lrn", Values::Value(strV));
          doc.add("timestamp", Values::Value(tsV)).add("status",Values::Value(lateV));

          // The value of Values::xxxValue, Values::Value and Document can be printed on Serial.

          Serial.println("Create document... ");

          Docs.createDocument(aClient, Firestore::Parent(FIREBASE_PROJECT_ID), documentPath, DocumentMask(), doc, asyncCB, "createDocumentTask");
          cnt++;
      }
    }
    // loop for reading rfid
     if ( ! rfid.PICC_IsNewCardPresent())
        return;

    // Select one of the cards
    if ( ! rfid.PICC_ReadCardSerial())
        return;

  constexpr int fromBlock = 4;
  constexpr int toBlock = 4;
  int hashIndex = 0;

  for (int startFromBlock = fromBlock; startFromBlock <= toBlock; startFromBlock++) {
    status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, startFromBlock, &key, &rfid.uid);
    if (status == MFRC522::STATUS_OK) {
      byte buffer[18];
      byte size = sizeof(buffer);
      status = rfid.MIFARE_Read(startFromBlock, buffer, &size);
      if (status == MFRC522::STATUS_OK) {
        for (byte i = 0; i < 16; i++) {
          hash[i] = buffer[i];
        }

        // hashIndex += blockSize;
      } else {
        // lcdController.changeLcdText("Scan again...");
        Serial.println("Scan again...");
        Serial.println(rfid.GetStatusCodeName(status));

      }
    } else {
      // lcdController.changeLcdText("Too fast!");
        Serial.println("Too fast");
    }
  }
  // upload the file to db
  taskCompleted = false;
  for (int i = 0; i < sizeof(hash) / sizeof(hash[0]); i++) {
    Serial.print(hash[i]);
    Serial.print(" ");
  }
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void asyncCB(AsyncResult &aResult)
{
    // WARNING!
    // Do not put your codes inside the callback and printResult.
\
    printResult(aResult);
}

void printResult(AsyncResult &aResult)
{
    if (aResult.isEvent())
    {
        Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.appEvent().message().c_str(), aResult.appEvent().code());
    }

    if (aResult.isDebug())
    {
        Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError())
    {
        Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    }

    if (aResult.available())
    {
        Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
    }
}

String getTimestampString(uint64_t sec, uint32_t nano)
{
    if (sec > 0x3afff4417f)
        sec = 0x3afff4417f;

    if (nano > 0x3b9ac9ff)
        nano = 0x3b9ac9ff;

    time_t now;
    struct tm ts;
    char buf[80];
    now = sec;
    ts = *localtime(&now);

    String format = "%Y-%m-%dT%H:%M:%S";

    if (nano > 0)
    {
        String fraction = String(double(nano) / 1000000000.0f, 9);
        fraction.remove(0, 1);
        format += fraction;
    }
    format += "Z";

    strftime(buf, sizeof(buf), format.c_str(), &ts);
    return buf;
}