# Interaktywna Mapa Lini Kolejowych
**EN: Interactive Map of Railway Lines**  
Client-Server application using [Leaflet](https://leafletjs.com/)
to visualize rail data downloaded from [OpenStreetMap](https://www.openstreetmap.org/) servers.

![screenshot](https://github.com/Kacperos155/Mapa-Lini-Kolejowych/2022-01-23.png)

## Using and Building
### Using
Ready to work version with rail data for Poland is ready to download in Realeases.
Executable will host website on localhost:80

### Flags  
Executable .exe file support this flags:
- "--rebuild-database" - Creating new database by downloading data for provided country from OverSteetMap by [OverpassAPI](http://overpass-api.de/)
 or by providing JSON file with data compatible with OverpassAPI [script](https://github.com/Kacperos155/Mapa-Lini-Kolejowych/blob/master/Mapa%20lini%20kolejowych/Overpass%20data/Overpass%20query.ql)
- "--force-console" - Turning off GUI and forcing interaction by console
- "--translate" - Translating files mentioned in [Website\languages.json](https://github.com/Kacperos155/Mapa-Lini-Kolejowych/blob/master/Mapa%20lini%20kolejowych/Website/languages.json)

### Manual Building
This repository has one git submodule and is using **Visual Studio** project files and depends on **vcpkg packages** (x64-windows-static).

## Dependencies
### C++:
- [Drogon](https://github.com/drogonframework/drogon) - Fast async HTTP Framework
- [SQLiteCpp](https://github.com/SRombauts/SQLiteCpp) - Easy to use C++ wrapper for SQLite3
- [Spatalite](https://www.gaia-gis.it/fossil/libspatialite/index) (included in repository) - SQLite3 extension for GIS
- [FMT](https://github.com/fmtlib/fmt) - Very useful string formating
- [CPR](https://github.com/libcpr/cpr) - C++ Curl wrapper
- [tinyfiledialogs](https://github.com/native-toolkit/tinyfiledialogs) - Native GUI dialogs eg. open file dialog
### JavaScript:
- [Leaflet](https://leafletjs.com/) - Open-source interactive map
- [Leaflet.Legend](https://github.com/ptma/Leaflet.Legend) (submodule) - Leaflet plugin for easy creation of map legend