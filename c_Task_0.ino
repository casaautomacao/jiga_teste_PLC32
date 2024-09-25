int httpCode;
String payload;


void Task0code(void * pvParameters) {

  while (1) {


    vTaskDelay(10); // Delay de 20 milissegundos
    //String enviar = "https://script.google.com/macros/s/AKfycbwCepQyW383Ci1pZSQYvxIuX9RVRd8ae8hXvqjo49023DIb6cRcH83tORffdYeDStHu/exec";



    if (eth_connected ) {
      String enviar = "https://script.google.com/macros/s/AKfycbwCepQyW383Ci1pZSQYvxIuX9RVRd8ae8hXvqjo49023DIb6cRcH83tORffdYeDStHu/exec";
      //113&value2=125&value3=55


      enviar += "?value1=";
      enviar += String(ETH.localIP()[0]);
      enviar += ".";
      enviar += String(ETH.localIP()[1]);
      enviar += ".";
      enviar += String(ETH.localIP()[2]);
      enviar += ".";
      enviar += String(ETH.localIP()[3]);


      enviar += "&value2=";
      enviar += String(ETH.macAddress());
      enviar += "&value3=PLC32";


      if (bit1 == 0) {
        mb.Hreg(15, 0);
      }
      if (bit1 == 1) {
        enviarMedicao(enviar);


        mb.Hreg(15, httpCode);

      }

    } else {
      // Se a ETH estiver desconectada, envia um código de erro e payload vazio
      mb.Hreg(15, 0);  // Envia código 0 (ETH desconectada)

    }



    if (millis() > temp3 + 200) {
      temp3 = millis();
      int value = mb.Hreg(16);  // Lê o valor das chaves registrador Hreg(16)

      // Variáveis para armazenar cada bit
      // Usa bitRead para separar os bits
      bit0 = bitRead(value, 0);  // Lê o bit 0
      bit1 = bitRead(value, 1);  // Lê o bit 1
      bit2 = bitRead(value, 2);  // Lê o bit 2
      bit3 = bitRead(value, 3);  // Lê o bit 3
      bit4 = bitRead(value, 4);  // Lê o bit 4
    }

  }

}


void enviarMedicao(String url)
{
  HTTPClient http;


  //url = String("https://script.google.com") + "/macros/s/" + GScriptId + "/exec?" + "value1=" + tensaoFinal + "&value2=" + String(correnteFinal, 2) + "&value3=" + tempoEnvio;


  Serial.print("Making a request");
  http.begin(url.c_str()); //Specify the URL and certificate
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  httpCode = http.GET();

  if (httpCode > 0) { //Check for the returning code
    payload = http.getString();

    Serial.println(httpCode);
    Serial.println(payload);
    //     testdrawstyles(payload);
    //if (httpCode == 200 or httpCode == 201) tempPing.Saida(0);
  }
  else {
    Serial.println("Error on HTTP request");
    httpCode = 0; // Define o código de erro como 0
    payload = ""; // Define payload vazio em caso de erro

  }


  // Envia o payload para o registrador Modbus 16 (limitado a um valor numérico)


  http.end();

}
