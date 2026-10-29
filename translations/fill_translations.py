#!/usr/bin/env python3
"""Fills the .ts files with translations (uk, da, de, pl).

Workflow when interface strings change:
  1. cmake --build build --target update_translations   (or run lupdate)
  2. add the new strings to T below
  3. python3 translations/fill_translations.py
Strings missing here stay "unfinished" and show in English.
"""
import os
import sys
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
LANGS = ["uk", "da", "de", "pl"]

# source text -> {lang: translation}; plurals are lists of forms
# (uk/pl: one, few, many; da/de: one, other).
T = {
    # --- errors -------------------------------------------------------------
    "Unsupported RAW output layout": dict(
        uk="Непідтримуваний формат даних RAW", da="Ikke-understøttet RAW-dataformat",
        de="Nicht unterstütztes RAW-Datenformat", pl="Nieobsługiwany format danych RAW"),
    "This build has no RAW support (LibRaw missing)": dict(
        uk="Ця збірка не підтримує RAW (немає LibRaw)",
        da="Denne version understøtter ikke RAW (LibRaw mangler)",
        de="Diese Version unterstützt kein RAW (LibRaw fehlt)",
        pl="Ta wersja nie obsługuje RAW (brak LibRaw)"),
    "Cannot read %1: %2": dict(uk="Не вдалося прочитати %1: %2", da="Kan ikke læse %1: %2",
                               de="%1 kann nicht gelesen werden: %2", pl="Nie można odczytać %1: %2"),
    "Processing failed": dict(uk="Помилка обробки", da="Behandlingen mislykkedes",
                              de="Verarbeitung fehlgeschlagen", pl="Przetwarzanie nie powiodło się"),
    "Cannot create folder %1": dict(uk="Не вдалося створити папку %1", da="Kan ikke oprette mappen %1",
                                    de="Ordner %1 kann nicht erstellt werden", pl="Nie można utworzyć folderu %1"),
    "Cannot open the catalog:\n%1": dict(uk="Не вдалося відкрити каталог:\n%1",
                                         da="Kan ikke åbne kataloget:\n%1",
                                         de="Der Katalog kann nicht geöffnet werden:\n%1",
                                         pl="Nie można otworzyć katalogu:\n%1"),

    # --- adjust panel -------------------------------------------------------------
    "Light & Colour": dict(uk="Світло і колір", da="Lys og farve", de="Licht & Farbe", pl="Światło i kolor"),
    "Auto tone": dict(uk="Автотон", da="Autotone", de="Autoton", pl="Autoton"),
    "Set exposure, contrast, whites and blacks from the histogram": dict(
        uk="Налаштувати експозицію, контраст, білі й чорні за гістограмою",
        da="Indstil eksponering, kontrast, hvide og sorte ud fra histogrammet",
        de="Belichtung, Kontrast, Weiß und Schwarz anhand des Histogramms einstellen",
        pl="Ustaw ekspozycję, kontrast, biele i czernie na podstawie histogramu"),
    "Temperature": dict(uk="Температура", da="Temperatur", de="Farbtemperatur", pl="Temperatura"),
    "Tint": dict(uk="Відтінок", da="Farvetone", de="Tönung", pl="Odcień"),
    "Exposure": dict(uk="Експозиція", da="Eksponering", de="Belichtung", pl="Ekspozycja"),
    "Contrast": dict(uk="Контраст", da="Kontrast", de="Kontrast", pl="Kontrast"),
    "Highlights": dict(uk="Світлі тони", da="Højlys", de="Lichter", pl="Światła"),
    "Shadows": dict(uk="Тіні", da="Skygger", de="Tiefen", pl="Cienie"),
    "Whites": dict(uk="Білі", da="Hvide", de="Weiß", pl="Biele"),
    "Blacks": dict(uk="Чорні", da="Sorte", de="Schwarz", pl="Czernie"),
    "Clarity": dict(uk="Чіткість", da="Klarhed", de="Klarheit", pl="Przejrzystość"),
    "Vibrance": dict(uk="Соковитість", da="Levende farver", de="Dynamik", pl="Jaskrawość"),
    "Saturation": dict(uk="Насиченість", da="Mætning", de="Sättigung", pl="Nasycenie"),
    "Black && white": dict(uk="Чорно-біле", da="Sort-hvid", de="Schwarzweiß", pl="Czarno-białe"),
    "Tone Curve": dict(uk="Крива тонів", da="Tonekurve", de="Gradationskurve", pl="Krzywa tonalna"),
    "Reset curve": dict(uk="Скинути криву", da="Nulstil kurve", de="Kurve zurücksetzen", pl="Resetuj krzywą"),
    "Crop & Straighten": dict(uk="Кадрування й вирівнювання", da="Beskær og ret ud",
                              de="Freistellen & Ausrichten", pl="Kadrowanie i prostowanie"),
    "Press <b>C</b> or the Crop button to drag the frame.": dict(
        uk="Натисніть <b>C</b> або кнопку «Кадрування», щоб змінити рамку.",
        da="Tryk på <b>C</b> eller knappen Beskær for at trække i rammen.",
        de="Drücken Sie <b>C</b> oder die Schaltfläche „Freistellen“, um den Rahmen zu ziehen.",
        pl="Naciśnij <b>C</b> lub przycisk Kadruj, aby przeciągnąć ramkę."),
    "Aspect": dict(uk="Пропорції", da="Format", de="Seitenverhältnis", pl="Proporcje"),
    "Free": dict(uk="Вільні", da="Fri", de="Frei", pl="Dowolne"),
    "Original": dict(uk="Оригінальні", da="Original", de="Original", pl="Oryginalne"),
    "Straighten": dict(uk="Вирівнювання", da="Ret ud", de="Ausrichten", pl="Prostowanie"),
    "⟲ 90°": dict(uk="⟲ 90°", da="⟲ 90°", de="⟲ 90°", pl="⟲ 90°"),
    "⟳ 90°": dict(uk="⟳ 90°", da="⟳ 90°", de="⟳ 90°", pl="⟳ 90°"),
    "Reset": dict(uk="Скинути", da="Nulstil", de="Zurücksetzen", pl="Resetuj"),
    "Detail": dict(uk="Деталізація", da="Detaljer", de="Details", pl="Szczegóły"),
    "Sharpening": dict(uk="Різкість", da="Skarphed", de="Schärfen", pl="Wyostrzanie"),
    "Radius": dict(uk="Радіус", da="Radius", de="Radius", pl="Promień"),
    "Effects": dict(uk="Ефекти", da="Effekter", de="Effekte", pl="Efekty"),
    "Vignette": dict(uk="Віньєтка", da="Vignettering", de="Vignettierung", pl="Winieta"),
    "Midpoint": dict(uk="Середина", da="Midtpunkt", de="Mittelpunkt", pl="Punkt środkowy"),

    # --- develop toolbar --------------------------------------------------------------
    "Undo": dict(uk="Скасувати", da="Fortryd", de="Rückgängig", pl="Cofnij"),
    "Undo (⌘Z)": dict(uk="Скасувати (⌘Z)", da="Fortryd (⌘Z)", de="Rückgängig (⌘Z)", pl="Cofnij (⌘Z)"),
    "Redo": dict(uk="Повторити", da="Gentag", de="Wiederholen", pl="Ponów"),
    "Redo (⇧⌘Z)": dict(uk="Повторити (⇧⌘Z)", da="Gentag (⇧⌘Z)", de="Wiederholen (⇧⌘Z)", pl="Ponów (⇧⌘Z)"),
    "Before / After": dict(uk="До / Після", da="Før / Efter", de="Vorher / Nachher", pl="Przed / Po"),
    "Show the original (\\)": dict(uk="Показати оригінал (\\)", da="Vis originalen (\\)",
                                   de="Original anzeigen (\\)", pl="Pokaż oryginał (\\)"),
    "Crop": dict(uk="Кадрування", da="Beskær", de="Freistellen", pl="Kadruj"),
    "Crop & straighten (C)": dict(uk="Кадрування й вирівнювання (C)", da="Beskær og ret ud (C)",
                                  de="Freistellen & Ausrichten (C)", pl="Kadrowanie i prostowanie (C)"),
    "Fit / 100%": dict(uk="Вмістити / 100%", da="Tilpas / 100%", de="Einpassen / 100%", pl="Dopasuj / 100%"),
    "Toggle zoom (Z) — or double-click the photo": dict(
        uk="Перемкнути масштаб (Z) — або двічі клацніть фото",
        da="Skift zoom (Z) — eller dobbeltklik på fotoet",
        de="Zoom umschalten (Z) – oder auf das Foto doppelklicken",
        pl="Przełącz powiększenie (Z) — lub kliknij dwukrotnie zdjęcie"),
    "Copy settings": dict(uk="Копіювати налаштування", da="Kopiér indstillinger",
                          de="Einstellungen kopieren", pl="Kopiuj ustawienia"),
    "Copy develop settings (⇧⌘C)": dict(uk="Копіювати налаштування обробки (⇧⌘C)",
                                        da="Kopiér fremkaldelsesindstillinger (⇧⌘C)",
                                        de="Entwicklungseinstellungen kopieren (⇧⌘C)",
                                        pl="Kopiuj ustawienia wywołania (⇧⌘C)"),
    "Paste settings": dict(uk="Вставити налаштування", da="Indsæt indstillinger",
                           de="Einstellungen einfügen", pl="Wklej ustawienia"),
    "Paste develop settings (⇧⌘V)": dict(uk="Вставити налаштування обробки (⇧⌘V)",
                                         da="Indsæt fremkaldelsesindstillinger (⇧⌘V)",
                                         de="Entwicklungseinstellungen einfügen (⇧⌘V)",
                                         pl="Wklej ustawienia wywołania (⇧⌘V)"),
    "Reset all adjustments": dict(uk="Скинути всі налаштування", da="Nulstil alle justeringer",
                                  de="Alle Anpassungen zurücksetzen", pl="Resetuj wszystkie korekty"),
    "Export…": dict(uk="Експорт…", da="Eksportér…", de="Exportieren…", pl="Eksportuj…"),
    "Export (⇧⌘E)": dict(uk="Експорт (⇧⌘E)", da="Eksportér (⇧⌘E)", de="Exportieren (⇧⌘E)", pl="Eksportuj (⇧⌘E)"),
    "Loading %1…": dict(uk="Завантаження %1…", da="Indlæser %1…", de="%1 wird geladen…", pl="Wczytywanie %1…"),
    "Cannot open %1\n%2": dict(uk="Не вдалося відкрити %1\n%2", da="Kan ikke åbne %1\n%2",
                               de="%1 kann nicht geöffnet werden\n%2", pl="Nie można otworzyć %1\n%2"),

    # --- export dialog ------------------------------------------------------------------
    "Export": dict(uk="Експорт", da="Eksportér", de="Exportieren", pl="Eksportuj"),
    "JPEG": dict(uk="JPEG", da="JPEG", de="JPEG", pl="JPEG"),
    "PNG": dict(uk="PNG", da="PNG", de="PNG", pl="PNG"),
    "TIFF": dict(uk="TIFF", da="TIFF", de="TIFF", pl="TIFF"),
    "Resize to long edge": dict(uk="Зменшити за довгою стороною", da="Skalér til lange kant",
                                de="Auf lange Kante skalieren", pl="Skaluj do dłuższej krawędzi"),
    "Choose…": dict(uk="Вибрати…", da="Vælg…", de="Auswählen…", pl="Wybierz…"),
    "e.g. _edit": dict(uk="напр. _edit", da="f.eks. _edit", de="z. B. _edit", pl="np. _edit"),
    "Format": dict(uk="Формат", da="Format", de="Format", pl="Format"),
    "JPEG quality": dict(uk="Якість JPEG", da="JPEG-kvalitet", de="JPEG-Qualität", pl="Jakość JPEG"),
    "Size": dict(uk="Розмір", da="Størrelse", de="Größe", pl="Rozmiar"),
    "Folder": dict(uk="Папка", da="Mappe", de="Ordner", pl="Folder"),
    "File name suffix": dict(uk="Суфікс імені файлу", da="Filnavnssuffiks", de="Dateinamenszusatz",
                             pl="Przyrostek nazwy pliku"),
    "Exporting %n photo(s). Originals are not modified.": dict(
        uk=["Експортується %n фото. Оригінали не змінюються."] * 3,
        da=["Eksporterer %n foto. Originalerne ændres ikke.",
            "Eksporterer %n fotos. Originalerne ændres ikke."],
        de=["%n Foto wird exportiert. Originale werden nicht verändert.",
            "%n Fotos werden exportiert. Originale werden nicht verändert."],
        pl=["Zostanie wyeksportowane %n zdjęcie. Oryginały nie są zmieniane.",
            "Zostaną wyeksportowane %n zdjęcia. Oryginały nie są zmieniane.",
            "Zostanie wyeksportowanych %n zdjęć. Oryginały nie są zmieniane."]),
    "Export to": dict(uk="Експортувати в", da="Eksportér til", de="Exportieren nach", pl="Eksportuj do"),

    # --- canvas ---------------------------------------------------------------------------
    "No photo selected": dict(uk="Фото не вибрано", da="Intet foto valgt", de="Kein Foto ausgewählt",
                              pl="Nie wybrano zdjęcia"),
    "Before": dict(uk="До", da="Før", de="Vorher", pl="Przed"),

    # --- library ----------------------------------------------------------------------------
    "FOLDERS": dict(uk="ПАПКИ", da="MAPPER", de="ORDNER", pl="FOLDERY"),
    "Import Folder…": dict(uk="Імпортувати папку…", da="Importér mappe…", de="Ordner importieren…",
                           pl="Importuj folder…"),
    "Search file name or camera": dict(uk="Пошук за назвою файлу або камерою",
                                       da="Søg efter filnavn eller kamera",
                                       de="Nach Dateiname oder Kamera suchen",
                                       pl="Szukaj nazwy pliku lub aparatu"),
    "Any rating": dict(uk="Будь-який рейтинг", da="Alle vurderinger", de="Jede Bewertung",
                       pl="Dowolna ocena"),
    "%1+ ●": dict(uk="%1+ ●", da="%1+ ●", de="%1+ ●", pl="%1+ ●"),
    "Hide rejected": dict(uk="Сховати відхилені", da="Skjul afviste", de="Abgelehnte ausblenden",
                          pl="Ukryj odrzucone"),
    "All photos": dict(uk="Усі фото", da="Alle fotos", de="Alle Fotos", pl="Wszystkie zdjęcia"),
    "Picked only": dict(uk="Лише обрані", da="Kun valgte", de="Nur ausgewählte", pl="Tylko wybrane"),
    "Rejected only": dict(uk="Лише відхилені", da="Kun afviste", de="Nur abgelehnte", pl="Tylko odrzucone"),
    "Sort: capture time": dict(uk="Сортування: час зйомки", da="Sortér: optagelsestid",
                               de="Sortierung: Aufnahmezeit", pl="Sortuj: czas wykonania"),
    "Sort: file name": dict(uk="Сортування: назва файлу", da="Sortér: filnavn",
                            de="Sortierung: Dateiname", pl="Sortuj: nazwa pliku"),
    "Sort: import time": dict(uk="Сортування: час імпорту", da="Sortér: importtid",
                              de="Sortierung: Importzeit", pl="Sortuj: czas importu"),
    "Thumbnail size": dict(uk="Розмір мініатюр", da="Miniaturestørrelse", de="Miniaturgröße",
                           pl="Rozmiar miniatur"),
    "<p style='font-size:18px'>No photos here yet</p><p>Use <b>Import Folder…</b> to add a folder of photos.<br>Your originals are never changed.</p>": dict(
        uk="<p style='font-size:18px'>Тут поки немає фото</p><p>Натисніть <b>Імпортувати папку…</b>, щоб додати папку з фото.<br>Ваші оригінали ніколи не змінюються.</p>",
        da="<p style='font-size:18px'>Ingen fotos her endnu</p><p>Brug <b>Importér mappe…</b> for at tilføje en mappe med fotos.<br>Dine originaler ændres aldrig.</p>",
        de="<p style='font-size:18px'>Noch keine Fotos</p><p>Mit <b>Ordner importieren…</b> fügen Sie einen Fotoordner hinzu.<br>Ihre Originale werden nie verändert.</p>",
        pl="<p style='font-size:18px'>Nie ma tu jeszcze zdjęć</p><p>Użyj <b>Importuj folder…</b>, aby dodać folder ze zdjęciami.<br>Twoje oryginały nigdy nie są zmieniane.</p>"),
    "All Photographs   %1": dict(uk="Усі фотографії   %1", da="Alle fotografier   %1",
                                  de="Alle Fotos   %1", pl="Wszystkie zdjęcia   %1"),

    # --- licenses ---------------------------------------------------------------------------
    "Licenses": dict(uk="Ліцензії", da="Licenser", de="Lizenzen", pl="Licencje"),
    "<h3>Svitlo %1</h3><p>Released under the MIT License.</p><pre style='white-space: pre-wrap'>%2</pre>": dict(
        uk="<h3>Svitlo %1</h3><p>Поширюється за ліцензією MIT.</p><pre style='white-space: pre-wrap'>%2</pre>",
        da="<h3>Svitlo %1</h3><p>Udgivet under MIT-licensen.</p><pre style='white-space: pre-wrap'>%2</pre>",
        de="<h3>Svitlo %1</h3><p>Veröffentlicht unter der MIT-Lizenz.</p><pre style='white-space: pre-wrap'>%2</pre>",
        pl="<h3>Svitlo %1</h3><p>Udostępniane na licencji MIT.</p><pre style='white-space: pre-wrap'>%2</pre>"),
    "<h3>Qt %1</h3><p>This application uses the Qt framework, © The Qt Company Ltd. and other contributors, under the <b>GNU Lesser General Public License version 3</b> (LGPL v3).</p><p>Qt is linked dynamically: the Qt libraries are separate files inside the application bundle (<code>Svitlo.app/Contents/Frameworks</code>) and can be replaced with a compatible, modified version.</p><p>Licence text: <a href='https://www.gnu.org/licenses/lgpl-3.0.html'>gnu.org/licenses/lgpl-3.0.html</a><br>Qt source code: <a href='https://download.qt.io/archive/qt/'>download.qt.io/archive/qt</a></p>": dict(
        uk="<h3>Qt %1</h3><p>Ця програма використовує фреймворк Qt, © The Qt Company Ltd. та інші автори, за ліцензією <b>GNU Lesser General Public License версії 3</b> (LGPL v3).</p><p>Qt підключено динамічно: бібліотеки Qt — це окремі файли всередині пакета програми (<code>Svitlo.app/Contents/Frameworks</code>), і їх можна замінити сумісною зміненою версією.</p><p>Текст ліцензії: <a href='https://www.gnu.org/licenses/lgpl-3.0.html'>gnu.org/licenses/lgpl-3.0.html</a><br>Вихідний код Qt: <a href='https://download.qt.io/archive/qt/'>download.qt.io/archive/qt</a></p>",
        da="<h3>Qt %1</h3><p>Dette program bruger Qt-frameworket, © The Qt Company Ltd. og andre bidragydere, under <b>GNU Lesser General Public License version 3</b> (LGPL v3).</p><p>Qt er linket dynamisk: Qt-bibliotekerne er separate filer i programpakken (<code>Svitlo.app/Contents/Frameworks</code>) og kan erstattes med en kompatibel, ændret version.</p><p>Licenstekst: <a href='https://www.gnu.org/licenses/lgpl-3.0.html'>gnu.org/licenses/lgpl-3.0.html</a><br>Qt-kildekode: <a href='https://download.qt.io/archive/qt/'>download.qt.io/archive/qt</a></p>",
        de="<h3>Qt %1</h3><p>Diese Anwendung verwendet das Qt-Framework, © The Qt Company Ltd. und weitere Mitwirkende, unter der <b>GNU Lesser General Public License Version 3</b> (LGPL v3).</p><p>Qt ist dynamisch gelinkt: Die Qt-Bibliotheken sind separate Dateien im Anwendungspaket (<code>Svitlo.app/Contents/Frameworks</code>) und können durch eine kompatible, geänderte Version ersetzt werden.</p><p>Lizenztext: <a href='https://www.gnu.org/licenses/lgpl-3.0.html'>gnu.org/licenses/lgpl-3.0.html</a><br>Qt-Quellcode: <a href='https://download.qt.io/archive/qt/'>download.qt.io/archive/qt</a></p>",
        pl="<h3>Qt %1</h3><p>Ta aplikacja korzysta z frameworka Qt, © The Qt Company Ltd. i inni autorzy, na licencji <b>GNU Lesser General Public License w wersji 3</b> (LGPL v3).</p><p>Qt jest łączony dynamicznie: biblioteki Qt to osobne pliki w pakiecie aplikacji (<code>Svitlo.app/Contents/Frameworks</code>) i można je zastąpić zgodną, zmodyfikowaną wersją.</p><p>Tekst licencji: <a href='https://www.gnu.org/licenses/lgpl-3.0.html'>gnu.org/licenses/lgpl-3.0.html</a><br>Kod źródłowy Qt: <a href='https://download.qt.io/archive/qt/'>download.qt.io/archive/qt</a></p>"),
    "<h3>LibRaw</h3><p>RAW decoding uses LibRaw, © LibRaw LLC. LibRaw is available under a choice of the <b>GNU LGPL 2.1</b> or the <b>CDDL 1.0</b>; this application uses it under the <b>CDDL 1.0</b>. The unmodified LibRaw source code is available at <a href='https://github.com/LibRaw/LibRaw'>github.com/LibRaw/LibRaw</a>.</p><p>LibRaw contains code from dcraw by Dave Coffin and DCB/FBDD demosaic code by Jacek Gozdz (BSD-style licence).</p><p>The full licence texts are included in the application bundle in <code>Svitlo.app/Contents/Resources/licenses</code>.</p>": dict(
        uk="<h3>LibRaw</h3><p>Для декодування RAW використовується LibRaw, © LibRaw LLC. LibRaw доступна на вибір за ліцензією <b>GNU LGPL 2.1</b> або <b>CDDL 1.0</b>; ця програма використовує її за <b>CDDL 1.0</b>. Незмінений вихідний код LibRaw доступний на <a href='https://github.com/LibRaw/LibRaw'>github.com/LibRaw/LibRaw</a>.</p><p>LibRaw містить код dcraw від Dave Coffin і код демозаїки DCB/FBDD від Jacek Gozdz (ліцензія типу BSD).</p><p>Повні тексти ліцензій містяться в пакеті програми в <code>Svitlo.app/Contents/Resources/licenses</code>.</p>",
        da="<h3>LibRaw</h3><p>RAW-afkodning bruger LibRaw, © LibRaw LLC. LibRaw fås efter eget valg under <b>GNU LGPL 2.1</b> eller <b>CDDL 1.0</b>; dette program bruger den under <b>CDDL 1.0</b>. Den uændrede LibRaw-kildekode findes på <a href='https://github.com/LibRaw/LibRaw'>github.com/LibRaw/LibRaw</a>.</p><p>LibRaw indeholder kode fra dcraw af Dave Coffin og DCB/FBDD-demosaickode af Jacek Gozdz (BSD-lignende licens).</p><p>De fulde licenstekster findes i programpakken i <code>Svitlo.app/Contents/Resources/licenses</code>.</p>",
        de="<h3>LibRaw</h3><p>Die RAW-Dekodierung verwendet LibRaw, © LibRaw LLC. LibRaw ist wahlweise unter der <b>GNU LGPL 2.1</b> oder der <b>CDDL 1.0</b> verfügbar; diese Anwendung nutzt sie unter der <b>CDDL 1.0</b>. Der unveränderte LibRaw-Quellcode ist unter <a href='https://github.com/LibRaw/LibRaw'>github.com/LibRaw/LibRaw</a> verfügbar.</p><p>LibRaw enthält Code aus dcraw von Dave Coffin sowie DCB/FBDD-Demosaicing-Code von Jacek Gozdz (BSD-ähnliche Lizenz).</p><p>Die vollständigen Lizenztexte befinden sich im Anwendungspaket unter <code>Svitlo.app/Contents/Resources/licenses</code>.</p>",
        pl="<h3>LibRaw</h3><p>Do dekodowania RAW używana jest biblioteka LibRaw, © LibRaw LLC. LibRaw jest dostępna do wyboru na licencji <b>GNU LGPL 2.1</b> lub <b>CDDL 1.0</b>; ta aplikacja korzysta z niej na licencji <b>CDDL 1.0</b>. Niezmodyfikowany kod źródłowy LibRaw jest dostępny na <a href='https://github.com/LibRaw/LibRaw'>github.com/LibRaw/LibRaw</a>.</p><p>LibRaw zawiera kod z dcraw autorstwa Dave'a Coffina oraz kod demozaikowania DCB/FBDD autorstwa Jacka Gozdzia (licencja typu BSD).</p><p>Pełne teksty licencji znajdują się w pakiecie aplikacji w <code>Svitlo.app/Contents/Resources/licenses</code>.</p>"),

    # --- main window --------------------------------------------------------------------------
    "Library": dict(uk="Бібліотека", da="Bibliotek", de="Bibliothek", pl="Biblioteka"),
    "Develop": dict(uk="Обробка", da="Fremkald", de="Entwickeln", pl="Wywoływanie"),
    "Ready": dict(uk="Готово", da="Klar", de="Bereit", pl="Gotowe"),
    "RAW support is not available in this build (LibRaw missing)": dict(
        uk="Підтримка RAW недоступна в цій збірці (немає LibRaw)",
        da="RAW-understøttelse er ikke tilgængelig i denne version (LibRaw mangler)",
        de="RAW-Unterstützung ist in dieser Version nicht verfügbar (LibRaw fehlt)",
        pl="Obsługa RAW nie jest dostępna w tej wersji (brak LibRaw)"),
    "&File": dict(uk="&Файл", da="&Fil", de="&Datei", pl="&Plik"),
    "Quit": dict(uk="Вийти", da="Afslut", de="Beenden", pl="Zakończ"),
    "&Edit": dict(uk="&Редагування", da="&Rediger", de="&Bearbeiten", pl="&Edycja"),
    "Copy Settings": dict(uk="Копіювати налаштування", da="Kopiér indstillinger",
                          de="Einstellungen kopieren", pl="Kopiuj ustawienia"),
    "Paste Settings": dict(uk="Вставити налаштування", da="Indsæt indstillinger",
                           de="Einstellungen einfügen", pl="Wklej ustawienia"),
    "Reset Settings": dict(uk="Скинути налаштування", da="Nulstil indstillinger",
                           de="Einstellungen zurücksetzen", pl="Resetuj ustawienia"),
    "Select All": dict(uk="Вибрати все", da="Vælg alle", de="Alles auswählen", pl="Zaznacz wszystko"),
    "&Photo": dict(uk="Ф&ото", da="F&oto", de="&Foto", pl="&Zdjęcie"),
    "Clear Rating": dict(uk="Прибрати рейтинг", da="Fjern vurdering", de="Bewertung entfernen",
                         pl="Usuń ocenę"),
    "Rate %1": dict(uk="Рейтинг %1", da="Vurdér %1", de="Mit %1 bewerten", pl="Oceń na %1"),
    "Pick": dict(uk="Обрати", da="Vælg", de="Auswählen", pl="Wybierz"),
    "Reject": dict(uk="Відхилити", da="Afvis", de="Ablehnen", pl="Odrzuć"),
    "Unflag": dict(uk="Зняти позначку", da="Fjern markering", de="Markierung entfernen",
                   pl="Usuń oznaczenie"),
    "Rotate Left": dict(uk="Повернути ліворуч", da="Rotér til venstre", de="Nach links drehen",
                        pl="Obróć w lewo"),
    "Rotate Right": dict(uk="Повернути праворуч", da="Rotér til højre", de="Nach rechts drehen",
                         pl="Obróć w prawo"),
    "Remove from Catalog…": dict(uk="Видалити з каталогу…", da="Fjern fra katalog…",
                                 de="Aus Katalog entfernen…", pl="Usuń z katalogu…"),
    "&View": dict(uk="&Вигляд", da="&Vis", de="&Ansicht", pl="&Widok"),
    "Crop Tool": dict(uk="Інструмент кадрування", da="Beskæringsværktøj", de="Freistellungswerkzeug",
                      pl="Narzędzie kadrowania"),
    "Toggle Zoom": dict(uk="Перемкнути масштаб", da="Skift zoom", de="Zoom umschalten",
                        pl="Przełącz powiększenie"),
    "Next Photo": dict(uk="Наступне фото", da="Næste foto", de="Nächstes Foto", pl="Następne zdjęcie"),
    "Previous Photo": dict(uk="Попереднє фото", da="Forrige foto", de="Vorheriges Foto",
                           pl="Poprzednie zdjęcie"),
    "Language": dict(uk="Мова", da="Sprog", de="Sprache", pl="Język"),
    "System Default": dict(uk="Як у системі", da="Systemstandard", de="Systemstandard",
                           pl="Domyślny systemowy"),
    "&Help": dict(uk="&Довідка", da="&Hjælp", de="&Hilfe", pl="Pomo&c"),
    "Support Svitlo…": dict(uk="Підтримати Svitlo…", da="Støt Svitlo…", de="Svitlo unterstützen…",
                            pl="Wesprzyj Svitlo…"),
    "Licenses…": dict(uk="Ліцензії…", da="Licenser…", de="Lizenzen…", pl="Licencje…"),
    "About Svitlo": dict(uk="Про Svitlo", da="Om Svitlo", de="Über Svitlo", pl="O Svitlo"),
    "<h3>Svitlo %1</h3><p>A free, open-source photo library and non-destructive editor.</p><p>Built with Qt %2%3.</p>": dict(
        uk="<h3>Svitlo %1</h3><p>Безкоштовна фотобібліотека та неруйнівний редактор з відкритим кодом.</p><p>Створено з Qt %2%3.</p>",
        da="<h3>Svitlo %1</h3><p>Et gratis fotobibliotek og ikke-destruktivt redigeringsprogram med åben kildekode.</p><p>Bygget med Qt %2%3.</p>",
        de="<h3>Svitlo %1</h3><p>Eine kostenlose Open-Source-Fotobibliothek mit nicht-destruktiver Bearbeitung.</p><p>Erstellt mit Qt %2%3.</p>",
        pl="<h3>Svitlo %1</h3><p>Darmowa biblioteka zdjęć i nieniszczący edytor o otwartym kodzie źródłowym.</p><p>Zbudowano przy użyciu Qt %2%3.</p>"),
    " and LibRaw": dict(uk=" і LibRaw", da=" og LibRaw", de=" und LibRaw", pl=" i LibRaw"),
    "The new language will be used after Svitlo restarts.": dict(
        uk="Нова мова застосується після перезапуску Svitlo.",
        da="Det nye sprog bruges, når Svitlo genstartes.",
        de="Die neue Sprache wird nach einem Neustart von Svitlo verwendet.",
        pl="Nowy język zostanie użyty po ponownym uruchomieniu Svitlo."),
    "Restart Now": dict(uk="Перезапустити", da="Genstart nu", de="Jetzt neu starten",
                        pl="Uruchom ponownie teraz"),
    "Later": dict(uk="Пізніше", da="Senere", de="Später", pl="Później"),
    "Import some photos first": dict(uk="Спершу імпортуйте фото", da="Importér først nogle fotos",
                                     de="Importieren Sie zuerst einige Fotos",
                                     pl="Najpierw zaimportuj zdjęcia"),
    "Import folder": dict(uk="Імпорт папки", da="Importér mappe", de="Ordner importieren",
                          pl="Importuj folder"),
    "Imported %n photo(s)": dict(
        uk=["Імпортовано %n фото"] * 3,
        da=["Importerede %n foto", "Importerede %n fotos"],
        de=["%n Foto importiert", "%n Fotos importiert"],
        pl=["Zaimportowano %n zdjęcie", "Zaimportowano %n zdjęcia", "Zaimportowano %n zdjęć"]),
    "No new photos found in %1": dict(uk="У %1 не знайдено нових фото", da="Ingen nye fotos fundet i %1",
                                      de="Keine neuen Fotos in %1 gefunden",
                                      pl="Nie znaleziono nowych zdjęć w %1"),
    "Select photos to export": dict(uk="Виберіть фото для експорту", da="Vælg fotos, der skal eksporteres",
                                    de="Fotos für den Export auswählen", pl="Wybierz zdjęcia do eksportu"),
    "Exporting…": dict(uk="Експорт…", da="Eksporterer…", de="Exportiere…", pl="Eksportowanie…"),
    "Cancel": dict(uk="Скасувати", da="Annuller", de="Abbrechen", pl="Anuluj"),
    "Export finished": dict(uk="Експорт завершено", da="Eksport fuldført", de="Export abgeschlossen",
                            pl="Eksport zakończony"),
    "Exported %1 of %2 photo(s).": dict(uk="Експортовано %1 з %2 фото.",
                                        da="Eksporterede %1 af %2 fotos.",
                                        de="%1 von %2 Fotos exportiert.",
                                        pl="Wyeksportowano zdjęcia: %1 z %2."),
    "Show Folder": dict(uk="Показати папку", da="Vis mappe", de="Ordner anzeigen", pl="Pokaż folder"),
    "Settings copied": dict(uk="Налаштування скопійовано", da="Indstillinger kopieret",
                            de="Einstellungen kopiert", pl="Skopiowano ustawienia"),
    "Settings pasted to %n photo(s)": dict(
        uk=["Налаштування вставлено в %n фото"] * 3,
        da=["Indstillinger indsat i %n foto", "Indstillinger indsat i %n fotos"],
        de=["Einstellungen auf %n Foto übertragen", "Einstellungen auf %n Fotos übertragen"],
        pl=["Wklejono ustawienia do %n zdjęcia", "Wklejono ustawienia do %n zdjęć",
            "Wklejono ustawienia do %n zdjęć"]),
    "Remove from catalog": dict(uk="Видалення з каталогу", da="Fjern fra katalog",
                                de="Aus Katalog entfernen", pl="Usuń z katalogu"),
    "Remove %n photo(s) from the catalog?\nThe files on disk are not deleted.": dict(
        uk=["Видалити %n фото з каталогу?\nФайли на диску не видаляються."] * 3,
        da=["Fjern %n foto fra kataloget?\nFilerne på disken slettes ikke.",
            "Fjern %n fotos fra kataloget?\nFilerne på disken slettes ikke."],
        de=["%n Foto aus dem Katalog entfernen?\nDie Dateien auf dem Datenträger werden nicht gelöscht.",
            "%n Fotos aus dem Katalog entfernen?\nDie Dateien auf dem Datenträger werden nicht gelöscht."],
        pl=["Usunąć %n zdjęcie z katalogu?\nPliki na dysku nie zostaną usunięte.",
            "Usunąć %n zdjęcia z katalogu?\nPliki na dysku nie zostaną usunięte.",
            "Usunąć %n zdjęć z katalogu?\nPliki na dysku nie zostaną usunięte."]),

    # --- small widgets -------------------------------------------------------------------------
    "Double-click to reset": dict(uk="Двічі клацніть, щоб скинути", da="Dobbeltklik for at nulstille",
                                  de="Zum Zurücksetzen doppelklicken", pl="Kliknij dwukrotnie, aby zresetować"),
    "Click to add a point, drag to move, double-click a point to remove it": dict(
        uk="Клацніть, щоб додати точку, перетягніть, щоб перемістити, двічі клацніть точку, щоб видалити",
        da="Klik for at tilføje et punkt, træk for at flytte, dobbeltklik på et punkt for at fjerne det",
        de="Klicken zum Hinzufügen eines Punkts, ziehen zum Verschieben, Doppelklick auf einen Punkt zum Entfernen",
        pl="Kliknij, aby dodać punkt, przeciągnij, aby przesunąć, kliknij dwukrotnie punkt, aby go usunąć"),

    # --- support dialog ----------------------------------------------------------------------------
    "Support Svitlo": dict(uk="Підтримати Svitlo", da="Støt Svitlo", de="Svitlo unterstützen",
                           pl="Wesprzyj Svitlo"),
    "<span style='font-size:18px; font-weight:600'>Enjoying Svitlo?</span>": dict(
        uk="<span style='font-size:18px; font-weight:600'>Подобається Svitlo?</span>",
        da="<span style='font-size:18px; font-weight:600'>Kan du lide Svitlo?</span>",
        de="<span style='font-size:18px; font-weight:600'>Gefällt Ihnen Svitlo?</span>",
        pl="<span style='font-size:18px; font-weight:600'>Podoba ci się Svitlo?</span>"),
    "Svitlo is free and open source, made in my spare time.<br>If it is useful to you, a small donation helps me keep improving it. Thank you! ♥": dict(
        uk="Svitlo безкоштовна програма з відкритим кодом, яку я роблю у вільний час.<br>Якщо вона вам корисна, невеликий донат допоможе мені й далі її покращувати. Дякую! ♥",
        da="Svitlo er gratis og open source, lavet i min fritid.<br>Hvis du har glæde af det, hjælper en lille donation mig med at blive ved med at forbedre det. Tak! ♥",
        de="Svitlo ist kostenlos und Open Source, entwickelt in meiner Freizeit.<br>Wenn es Ihnen nützt, hilft mir eine kleine Spende, es weiter zu verbessern. Danke! ♥",
        pl="Svitlo jest darmowe i ma otwarty kod źródłowy, tworzę je w wolnym czasie.<br>Jeśli ci się przydaje, mała darowizna pomoże mi dalej je rozwijać. Dziękuję! ♥"),
    "<b>Card or PayPal</b>": dict(uk="<b>Картка або PayPal</b>", da="<b>Kort eller PayPal</b>",
                                  de="<b>Karte oder PayPal</b>", pl="<b>Karta lub PayPal</b>"),
    "Open Ko-fi": dict(uk="Відкрити Ko-fi", da="Åbn Ko-fi", de="Ko-fi öffnen", pl="Otwórz Ko-fi"),
    "<b>Bitcoin (BTC)</b>": dict(uk="<b>Bitcoin (BTC)</b>", da="<b>Bitcoin (BTC)</b>",
                                 de="<b>Bitcoin (BTC)</b>", pl="<b>Bitcoin (BTC)</b>"),
    "Use “Copy address” to copy it exactly": dict(
        uk="Скористайтеся «Копіювати адресу», щоб скопіювати її точно",
        da="Brug “Kopiér adresse” for at kopiere den præcist",
        de="Verwenden Sie „Adresse kopieren“, um sie exakt zu kopieren",
        pl="Użyj „Kopiuj adres”, aby skopiować go dokładnie"),
    "Send only BTC on the Bitcoin network.\nOther coins sent here will be lost.": dict(
        uk="Надсилайте лише BTC у мережі Bitcoin.\nІнші монети, надіслані сюди, буде втрачено.",
        da="Send kun BTC på Bitcoin-netværket.\nAndre mønter sendt hertil går tabt.",
        de="Nur BTC im Bitcoin-Netzwerk senden.\nAndere hierher gesendete Coins gehen verloren.",
        pl="Wysyłaj tylko BTC w sieci Bitcoin.\nInne monety wysłane tutaj zostaną utracone."),
    "Copy address": dict(uk="Копіювати адресу", da="Kopiér adresse", de="Adresse kopieren", pl="Kopiuj adres"),
    "Don't show again": dict(uk="Більше не показувати", da="Vis ikke igen", de="Nicht mehr anzeigen",
                             pl="Nie pokazuj ponownie"),
    "Remind me later": dict(uk="Нагадати пізніше", da="Påmind mig senere", de="Später erinnern",
                            pl="Przypomnij później"),
    "Close": dict(uk="Закрити", da="Luk", de="Schließen", pl="Zamknij"),
    "Copied ✓": dict(uk="Скопійовано ✓", da="Kopieret ✓", de="Kopiert ✓", pl="Skopiowano ✓"),
}

PLURAL_FORMS = {"uk": 3, "pl": 3, "da": 2, "de": 2}


def fill(lang):
    path = os.path.join(HERE, f"svitlo_{lang}.ts")
    tree = ET.parse(path)
    root = tree.getroot()
    root.set("language", lang)
    missing = []
    for msg in root.iter("message"):
        src = msg.find("source").text or ""
        tr = msg.find("translation")
        entry = T.get(src, {}).get(lang)
        if entry is None:
            missing.append(src)
            continue
        for child in list(tr):
            tr.remove(child)
        tr.text = None
        tr.attrib.pop("type", None)
        if msg.get("numerus") == "yes":
            forms = entry if isinstance(entry, list) else [entry]
            assert len(forms) == PLURAL_FORMS[lang], (lang, src)
            for f in forms:
                ET.SubElement(tr, "numerusform").text = f
        else:
            assert not isinstance(entry, list), (lang, src)
            tr.text = entry
    tree.write(path, encoding="utf-8", xml_declaration=True)
    return missing


def main():
    ok = True
    for lang in LANGS:
        missing = fill(lang)
        if missing:
            ok = False
            print(f"[{lang}] {len(missing)} untranslated:")
            for m in missing:
                print("   ", repr(m))
        else:
            print(f"[{lang}] all strings translated")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
