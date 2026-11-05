
/* ============================================================
 * EV Charge Simulator - Map Module
 *
 * Features:
 * - Interactive map using Leaflet.
 * - Origin and destination selection.
 * - Address search using OpenStreetMap Nominatim.
 * - Route calculation using OSRM.
 * - EV charging station search using Overpass API.
 * - Charging stations within 5 km of the route.
 * - Communication with Qt through QWebChannel.
 * - Automatic map resizing inside QStackedWidget.
 * ============================================================ */


// ------------------------------------------------------------
// Configuration
// ------------------------------------------------------------

const MAP_DEFAULT_CENTER = [-23.55, -46.63];
const MAP_DEFAULT_ZOOM = 8;

const CHARGING_STATION_RADIUS_METERS = 5000;

const NOMINATIM_URL =
    'https://nominatim.openstreetmap.org/search';

const OSRM_URL =
    'https://router.project-osrm.org/route/v1/driving/';

const OVERPASS_URL =
    'https://overpass-api.de/api/interpreter';


// ------------------------------------------------------------
// Map initialization
// ------------------------------------------------------------

const map = L.map('map', {
    zoomControl: false
}).setView(MAP_DEFAULT_CENTER, MAP_DEFAULT_ZOOM);

L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    maxZoom: 19,
    attribution: '&copy; OpenStreetMap contributors'
}).addTo(map);

L.control.zoom({
    position: 'bottomright'
}).addTo(map);


// ------------------------------------------------------------
// DOM elements
// ------------------------------------------------------------

const mapElement = document.getElementById('map');

const hintElement = document.getElementById('hint');

const routeButton = document.getElementById('routeButton');
const clearButton = document.getElementById('clearButton');

const originInput = document.getElementById('originInput');
const destinationInput = document.getElementById('destinationInput');

const searchOriginButton =
    document.getElementById('searchOriginButton');

const searchDestinationButton =
    document.getElementById('searchDestinationButton');

const originResults =
    document.getElementById('originResults');

const destinationResults =
    document.getElementById('destinationResults');

const swapButton = document.getElementById('swapButton');


if (
    !mapElement ||
    !hintElement ||
    !routeButton ||
    !clearButton ||
    !originInput ||
    !destinationInput ||
    !searchOriginButton ||
    !searchDestinationButton ||
    !originResults ||
    !destinationResults ||
    !swapButton
) {
    throw new Error(
        'Map HTML is missing required DOM elements.'
    );
}


// ------------------------------------------------------------
// State
// ------------------------------------------------------------

let origin = null;
let destination = null;

let originMarker = null;
let destinationMarker = null;
let routeLayer = null;

let chargingStationsLayer = L.layerGroup().addTo(map);
let chargingStations = [];

let bridge = null;
let bridgeReady = false;

let chooseOrigin = true;
let calculatingRoute = false;
let geocoding = false;
let loadingChargingStations = false;


// ------------------------------------------------------------
// Map resize handling
// ------------------------------------------------------------

function refreshMapSize() {
    if (!mapElement || !mapElement.isConnected) {
        return;
    }

    requestAnimationFrame(() => {
        map.invalidateSize({
            animate: false,
            pan: false
        });
    });
}

const mapResizeObserver = new ResizeObserver(() => {
    refreshMapSize();
});

mapResizeObserver.observe(mapElement);

window.addEventListener('resize', refreshMapSize);

window.addEventListener('pageshow', refreshMapSize);

window.addEventListener('load', refreshMapSize);

document.addEventListener('visibilitychange', () => {
    if (!document.hidden) {
        refreshMapSize();
    }
});

map.whenReady(() => {
    refreshMapSize();
});


// ------------------------------------------------------------
// Qt WebChannel integration
// ------------------------------------------------------------

function initializeBridge() {
    if (!window.qt || !window.QWebChannel) {
        console.error('Qt WebChannel is not available.');

        hintElement.textContent =
            'Erro: comunicação com a aplicação indisponível.';

        return;
    }

    new QWebChannel(qt.webChannelTransport, (channel) => {
        bridge = channel.objects.mapBridge;

        if (!bridge) {
            console.error(
                'mapBridge object was not registered.'
            );

            hintElement.textContent =
                'Erro ao inicializar comunicação com Qt.';

            return;
        }

        bridgeReady = true;

        console.log('Qt WebChannel connected.');

        bridge.mapReady();

        hintElement.textContent =
            'Mapa carregado. Selecione a origem.';

        // Synchronize Leaflet dimensions after the page is displayed.
        [100, 300, 600, 1000].forEach(delay => {
            setTimeout(refreshMapSize, delay);
        });
    });
}


// ------------------------------------------------------------
// Bridge helpers
// ------------------------------------------------------------

function sendToQt(method, ...args) {
    if (!bridgeReady || !bridge) {
        console.warn(
            `Cannot call ${method}: WebChannel not ready.`
        );

        return false;
    }

    if (typeof bridge[method] !== 'function') {
        console.error(
            `Qt bridge method not found: ${method}`
        );

        return false;
    }

    try {
        bridge[method](...args);
        return true;

    } catch (error) {
        console.error(
            `Qt bridge call failed (${method}):`,
            error
        );

        return false;
    }
}


// ------------------------------------------------------------
// Route cleanup
// ------------------------------------------------------------

function removeCurrentRoute() {
    if (routeLayer) {
        map.removeLayer(routeLayer);
        routeLayer = null;
    }

    clearChargingStations();

    sendToQt('clearRoute');
}


// ------------------------------------------------------------
// Map markers
// ------------------------------------------------------------

function createMarkerIcon(color) {
    return L.divIcon({
        className: '',

        html: `
            <div style="
                width: 18px;
                height: 18px;
                border: 3px solid white;
                border-radius: 50%;
                background: ${color};
                box-shadow: 0 1px 8px #000;
            "></div>
        `,

        iconSize: [24, 24],
        iconAnchor: [12, 12]
    });
}

function setOriginMarker(position) {
    if (originMarker) {
        map.removeLayer(originMarker);
    }

    originMarker = L.marker(position, {
        icon: createMarkerIcon('#20d68b')
    })
        .addTo(map)
        .bindPopup('Partida');
}

function setDestinationMarker(position) {
    if (destinationMarker) {
        map.removeLayer(destinationMarker);
    }

    destinationMarker = L.marker(position, {
        icon: createMarkerIcon('#ff5d5d')
    })
        .addTo(map)
        .bindPopup('Destino');
}


// ------------------------------------------------------------
// Charging station cleanup
// ------------------------------------------------------------

function clearChargingStations() {
    chargingStationsLayer.clearLayers();
    chargingStations = [];
}


// ------------------------------------------------------------
// Charging station utilities
// ------------------------------------------------------------

function escapeHtml(value) {
    return String(value ?? '').replace(/[&<>"']/g, char => ({
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        '"': '&quot;',
        "'": '&#039;'
    })[char]);
}


// Calculate distance from a geographic point to a route segment.
function distancePointToSegmentMeters(point, start, end) {
    const earthRadius = 6371000;
    const referenceLat = point[0] * Math.PI / 180;

    function project(coordinate) {
        const lat = coordinate[0] * Math.PI / 180;
        const lon = coordinate[1] * Math.PI / 180;

        return {
            x: earthRadius * lon * Math.cos(referenceLat),
            y: earthRadius * lat
        };
    }

    const p = project(point);
    const a = project(start);
    const b = project(end);

    const dx = b.x - a.x;
    const dy = b.y - a.y;

    const lengthSquared = dx * dx + dy * dy;

    if (lengthSquared === 0) {
        return Math.hypot(
            p.x - a.x,
            p.y - a.y
        );
    }

    const t = Math.max(
        0,
        Math.min(
            1,
            ((p.x - a.x) * dx + (p.y - a.y) * dy) /
            lengthSquared
        )
    );

    const closestX = a.x + t * dx;
    const closestY = a.y + t * dy;

    return Math.hypot(
        p.x - closestX,
        p.y - closestY
    );
}


// Find the shortest distance from a station to the route.
function distanceToRouteMeters(station, routeCoordinates) {
    let minimumDistance = Infinity;

    for (let i = 0; i < routeCoordinates.length - 1; i++) {
        const start = [
            routeCoordinates[i][1],
            routeCoordinates[i][0]
        ];

        const end = [
            routeCoordinates[i + 1][1],
            routeCoordinates[i + 1][0]
        ];

        const distance = distancePointToSegmentMeters(
            station,
            start,
            end
        );

        minimumDistance = Math.min(
            minimumDistance,
            distance
        );

        if (minimumDistance <= 10) {
            return minimumDistance;
        }
    }

    return minimumDistance;
}


// Create a bounding box around the route with a 5 km margin.
function getRouteBoundingBox(routeCoordinates) {
    const latitudes = routeCoordinates.map(point => point[1]);
    const longitudes = routeCoordinates.map(point => point[0]);

    const minLat = Math.min(...latitudes);
    const maxLat = Math.max(...latitudes);

    const minLon = Math.min(...longitudes);
    const maxLon = Math.max(...longitudes);

    const centerLat = (minLat + maxLat) / 2;

    const latMargin =
        CHARGING_STATION_RADIUS_METERS / 111320;

    const lonMargin =
        CHARGING_STATION_RADIUS_METERS /
        (
            111320 *
            Math.max(
                0.01,
                Math.cos(centerLat * Math.PI / 180)
            )
        );

    return {
        south: minLat - latMargin,
        west: minLon - lonMargin,
        north: maxLat + latMargin,
        east: maxLon + lonMargin
    };
}


// ------------------------------------------------------------
// Charging station markers
// ------------------------------------------------------------

function createChargingStationMarker(station) {
    const icon = L.divIcon({
        className: '',

        html: `
            <div style="
                width: 30px;
                height: 30px;
                display: flex;
                align-items: center;
                justify-content: center;
                background: #18263A;
                border: 2px solid #22C7B8;
                border-radius: 50%;
                color: #22C7B8;
                font-size: 17px;
                box-shadow: 0 2px 8px rgba(0,0,0,0.5);
            ">⚡</div>
        `,

        iconSize: [30, 30],
        iconAnchor: [15, 15]
    });

    const tags = station.tags || {};

    const name =
        tags.name || 'Estação de recarga';

    const operator =
        tags.operator || 'Não informado';

    const capacity =
        tags.capacity || 'Não informado';

    const openingHours =
        tags.opening_hours || 'Não informado';

    const connectors = Object.entries(tags)
        .filter(([key]) => key.startsWith('socket:'))
        .map(([key, value]) => (
            `${escapeHtml(key)}: ${escapeHtml(value)}`
        ));

    const connectorText = connectors.length > 0
        ? connectors.join('<br>')
        : 'Não informado';

    const distanceKm = (
        station.distance / 1000
    ).toFixed(2);

    const popupContent = `
        <div style="min-width: 190px;">
            <strong>${escapeHtml(name)}</strong>
            <hr>

            <b>Operadora:</b>
            ${escapeHtml(operator)}
            <br>

            <b>Quantidade:</b>
            ${escapeHtml(capacity)}
            <br>

            <b>Conectores:</b>
            <br>
            ${connectorText}
            <br>

            <b>Funcionamento:</b>
            ${escapeHtml(openingHours)}
            <br>

            <b>Distância da rota:</b>
            ${distanceKm} km
        </div>
    `;

    return L.marker(
        [station.lat, station.lon],
        { icon }
    ).bindPopup(popupContent);
}


// ------------------------------------------------------------
// Charging station search - Overpass API
// ------------------------------------------------------------

async function loadChargingStations(routeCoordinates) {
    if (loadingChargingStations) {
        return 0;
    }

    if (
        !Array.isArray(routeCoordinates) ||
        routeCoordinates.length < 2
    ) {
        return 0;
    }

    loadingChargingStations = true;

    clearChargingStations();

    hintElement.textContent =
        'Rota calculada. Procurando estações de recarga próximas...';

    try {
        const bounds = getRouteBoundingBox(routeCoordinates);

        const query = `
            [out:json][timeout:25];
            (
                nwr["amenity"="charging_station"]
                (${bounds.south},${bounds.west},${bounds.north},${bounds.east});
            );
            out center tags;
        `;

        const response = await fetch(
            OVERPASS_URL,
            {
                method: 'POST',

                headers: {
                    'Content-Type':
                        'application/x-www-form-urlencoded'
                },

                body: new URLSearchParams({
                    data: query
                }).toString()
            }
        );

        if (!response.ok) {
            throw new Error(
                `Overpass HTTP ${response.status}`
            );
        }

        const data = await response.json();

        if (!Array.isArray(data.elements)) {
            throw new Error(
                'Resposta inválida da Overpass API.'
            );
        }

        const stations = [];

        for (const element of data.elements) {
            const lat =
                element.lat ?? element.center?.lat;

            const lon =
                element.lon ?? element.center?.lon;

            if (
                !Number.isFinite(lat) ||
                !Number.isFinite(lon)
            ) {
                continue;
            }

            const distance = distanceToRouteMeters(
                [lat, lon],
                routeCoordinates
            );

            if (
                distance <= CHARGING_STATION_RADIUS_METERS
            ) {
                stations.push({
                    lat,
                    lon,
                    tags: element.tags || {},
                    distance
                });
            }
        }

        // Remove duplicate coordinates.
        const uniqueStations = [];
        const seen = new Set();

        for (const station of stations) {
            const key =
                `${station.lat.toFixed(5)},` +
                `${station.lon.toFixed(5)}`;

            if (!seen.has(key)) {
                seen.add(key);
                uniqueStations.push(station);
            }
        }

        chargingStations = uniqueStations;

        for (const station of chargingStations) {
            createChargingStationMarker(station)
                .addTo(chargingStationsLayer);
        }

        console.log(
            `[Charging Stations] ${chargingStations.length} stations found.`
        );

        return chargingStations.length;

    } catch (error) {
        console.error(
            'Charging station search failed:',
            error
        );

        hintElement.textContent =
            'Rota calculada, mas não foi possível carregar as estações.';

        return 0;

    } finally {
        loadingChargingStations = false;
    }
}


// ------------------------------------------------------------
// Origin and destination selection
// ------------------------------------------------------------

function selectOrigin(position, address = '') {
    if (calculatingRoute || loadingChargingStations) {
        return;
    }

    origin = position;

    setOriginMarker(position);

    if (destinationMarker) {
        map.removeLayer(destinationMarker);
        destinationMarker = null;
    }

    destination = null;

    destinationInput.value = '';

    destinationResults.innerHTML = '';
    destinationResults.style.display = 'none';

    removeCurrentRoute();

    chooseOrigin = false;

    if (address) {
        originInput.value = address;
    }

    map.setView(position, 15);

    hintElement.textContent =
        'Origem definida. Informe ou selecione o destino.';
}


function selectDestination(position, address = '') {
    if (calculatingRoute || loadingChargingStations) {
        return;
    }

    if (!origin) {
        hintElement.textContent =
            'Defina a origem antes de selecionar o destino.';

        return;
    }

    removeCurrentRoute();

    destination = position;

    setDestinationMarker(position);

    chooseOrigin = true;

    if (address) {
        destinationInput.value = address;
    }

    map.setView(position, 15);

    hintElement.textContent =
        'Origem e destino definidos. Clique em Calcular rota.';
}


// ------------------------------------------------------------
// Address search - Nominatim
// ------------------------------------------------------------

async function searchAddress(
    query,
    resultsElement,
    onSelect
) {
    const address = query.trim();

    if (!address) {
        hintElement.textContent =
            'Digite um endereço para pesquisar.';

        return;
    }

    if (geocoding) {
        hintElement.textContent =
            'Aguarde a conclusão da pesquisa anterior.';

        return;
    }

    geocoding = true;

    searchOriginButton.disabled = true;
    searchDestinationButton.disabled = true;

    resultsElement.innerHTML = '';
    resultsElement.style.display = 'none';

    hintElement.textContent = 'Pesquisando endereço...';

    try {
        const params = new URLSearchParams({
            q: address,
            format: 'jsonv2',
            limit: '5',
            addressdetails: '1'
        });

        const response = await fetch(
            `${NOMINATIM_URL}?${params.toString()}`,
            {
                headers: {
                    'Accept': 'application/json'
                }
            }
        );

        if (!response.ok) {
            throw new Error(
                `HTTP ${response.status}`
            );
        }

        const results = await response.json();

        if (
            !Array.isArray(results) ||
            results.length === 0
        ) {
            hintElement.textContent =
                'Nenhum endereço encontrado. Tente informar mais detalhes.';

            return;
        }

        results.forEach(result => {
            const button =
                document.createElement('button');

            button.type = 'button';
            button.className = 'search-result';

            button.textContent = result.display_name;

            button.addEventListener('click', () => {
                const latitude = Number(result.lat);
                const longitude = Number(result.lon);

                if (
                    !Number.isFinite(latitude) ||
                    !Number.isFinite(longitude)
                ) {
                    hintElement.textContent =
                        'Coordenadas inválidas para o endereço selecionado.';

                    return;
                }

                onSelect(
                    [latitude, longitude],
                    result.display_name
                );

                resultsElement.innerHTML = '';
                resultsElement.style.display = 'none';
            });

            resultsElement.appendChild(button);
        });

        resultsElement.style.display = 'block';

        hintElement.textContent =
            'Selecione o endereço correto na lista.';

    } catch (error) {
        console.error(
            'Address search failed:',
            error
        );

        hintElement.textContent =
            `Falha ao pesquisar endereço: ${error.message}`;

    } finally {
        geocoding = false;

        searchOriginButton.disabled = false;
        searchDestinationButton.disabled = false;
    }
}


// ------------------------------------------------------------
// Address search events
// ------------------------------------------------------------

searchOriginButton.addEventListener('click', () => {
    searchAddress(
        originInput.value,
        originResults,
        selectOrigin
    );
});

searchDestinationButton.addEventListener('click', () => {
    searchAddress(
        destinationInput.value,
        destinationResults,
        selectDestination
    );
});

originInput.addEventListener('keydown', event => {
    if (event.key === 'Enter') {
        event.preventDefault();
        searchOriginButton.click();
    }
});

destinationInput.addEventListener('keydown', event => {
    if (event.key === 'Enter') {
        event.preventDefault();
        searchDestinationButton.click();
    }
});


// ------------------------------------------------------------
// Swap origin and destination
// ------------------------------------------------------------

swapButton.addEventListener('click', () => {
    if (
        calculatingRoute ||
        geocoding ||
        loadingChargingStations
    ) {
        return;
    }

    if (!origin || !destination) {
        hintElement.textContent =
            'Defina origem e destino antes de inverter.';

        return;
    }

    const previousOrigin = origin;
    const previousDestination = destination;

    const previousOriginAddress = originInput.value;
    const previousDestinationAddress = destinationInput.value;

    removeCurrentRoute();

    origin = previousDestination;
    destination = previousOrigin;

    originInput.value = previousDestinationAddress;
    destinationInput.value = previousOriginAddress;

    setOriginMarker(origin);
    setDestinationMarker(destination);

    chooseOrigin = true;

    const bounds = L.latLngBounds([
        origin,
        destination
    ]);

    map.fitBounds(bounds, {
        padding: [40, 40]
    });

    hintElement.textContent =
        'Origem e destino invertidos. Calcule a nova rota.';
});


// ------------------------------------------------------------
// Map interaction
// ------------------------------------------------------------

map.on('click', event => {
    if (
        calculatingRoute ||
        geocoding ||
        loadingChargingStations
    ) {
        return;
    }

    const position = [
        event.latlng.lat,
        event.latlng.lng
    ];

    if (chooseOrigin || !origin) {
        selectOrigin(position);
    } else {
        selectDestination(position);
    }

    sendToQt(
        'coordinatesSelected',
        position[0],
        position[1]
    );
});


// ------------------------------------------------------------
// Route calculation - OSRM
// ------------------------------------------------------------

async function calculateRoute() {
    if (
        calculatingRoute ||
        geocoding ||
        loadingChargingStations
    ) {
        return;
    }

    if (!origin || !destination) {
        hintElement.textContent =
            'Defina origem e destino primeiro.';

        return;
    }

    if (!bridgeReady) {
        hintElement.textContent =
            'Aguarde a conexão com a aplicação.';

        console.error('WebChannel is not ready.');

        return;
    }

    const [originLat, originLon] = origin;
    const [destinationLat, destinationLon] = destination;

    const routeUrl =
        `${OSRM_URL}` +
        `${originLon},${originLat};` +
        `${destinationLon},${destinationLat}` +
        '?overview=full&geometries=geojson';

    calculatingRoute = true;
    routeButton.disabled = true;

    // Remove previous route and stations.
    removeCurrentRoute();

    hintElement.textContent = 'Calculando rota...';

    sendToQt(
        'routeRequested',
        originLat,
        originLon,
        destinationLat,
        destinationLon
    );

    try {
        const response = await fetch(routeUrl);

        if (!response.ok) {
            throw new Error(
                `HTTP ${response.status}`
            );
        }

        const data = await response.json();

        if (
            data.code !== 'Ok' ||
            !Array.isArray(data.routes) ||
            data.routes.length === 0
        ) {
            throw new Error(
                data.message || 'Nenhuma rota encontrada.'
            );
        }

        const route = data.routes[0];

        const distanceKm =
            route.distance / 1000;

        const durationMinutes =
            route.duration / 60;

        if (
            !Number.isFinite(distanceKm) ||
            !Number.isFinite(durationMinutes) ||
            distanceKm < 0 ||
            durationMinutes < 0 ||
            !route.geometry ||
            !Array.isArray(route.geometry.coordinates)
        ) {
            throw new Error(
                'Resposta inválida do serviço de rotas.'
            );
        }

        console.log('OSRM route:', {
            distanceKm,
            durationMinutes
        });

        // Send route information to Qt.
        const sent = sendToQt(
            'routeCalculated',
            distanceKm,
            durationMinutes
        );

        if (!sent) {
            throw new Error(
                'Não foi possível enviar os dados da rota ao Qt.'
            );
        }

        // Draw route.
        routeLayer = L.geoJSON(route.geometry, {
            style: {
                color: '#078ff0',
                weight: 6,
                opacity: 0.95
            }
        }).addTo(map);

        map.fitBounds(routeLayer.getBounds(), {
            padding: [40, 40]
        });

        // Release route calculation lock before station lookup.
        calculatingRoute = false;

        const stationCount = await loadChargingStations(
            route.geometry.coordinates
        );

        hintElement.textContent =
            `Rota: ${distanceKm.toFixed(1)} km · ` +
            `${Math.round(durationMinutes)} min (estimativa) · ` +
            `${stationCount} estações próximas`;

    } catch (error) {
        console.error(
            'Route calculation failed:',
            error
        );

        hintElement.textContent =
            `Falha ao calcular rota: ${error.message}`;

    } finally {
        calculatingRoute = false;
        routeButton.disabled = false;
    }
}

routeButton.addEventListener('click', calculateRoute);


// ------------------------------------------------------------
// Clear map
// ------------------------------------------------------------

function clearMap() {
    if (
        calculatingRoute ||
        geocoding ||
        loadingChargingStations
    ) {
        return;
    }

    if (originMarker) {
        map.removeLayer(originMarker);
    }

    if (destinationMarker) {
        map.removeLayer(destinationMarker);
    }

    if (routeLayer) {
        map.removeLayer(routeLayer);
    }

    origin = null;
    destination = null;

    originMarker = null;
    destinationMarker = null;
    routeLayer = null;

    clearChargingStations();

    chooseOrigin = true;

    // Clear address fields.
    originInput.value = '';
    destinationInput.value = '';

    // Clear search results.
    originResults.innerHTML = '';
    destinationResults.innerHTML = '';

    originResults.style.display = 'none';
    destinationResults.style.display = 'none';

    sendToQt('clearRoute');

    hintElement.textContent =
        'Pontos removidos. Selecione uma nova origem.';

    map.setView(
        MAP_DEFAULT_CENTER,
        MAP_DEFAULT_ZOOM
    );
}

clearButton.addEventListener('click', clearMap);


// ------------------------------------------------------------
// Initialization
// ------------------------------------------------------------

initializeBridge();

console.log('[Map] Initialization completed.');
