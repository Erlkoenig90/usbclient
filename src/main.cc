/*
 * Copyright (c) 2017, Niklas Gürtler
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 *    disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *    following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#include <iostream>
#include <stdexcept>
#include <iomanip>
#include <cstring>
#include <random>
#include <vector>
#include <string>
#include <chrono>
#include "libusb.h"

/**
 * Wird dieser Funktion ein libusb-Error Code übergeben, löst sie eine Exception mit der
 * gegebenen Fehlermeldung und den dem Code entsprechenden Informationen der LibUsb aus
 */
template <typename Ret>
Ret lu_err (Ret r, std::string errmsg) {
	if (r < 0)
		// Frage Fehlerbeschreibung der Libusb ab und baue Fehlermeldung zusammen
		throw std::runtime_error (errmsg + libusb_error_name (static_cast<int> (r)) + " - " + libusb_strerror (static_cast<libusb_error> (r)));
	return r;
}

/// Ein Dummy-Struct zur Freigabe des libusb context. Kann als "Deleter" in std::unique_ptr genutzt werden.
struct ExitLibusb {
	void operator () (libusb_context* ctx) {
		libusb_exit (ctx);
	}
};
/// Ein libusb_context welcher in diesem unique_ptr verpackt wird, wird automatisch korrekt freigegeben.
using CtxPtr = std::unique_ptr<libusb_context, ExitLibusb>;

/// Ein Dummy-Struct zur Freigabe von libusb_device* Listen. Kann als "Deleter" in std::unique_ptr genutzt werden.
struct FreeDeviceList {
	void operator () (libusb_device ** list) {
		libusb_free_device_list (list, 1);
	}
};
/// Eine Liste aus libusb_device* welche in diesem unique_ptr verpackt wird, wird automatisch korrekt freigegeben.
using DevListPtr = std::unique_ptr<libusb_device* [], FreeDeviceList>;

/// Ein Dummy-Struct zur Freigabe von libusb_device_handle. Kann als "Deleter" in std::unique_ptr genutzt werden.
struct CloseDevice {
	void operator () (libusb_device_handle* dev) {
		libusb_close (dev);
	}
};
/// Ein libusb_device_handle welcher in diesem unique_ptr verpackt wird, wird automatisch korrekt freigegeben.
using DevPtr = std::unique_ptr<libusb_device_handle, CloseDevice>;

/**
 * Sucht ein geeignetes USB-Gerät, öffnet es und gibt das entsprechende libusb-Handle zurück.
 * Außerdem wird der USB-Deskriptor in den Parameter "desc" geschrieben. Falls kein Gerät gefunden
 * wurde, wird eine Exception ausgelöst.
 */
DevPtr openDevice (libusb_device_descriptor& desc) {
	// Die Liste der angeschlossenen Geräte
	libusb_device **list_raw;
	// Frage Liste ab, libusb_get_device_list allokiert Speicher
	ssize_t cnt = lu_err(libusb_get_device_list (nullptr, &list_raw), "Liste angeschlossener Geräte konnte nicht abgefragt werden: ");
	
	// Verpacke Liste in unique_ptr für automatische Freigabe
	DevListPtr list (list_raw);

	// Iteriere gefundene Geräte
	ssize_t iFound = -1;
	std::cout << "Angeschlossene Geräte:\n";
	for (ssize_t i = 0; i < cnt; i++) {
		// Das Gerät
		libusb_device *device = list [i];
		libusb_device_descriptor deviceDescriptor;
		// Frage Device Descriptor ab
		lu_err (libusb_get_device_descriptor (device, &deviceDescriptor), "Konnte Geräte-Deskriptor nicht abfragen: ");

		// Gebe Adresse und VID/PID des Geräts aus
		std::cout	<< std::dec << int { libusb_get_bus_number(device) } << ":" << int { libusb_get_port_number(device) } << ":" << int { libusb_get_device_address (device) } << " "
					<< std::hex << std::setw(4) << std::setfill('0') << deviceDescriptor.idVendor << ":"
					<< std::hex << std::setw(4) << std::setfill('0') << deviceDescriptor.idProduct << std::endl;

		// Prüfe auf gewünschte VID+PID
		if (iFound == -1 && deviceDescriptor.idVendor == 0xDEAD && deviceDescriptor.idProduct == 0xBEEF) {
			// Merke Index
			iFound = i;
			// Merke Device-Descriptor
			std::memcpy (&desc, &deviceDescriptor, sizeof (deviceDescriptor));
		}
	}
	if (iFound == -1)
		throw std::runtime_error ("Kein passendes USB-Gerät gefunden.");
		

	// Öffne Device
	libusb_device_handle *handle = nullptr;
	lu_err (libusb_open (list [iFound], &handle), "Konnte Gerät nicht öffnen: ");
	
	// Verpacke Handle in unique_ptr für automatische Freigabe
	DevPtr devPtr (handle);

	// Beanspruche das Interface für diese Anwendung (sendet nichts auf dem Bus)
	lu_err (libusb_claim_interface (handle, 0), "Konnte Interface nicht öffnen: ");

	return devPtr;
}

/**
 * Fragt die String-Deskriptoren für iManufacturer, iProduct und iSerialNumber des Geräts ab
 * und gibt sie aus, falls vorhanden.
 */
void queryStrings (libusb_device_handle* handle, libusb_device_descriptor& foundDeviceDescriptor) {
	// libusb erwartet einen String-Puffer. 256 Bytes ist die Maximal-Länge bei String-Deskriptoren, und wir brauchen noch ein Zeichen mehr zum Terminieren
	unsigned char strBuffer [257];
	int len;

	if (foundDeviceDescriptor.iManufacturer != 0) {
		// Frage Deskriptor ab
		len = lu_err (libusb_get_string_descriptor_ascii (handle, foundDeviceDescriptor.iManufacturer, strBuffer, sizeof (strBuffer)-1), "Konnte Hersteller-String nicht abfragen: ");
		// Setze terminierendes 0-Byte
		strBuffer [len] = 0;
		std::cout << "Manufacturer: " << strBuffer << std::endl;
	}
	if (foundDeviceDescriptor.iProduct != 0) {
		// Frage Deskriptor ab
		len = lu_err (libusb_get_string_descriptor_ascii (handle, foundDeviceDescriptor.iProduct, strBuffer, sizeof (strBuffer)-1), "Konnte Produkt-String nicht abfragen: ");
		// Setze terminierendes 0-Byte
		strBuffer [len] = 0;
		std::cout << "Product: " << strBuffer << std::endl;
	}
	if (foundDeviceDescriptor.iSerialNumber != 0) {
		// Frage Deskriptor ab
		len = lu_err (libusb_get_string_descriptor_ascii (handle, foundDeviceDescriptor.iSerialNumber, strBuffer, sizeof (strBuffer)-1), "Konnte Seriennummer-String nicht abfragen: ");
		// Setze terminierendes 0-Byte
		strBuffer [len] = 0;
		std::cout << "Serial: " << strBuffer << std::endl;
	}
}

/**
 * Fragt den aktuellen Zustand der LED's ab und gibt ihn auf der Konsole aus. Wenn als
 * Parameter an das Programm zwei Zahlen übergeben wurde, werden die LED's entsprechend gesetzt
 */
void ledHandling (libusb_device_handle *handle, const std::vector<std::string>& args) {
	// Frage aktuellen Zustand ab, empfange dazu ein 1-Byte-Paket
	uint8_t ledData;
	lu_err (libusb_control_transfer (handle, 0xC0, 2, 0, 0, &ledData, 1, 0), "Konnte LED-Zustand nicht abfragen: ");

	// Extrahiere Daten aus Paket und gebe sie aus
	std::cout << "LED1: " << int {ledData & 1} << std::endl << "LED2: " << int {(ledData & 2) >> 1} << std::endl;

	if (args.size () > 2) {
		// Prüfe Kommandozeilenargumente
		bool LED1 = args[1] == "1";
		bool LED2 = args[2] == "1";
		// Baue Paket zusammen
		ledData = static_cast<uint8_t> (uint8_t{ LED1 }  | (uint8_t{ LED2 } << 1));

		// Sende Anfrage, nutze Paket für wValue
		lu_err (libusb_control_transfer (handle, 0x40, 1, ledData, 0, nullptr, 0, 0), "Konnte LED-Zustand nicht setzen: ");
	}
}

/**
 * Dreht den übergebenen Integer um.
 */
template <typename T>
T reverse (T val) {
	T temp = 0;
	// Iteriere jedes Bit
	for (size_t i = 0; i < CHAR_BIT * sizeof (T); ++i) {
		// Übernehme unterstes Bit der Eingabe in unterstes Bit der Ausgabe
		temp = static_cast<T> ((temp << 1) | (val & 1));
		// Shifte Ausgabe eins nach Links
		val = static_cast<T> (val >> 1);
	}
	return temp;
}

/**
 * Sendet eine zufällige Byte-Folge an den Bulk-Endpoint 1, empfängt die Antwort,
 * und prüft ob sie korrekt ist, d.h. jedes Byte umgedreht wurde.
 */
bool dataHandling (libusb_device_handle *handle) {
	// Puffer für beide Datenpakte, um sie vergleichen zu können
	unsigned char txBuffer [64], rxBuffer [64];
	// Initialisiere Pseude-Zufallszahlengenerator und nehme aktuelle Uhrzeit als Seed
	std::mt19937 gen (static_cast<uint_fast32_t> (std::chrono::system_clock::now ().time_since_epoch ().count ()));
	// Initialisiere uniforme Verteilung im Bereich 0-255
	std::uniform_int_distribution<uint16_t> dist (0, 0xFF);
	std::cout << "Sende Daten     : ";

	// Fülle Sendepuffer
	for (uint8_t& val : txBuffer) {
		// Berechne Zufallswert
		val = static_cast<uint8_t> (dist (gen));
		// Gebe ihn aus und speichere ihn
		std::cout << std::hex << std::setw (2) << std::setfill ('0') << int{ val }  << ", ";
	}
	std::cout << std::endl;
	// Sende Datenblock
	int sent;
	lu_err (libusb_bulk_transfer (handle, 1, txBuffer, sizeof (txBuffer), &sent, 0), "OUT Transfer fehlgeschlagen: ");

	// Empfange antwort
	lu_err (libusb_bulk_transfer (handle, 0x81, rxBuffer, sizeof (rxBuffer), &sent, 0), "IN Transfer fehlgeschlagen: ");

	std::cout << "Empfangene Daten: ";
	bool ok = true;
	// Iteriere empfangenes Paket
	for (size_t i = 0; i < sizeof (rxBuffer); ++i) {
		// Gebe empfangenes Byte aus
		std::cout << std::hex << std::setw (2) << std::setfill ('0') << int{ rxBuffer [i] } << ", ";
		// Prüfe ob Byte korrekt umgedreht wurde
		ok = ok && (rxBuffer [i] == reverse (txBuffer [i]));
	}
	std::cout << std::endl;
	// Prüfe ob alle Bytes korrekt gedreht wurden
	std::cout << "Daten stimmen überein: " << std::boolalpha << ok << std::endl;

	return true;
}

int main (int argc, char* argv []) {
	try {
		// Konvertiere Programmargumente in C++-Datenstruktur
		std::vector<std::string> args (argv, argv+argc);
		
		// Initialisiere libusb
		libusb_context* ctx;
		lu_err (libusb_init (&ctx), "Initialisierung von libusb fehlgeschlagen: ");
		// Verpacke libusb Kontext in unique_ptr für automatische Freigabe
		CtxPtr ctxPtr (ctx);

		// Öffne Gerät
		libusb_device_descriptor foundDeviceDescriptor {};
		DevPtr handle = openDevice (foundDeviceDescriptor);

		// Strings aus Device-Descriptor abfragen & ausgeben
		queryStrings (handle.get (), foundDeviceDescriptor);
		// LED's abfragen & setzen
		ledHandling (handle.get (), args);
		// Daten auf Bulk Endpoint 1 senden/empfangen
		return (dataHandling (handle.get ()) ? 0 : 1);
	} catch (const std::exception& e) {
		// Gebe Exception-Text aus
		std::cerr << e.what () << std::endl;

		return 1;
	}
}

