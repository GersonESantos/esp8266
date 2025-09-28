////////////////////////////////////////////////////////////////////////////////////////////////////
#define PINO_SENSOR_SOLO A0  // Variáveis e configuração do sensor de umidade do solo

int    leituraSolo = 0;
float  tensaoSolo  = 0.0;
float  nivelSolo   = 0.0;
String classific   = "";

////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  pinMode(PINO_SENSOR_SOLO, INPUT);
  Serial.begin(115200);  // Inicializa a comunicação serial
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  // Leitura do sensor de umidade do solo
  leituraSolo = analogRead(PINO_SENSOR_SOLO);
  tensaoSolo  = leituraSolo * (3.3 / 4095.0);
  nivelSolo   = (leituraSolo / 4095.0) * 100.0;

  // Classificação da umidade do solo com base no nível percentual
  if (nivelSolo <= 5)         { classific = "⚠️ Solo seco extremo - risco de morte vegetal!";
  } else if (nivelSolo <= 20) { classific = "🔴 Solo muito seco - irrigação urgente!";
  } else if (nivelSolo <= 40) { classific = "🟠 Solo seco - precisa de água.";
  } else if (nivelSolo <= 60) { classific = "🟡 Solo moderado - pode irrigar logo.";
  } else if (nivelSolo <= 80) { classific = "🟢 Solo ideal - umidade equilibrada.";
  } else if (nivelSolo <= 95) { classific = "🔵 Solo úmido - sem irrigação necessária.";
  } else                      { classific = "💧  Solo encharcado - perigo de fungos!"; }

  // Exibição no monitor serial
  Serial.println("--------------------------------------------------");
  Serial.print("Leitura do sensor: "); Serial.println(leituraSolo);
  Serial.print("Tensão no sensor:  "); Serial.print(tensaoSolo, 2); Serial.println(" V");
  Serial.print("Nível do solo:     "); Serial.print(nivelSolo, 2);  Serial.println(" %");
  Serial.print("Classificação:     "); Serial.println(classific);

  delay(1000);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
