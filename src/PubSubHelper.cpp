#include "MitoSoft.h"
#include <Arduino.h>
#include <string.h>
#include <PubSubClient.h>

//https://github.com/knolleary/pubsubclient

bool PubSubHelper::_writeLog;

String PubSubHelper::_topic;

String PubSubHelper::_message;

bool PubSubHelper::_onMessageReceived;

PubSubHelper::PubSubHelper(PubSubClient& client, unsigned long reconnectionTime, bool writeLog)
{
	this->_mqttClient = &client;
	this->_writeLog = writeLog;
	this->_reconnectionTime = reconnectionTime;
	this->_actualTime = millis();
	
	_onConnected = false;
	_onMessageReceived = false;
	
	_topic = "";
	_message = "";
}

void PubSubHelper::messageReceived(char* topic, byte* payload, unsigned int length)
{
	_topic = (String)topic;

	String message = "";
	for (int i = 0; i < length; i++) {
		message = message + (char)payload[i];
	}
	_message = message;

	writeSerial("Message received - Topic " + _topic + "; Message " + _message);
	
	if(_topic != "") {
	   _onMessageReceived = true;
	}
}

void PubSubHelper::init(char* clientId) {
	this->_clientId = clientId;
}

String PubSubHelper::getLastTopic()
{
	return _topic;
}

String PubSubHelper::getLastMessage()
{
	return _message;
}

//Es wird nur QOS Level 0 und 1 unterstützt (mit 2 funktioniert es erst gat nicht!)
//QOS0 => Nachrichten maximal einmal liefern (eine Nachricht wird nur einmal geschickt ohne Rücksicht auf Verluste)
//QOS1 => Nachrichten werden mindestens einmal abgeliefert (könnte aber auch mehrere Male abgeliefert werden!!!)
void PubSubHelper::subscribe(String topic)
{
	char t[topic.length() + 1];
	topic.toCharArray(t, topic.length() + 1);

	_mqttClient->subscribe(t, 0);

	writeSerial("MQTT subscription - Topic " + (String)topic);
}

//Es wird nur QOS Level 0 unterstützt
//QOS0 => Nachrichten maximal einmal liefern (Fire and forget)
void PubSubHelper::publish(String topic, String message, bool retain = false)
{
	if (_mqttClient->connected()) {
				
		char t[topic.length() + 1];
		topic.toCharArray(t, topic.length() + 1);

		char m[message.length() + 1];
		message.toCharArray(m, message.length() + 1);

		_mqttClient->publish(t, m, retain);

		writeSerial("MQTT message published - Topic " + (String)topic + "; Message " + (String)message);
	}
}

//nonblocking reconnect
//https://github.com/knolleary/pubsubclient/blob/v2.7/examples/mqtt_reconnect_nonblocking/mqtt_reconnect_nonblocking.ino
void PubSubHelper::loop() {
	if (!_mqttClient->connected()) {
		if (millis() - _actualTime > _reconnectionTime) {
			_actualTime = millis();
			writeSerial("MQTT reconnecting... " + String(millis()) + " millis");
			this->connect();
		}
	}
	else {
		_mqttClient->loop(); // Client connected
	}
};

void PubSubHelper::writeSerial(String text) {
	if (true == _writeLog) {
		Serial.println("PubSubHelper: " + text);
	}
}


bool PubSubHelper::connect() {
	if (!_mqttClient->connect(_clientId)) {
		writeSerial("MQTT connection failed: Error code " + String(_mqttClient->state()));

		return false;
	}
	else {
		writeSerial("MQTT connected to broker: " + (String)_clientId);

		_mqttClient->setCallback(*messageReceived); //eventuell die 
		//setCallback Funktion in Konstruktor verlagern
        
		_onConnected = true;
		
		return true;
	}
}

//*******************************************************************************************************************************
//EVENTS
//
bool PubSubHelper::onConnected() {	
	bool connected = _onConnected;
	_onConnected = false;
	return connected;
}

bool PubSubHelper::onMessageReceived() {	
	bool messageReceived = _onMessageReceived;
	_onMessageReceived = false;
	return messageReceived;
}