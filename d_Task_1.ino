




void Task1code(void *pvParameters) {
  unsigned long lastEthCheckTime = 0;
  unsigned long lastToggleTime13 = 0;
  unsigned long lastToggleTime12 = 0;
  unsigned long lastToggleTime15 = 0;
  unsigned long lastToggleTime18 = 0;

  // Define os intervalos em milissegundos para cada frequência
  const unsigned long interval13 = 1000 / (5 * 2);  // 5 Hz
  const unsigned long interval12 = 1000 / (10 * 2); // 10 Hz
  const unsigned long interval15 = 1000 / (2 * 2);  // 2 Hz
  const unsigned long interval18 = 1000 / (1 * 2);  // 1 Hz

  // Lista de endereços I2C anteriores
  byte previousAddresses[6] = {0}; // Armazena os endereços I2C da última varredura
  int numPreviousAddresses = 0;

  unsigned int read_value = 0;        // Sets/reset the variable for reading DAC to 0
  unsigned int vref = 4095;           // Sets vref value for calculations (set to 2500 for internal reference)
  float voltage = 0;

  while (1) {
    byte error, address;
    int nDevices = 0;  // Reseta o número de dispositivos detectados a cada iteração
    vTaskDelay(5); // Delay de 5 milissegundos

    mb.task();  // Executa a tarefa Modbus

    // Atualiza o valor do registrador holding (por exemplo, o valor do servo)
    mb.Hreg(ServoHreg, 55);

    // Atualiza os estados dos optoacopladores nos registradores de status (discretos)
    for (int i = 0; i < 3; i++) {
      mb.Ists(i, digitalRead(vetotIN[i]));
    }

    // Piscar os LEDs em frequências diferentes
    if (millis() - lastToggleTime13 >= interval13) {
      lastToggleTime13 = millis();
      digitalWrite(13, !digitalRead(13)); // Alterna o estado do pino 13
      //  int value = mb.Hreg(16);  // Lê o valor do registrador Hreg(16)

      // Variáveis para armazenar cada bit
      // Usa bitRead para separar os bits
      // bit0 = bitRead(value, 0);  // Lê o bit 0
      //bit1 = bitRead(value, 1);  // Lê o bit 1
      // bit2 = bitRead(value, 2);  // Lê o bit 2
      // bit3 = bitRead(value, 3);  // Lê o bit 3
      // bit4 = bitRead(value, 4);  // Lê o bit 4

    }
    if (millis() - lastToggleTime12 >= interval12) {
      lastToggleTime12 = millis();
      digitalWrite(12, !digitalRead(12)); // Alterna o estado do pino 12



    }
    if (millis() - lastToggleTime15 >= interval15) {
      lastToggleTime15 = millis();
      digitalWrite(15, !digitalRead(15)); // Alterna o estado do pino 15
    }
    if (millis() - lastToggleTime18 >= interval18) {
      lastToggleTime18 = millis();
      digitalWrite(18, !digitalRead(18)); // Alterna o estado do pino 18

    }

    // Verifica a leitura de um coil
    if (millis() > ts + 1000) {
      ts = millis();
      int value = mb.Hreg(16);  // Lê o valor do registrador Hreg(16)

      // Variáveis para armazenar cada bit
      // Usa bitRead para separar os bits
      //bit0 = bitRead(value, 0);  // Lê o bit 0
      //bit1 = bitRead(value, 1);  // Lê o bit 1
      //bit2 = bitRead(value, 2);  // Lê o bit 2
      // bit3 = bitRead(value, 3);  // Lê o bit 3
      // bit4 = bitRead(value, 4);  // Lê o bit 4

      // Agora, você pode usar as variáveis bit0, bit1, bit2, bit3, e bit4 como precisar
      Serial.print("i2c: "); Serial.println(bit0);
      Serial.print("eth: "); Serial.println(bit1);
      Serial.print("EXP9x7: "); Serial.println(bit2);
      Serial.print("analog: "); Serial.println(bit3);
      Serial.print("reserva: "); Serial.println(bit4);
      // //
      Serial.println();


      if (bit0 == 1) {
        double Irms = emon1.calcIrms(370 * 2); // Calcula o Irms
        uint16_t currentValue = static_cast<uint16_t>(Irms * 100); // Converte o valor de corrente
        mb.Hreg(8, currentValue); // Atualiza o valor do registrador com a corrente

        byte currentAddresses[6] = {0}; // Armazena os endereços I2C da varredura atual
        int numCurrentAddresses = 0;

        for (address = 0x01; address < 0x7f; address++) {
          Wire.beginTransmission(address);
          error = Wire.endTransmission();

          if (error == 0) {
            // Armazena os endereços I2C atuais
            currentAddresses[numCurrentAddresses] = address;
            numCurrentAddresses++;

            // Armazena o endereço I2C encontrado no registrador Modbus
            mb.Hreg(9 + nDevices, address);
            nDevices++;
            if (nDevices >= 6) break; // Limita a quantidade de dispositivos

          } else if (error != 2) {
            Serial.printf("Error %d at address 0x%02X\n", error, address);
          }
        }

        // Limpa os registradores de dispositivos desconectados
        for (int i = 0; i < 6; i++) {
          bool found = false;
          for (int j = 0; j < numCurrentAddresses; j++) {
            if (previousAddresses[i] == currentAddresses[j]) {
              found = true;
              break;
            }
          }
          if (!found && previousAddresses[i] != 0) {
            mb.Hreg(9 + i, 0); // Limpa o registrador se o endereço anterior não foi encontrado
          }
        }

        // Atualiza a lista de endereços anteriores
        for (int i = 0; i < numCurrentAddresses; i++) {
          previousAddresses[i] = currentAddresses[i];
        }
        numPreviousAddresses = numCurrentAddresses;

        // Limpa os registradores restantes, se necessário
        for (int i = numCurrentAddresses; i < 6; i++) {
          mb.Hreg(9 + i, 0); // Limpa os registradores não usados
        }

        DateTime now = rtc.now();
        mb.Hreg(1, now.day());    // Dia
        mb.Hreg(2, now.month());  // Mês
        mb.Hreg(3, now.year());   // Ano
        mb.Hreg(4, now.hour());   // Hora
        mb.Hreg(5, now.minute()); // Minuto
        mb.Hreg(6, now.second()); // Segundo
      }
      //if (bit0 == 0) {
       // for (int i = 0; i < 15; i++) {
        //  mb.Hreg(i, 0); // Limpa os registradores
        //}
     // }

    }


    if (bit2 == 1) {


      unsigned long currentMillis = millis();



      // Controle do período de ligar os relés
      if (relesOn && !delayPeriod) {
        if (currentMillis - previousMillisRelesOn >= intervalRelesOn) {
          previousMillisRelesOn = currentMillis;

          gpio.digitalWrite(currentRele, 1); // Relé On

          currentRele++;
          if (currentRele > 15) {
            currentRele = 9;
            relesOn = false;
            delayPeriod = true;
            delayStart = currentMillis;
          }
        }
      }

      // Período de espera após ligar todos os relés
      if (delayPeriod && (currentMillis - delayStart >= delayInterval)) {
        delayPeriod = false;
      }

      // Controle do período de desligar os relés
      if (!relesOn && !delayPeriod) {
        if (currentMillis - previousMillisRelesOff >= intervalRelesOff) {
          previousMillisRelesOff = currentMillis;

          gpio.digitalWrite(currentRele, 0); // Relé Off

          currentRele++;
          if (currentRele > 15) {
            currentRele = 9;
            relesOn = true;
            delayPeriod = true;
            delayStart = currentMillis;
          }
        }
      }


      if (millis() > temp + 300) {
        temp = millis();

        for (int i = 0; i < 9; i++) {

          // Atualiza o registrador de status de entrada (3 a 11)
          mb.Ists(i + 3, gpio.digitalRead(i));
        }
      }

    }


    if (millis() > temp2 + 300) {
      if (bit3 == 1) {
        temp2 = millis();
        for (int x = 0; x < 8; x++) {                   // Loops from 0 to 7

          read_value = adc.read(x, SD);                 // Read value of ADC channel x (0 to 7) in Single-ended mode
          voltage = read_value * (vref / 4095.0);       // Calculate voltage output value according to the voltage reference and resolution

          // Prints...
          read_value = read_value  + x * 30;
          mb.Hreg(17 + x, read_value);                  // Atribui o valor lido ao registrador Hreg correspondente (17 a 24)


        }

      }
     // if (bit3 == 0) {
     //   for (int i = 0; i < 8; i++) {
       //   mb.Hreg(i+17, 0); // Limpa os registradores
      //  }
     // }

    }
  }
}
