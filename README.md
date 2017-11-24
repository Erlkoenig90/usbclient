# usbclient
Dies ist ein simples PC-Programm zur Ansteuerung des USB-Gerätes welches im Projekt [f1usb](https://github.com/Erlkoenig90/f1usb) entwickelt wurde. Es basiert auf [libusb](http://libusb.info) und funktioniert unter Linux und Windows. Auf der [Releases](https://github.com/Erlkoenig90/usbclient/releases)-Seite können fertig kompilierte Dateien heruntergeladen werden.

## Kompilieren
Das Projekt nutzt [CMake](https://cmake.org/) als Build-System. Unter Linux kann es so kompiliert werden:
```shell
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
make
```
Statt "Release" kann auch "Debug" angegeben werden um Optimierungen aus- und Debuginformationen einzuschalten.

Unter Windows kann mit CMake ein Visual Studio-Projekt für 64bit erzeugt werden:
```shell
cmake -G "Visual Studio 15 2017 Win64"
```
Alternativ für 32bit:
```shell
cmake -G "Visual Studio 15 2017"
```

Das Visual Studio-Projekt enthält vier Konfigurationen:

Name | Beschreibung
-----|-------------
Debug | Unoptimierte Debug-Version, dynamisch gelinkt (libusb-DLL wird zur Ausführung gebraucht)
Release | Optimiertes Programm mit statisch gelinkten Bibliotheken (keine DLLs benötigt)
MinSizeRel | Auf Größe optimiertes Programm mit dynamisch gelinkten Bibliotheken (libusb-DLL wird zur Ausführung gebraucht)
RelWithDebInfo | Wie Release, mit Debug-Informationen

Unter Linux wird pkg-config genutzt, um libusb zu finden, welches per Paketmanager installiert werden muss. Für Windows enthält das Projekt fertig kompilierte Binaries im "libusb-msvc"-Verzeichnis, die automatisch mit gelinkt werden. Diese wurden mit und für Visual Studio 15 2017 erstellt. Für ältere Versionen können die Bibliotheksdateien von der libusb-Website heruntergeladen werden. Die statische Version davon funktioniert dann aber nicht mit der aktuellen Visual Studio-Version.

Tip: Alle Dateien, die nicht zum git-Repository gehören, können so gelöscht werden:
```shell
git clean -fdx
```
So kann man die große Zahl an durch CMake und Visual Studio angelegten Dateien aufräumen, ohne den Source-Code zu löschen.

## Funktion
Das Programm greift via libusb direkt auf ein am PC angeschlossenes Gerät zu, welches mit dem "USB-Hello-World" [f1usb](https://github.com/Erlkoenig90/f1usb) erstellt sein sollte. Es zeigt zunächst die Adressen und ID's aller angeschlossenen Geräte an, öffnet falls möglich das mit der passenden ID, und zeigt dessen String-Deskriptoren an. Es fragt den aktuellen Zustand der LED's ab, und erlaubt das Setzen der LED's auf die über zwei Kommandozeilen-Argumente anzugebenden Zustände, welche 1 oder 0 sein müssen. Außerdem sendet es eine zufällige Folge an Bytes an den Bulk Endpoint 1, empfängt die gleich lange Antwort, zeigt beide an und prüft, ob in der Antwort wie gewünscht jedes Byte umgedreht wurde. Ein Beispiel-Lauf des Programms ist (gekürzt):
```shell
$ ./usbclient 0 1
Angeschlossene Geräte:
2:8:5 04f2:b336
2:5:9 04ca:300b
2:2:10 dead:beef
2:4:6 046d:0a1f
[...]
Manufacturer: ACME Corp.
Product: Fluxkompensator
Serial: 42-1337-47/11
LED1: 1
LED2: 0
Sende Daten     : de, c8, 1d, a0, 7b, 33, 65, f0 [...]
Empfangene Daten: 7b, 13, b8, 05, de, cc, a6, 0f [...]
Daten stimmen überein: true
```

## Lizenz
Dieser Code steht unter der BSD-Lizenz, siehe dazu die Datei [LICENSE](LICENSE).
