#pragma once
#include <Arduino.h>

class Uploader {
public:
  bool uploadCSV();   // <- declaración
};

extern Uploader uploader;