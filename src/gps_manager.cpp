#include "gps_manager.h"
static bool isLeap(int y) {
  return (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
}

static int daysInMonth(int y, int m) {
  static const int d[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (m == 2) return d[m-1] + (isLeap(y) ? 1 : 0);
  return d[m-1];
}

// Día de la semana: 0=Domingo..6=Sábado (Zeller simplificado)
static int dayOfWeek(int y, int m, int d) {
  if (m < 3) { m += 12; y -= 1; }
  int K = y % 100;
  int J = y / 100;
  int h = (d + (13*(m + 1))/5 + K + K/4 + J/4 + 5*J) % 7;
  // h: 0=Sábado,1=Domingo,...6=Viernes
  int dow = (h + 6) % 7; // -> 0=Domingo..6=Sábado
  return dow;
}

static int lastSundayOfMonth(int y, int m) {
  int dim = daysInMonth(y, m);
  int dow = dayOfWeek(y, m, dim); // 0=Domingo
  return dim - dow; // si dim es domingo dow=0 => dim
}

// DST Europa (incluye España):
// - Empieza: último domingo de marzo a las 01:00 UTC
// - Termina: último domingo de octubre a las 01:00 UTC
static bool isDST_EuropeMadrid_UTC(int y, int mo, int d, int h, int mi, int s) {
  int startDay = lastSundayOfMonth(y, 3);
  int endDay   = lastSundayOfMonth(y, 10);

  // Antes de marzo o después de octubre => no DST
  if (mo < 3 || mo > 10) return false;
  // Entre abril y septiembre => DST
  if (mo > 3 && mo < 10) return true;

  // Marzo: DST a partir del último domingo 01:00 UTC
  if (mo == 3) {
    if (d > startDay) return true;
    if (d < startDay) return false;
    // d == startDay
    if (h > 1) return true;
    if (h < 1) return false;
    // h==1: a partir de 01:00:00
    return (mi > 0 || s >= 0);
  }

  // Octubre: DST hasta el último domingo 01:00 UTC (excluido)
  if (mo == 10) {
    if (d < endDay) return true;
    if (d > endDay) return false;
    // d == endDay
    if (h < 1) return true;
    if (h > 1) return false;
    // h==1: en 01:00 UTC ya cambió a estándar => no DST
    return false;
  }

  return false;
}

// Suma horas a una fecha/hora simple (maneja día/mes/año)
static void addHours(int &y, int &mo, int &d, int &h, int hoursToAdd) {
  h += hoursToAdd;
  while (h >= 24) { h -= 24; d += 1; }
  while (h < 0)   { h += 24; d -= 1; }

  // Normaliza día hacia delante
  while (d > daysInMonth(y, mo)) {
    d -= daysInMonth(y, mo);
    mo += 1;
    if (mo > 12) { mo = 1; y += 1; }
  }
  // Normaliza día hacia atrás (por si restaras, no es tu caso normalmente)
  while (d < 1) {
    mo -= 1;
    if (mo < 1) { mo = 12; y -= 1; }
    d += daysInMonth(y, mo);
  }
}
//aqui acaba

GpsManager gpsManager;

void GpsManager::begin(int rxPin, int txPin, uint32_t baud) {
    Serial1.begin(baud, SERIAL_8N1, rxPin, txPin);
}

void GpsManager::update() {
    // Leer GPS UNA vez
    while (Serial1.available()) {
        char c = Serial1.read();
        gps.encode(c);

        // DEBUG NMEA crudo (descomenta solo si lo necesitas)
        // Serial.write(c);
    }

    // timestamp local (segundos desde encendido) si lo sigues usando
    fix.ts = millis() / 1000;

    // Posición
    if (gps.location.isValid() && gps.location.age() < 2000) {
        fix.valid = true;
        fix.lat = gps.location.lat();
        fix.lon = gps.location.lng();
    } else {
        fix.valid = false;
    }

    // Fecha + hora local España (Europe/Madrid) desde UTC GPS
    if (gps.date.isValid() && gps.time.isValid()) {
        int y  = gps.date.year();
        int mo = gps.date.month();
        int d  = gps.date.day();
        int h  = gps.time.hour();
        int mi = gps.time.minute();
        int s  = gps.time.second();

        bool dst = isDST_EuropeMadrid_UTC(y, mo, d, h, mi, s);
        int offset = dst ? 2 : 1; // CEST +2, CET +1
        addHours(y, mo, d, h, offset);

        fix.timeValid = true;

      
        fix.year  = (uint16_t)y;
        fix.month = (uint8_t)mo;
        fix.day   = (uint8_t)d;

        // ✅ GUARDAR HORA
        fix.hour   = (uint8_t)h;
        fix.minute = (uint8_t)mi;
        fix.second = (uint8_t)s;
    } else {
        fix.timeValid = false;
        // NO borres year/month/day aquí para evitar que vuelva a 0000-00-00
    }
}

int GpsManager::sats() {
    return gps.satellites.isValid() ? gps.satellites.value() : -1;
}

double GpsManager::hdop() {
    return gps.hdop.isValid() ? gps.hdop.hdop() : -1.0;
}




bool GpsManager::locValid() const {
    return gps.location.isValid();
}

uint32_t GpsManager::locAge() {
    return gps.location.age();
}

