#include <mcp_can.h> // kütüphaneyi MCP2515 kontrol ünitesiyle çalışmak için bağlama
#include <SPI.h>  // Seri Çevresel Protokol Kütüphanesi Bağlantısı

#define TIME_MOT 5000UL    //  motor sıcaklığının MediaNav ekranında görüntülenme süresi (6000UL = 6 saniye)
#define TIME_EXT 15000UL    //  dış sıcaklığın MediaNav ekranında görüntüleme süresi (6000UL = 6 saniye)
#define DELAY_DATA 10UL    //  yeni veri yazmadan önce gecikme (10UL = 6 milisaniye)
#define DELAY_DATA2 2000UL //  eski verilerin MediaNav'a kaydedilmesinden sonra ve sürekli atlama olmaması için okumaların güncellenme zamanı (2000UL = 2 saniye)

unsigned long timeData, delayData, delayData2, time3 = 0;
long unsigned int rxId, rxId2;
unsigned int data = 0x0;
unsigned int newData, engTemp, extTemp, fuelLev = 0x0;
bool refreshData = false;
bool refreshData2 = true; //  veri güncellemesine izin ver
byte flag = 1; // motor sıcaklık okumalarıyla başlayın
unsigned char len, len2 = 0;
unsigned char rxBuf[8], rxBuf2[8];
unsigned char stmp[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // sıcaklık verileri için mesaj şablonu
unsigned char stmp2[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Driving ECO2 için mesaj şablonu

MCP_CAN CAN1(5); // CS'yi 1 veriyolundan pim 5'e alacak şekilde ayarlama
MCP_CAN CAN2(10); // CS'yi 2 veriyolundan pim 10'a gönderecek şekilde ayarlama

void setup(){
     while (CAN_OK != CAN1.begin(CAN_500KBPS, MCP_8MHz)); // init can bus : baudrate = 500k / 8MHz
     while (CAN_OK != CAN2.begin(CAN_500KBPS, MCP_8MHz)); // init can bus : baudrate = 500k / 8MHz
}

void loop(){
        CAN1.readMsgBuf(&len, rxBuf); // veri okuma: len = data length, buf = data byte(s)      
        rxId = CAN1.getCanId(); // mesaj kimliği al
        if(rxId == 0x646){CAN2.sendMsgBuf(0x314, 0, 8, rxBuf); stmp2[1]=rxBuf[1]; CAN2.sendMsgBuf(0x548, 0, 8, stmp2);} // Driving ECO2
        if(rxId == 0x5DA){engTemp=rxBuf[0];}  //  Motor sıcaklığı
        if(rxId == 0x3B7){extTemp=rxBuf[0];}  //  Dış sıcaklık
        if(rxId == 0x6FB){fuelLev=(rxBuf[3]/2.0)+40;} // yakıt seviyesi
        
       // belirlenen zaman değerlerine bağlı olarak endikasyonların değiştirilmesi
       if(flag==1){
        if(timeData + TIME_MOT <= millis()) {refreshData2 = true; flag = 2; timeData=millis();}
      }
       if(flag==2){
        if(timeData + TIME_EXT <= millis()) {refreshData2 = true; flag = 1; timeData=millis();}
      }

        if(flag==1){newData=engTemp;}
        if(flag==2){newData=extTemp;}

      // Veriler değiştiyse, yenilerini göndermeden önce, okumaları sıfırlarız, her 2 saniyede bir kontrol ederiz ve sürekli olarak değil, atlamalardan kurtuluruz
      if (!refreshData &&(refreshData2 || delayData2+DELAY_DATA2 <= millis())){
        if (refreshData2){refreshData2=false;}
        delayData2=millis();
        if(newData != data){
          refreshData=true; //  yeni veri yazmaya izin ver
          stmp[2]=0xFF;
          CAN2.sendMsgBuf(0x558, 0, 8, stmp); // sıfırlamak
          delayData=millis();      
         }
   }
      if(refreshData) { // veri güncelleme izni varsa
          if(delayData+DELAY_DATA <= millis()) { // sonra genel çalışma döngüsünü durdurmadan 10 ms bekleriz
          refreshData=false;
          data=newData; // yeni veri yaz
          stmp[2]=data;
          CAN2.sendMsgBuf(0x558, 0, 8, stmp); //  MediaNav'a veri gönder.
          delayData2=millis();
          delayData=millis();
       }  
      }else{
      /*yenileri kayıtlı değilse, her 2 saniyede bir eski veriler yazın çünkü 4 saniye içinde telsiz en azından bazı veriler almazsa kısa çizgi alırız*/
     if(delayData+DELAY_DATA2 <= millis()){CAN2.sendMsgBuf(0x558, 0, 8, stmp);delayData=millis();} 
    }

     //  Driving ECO2 veri sıfırlama bölümünü düzelt
     if(time3 + 1000 < millis()){
     CAN2.readMsgBuf(&len2, rxBuf2); // veri okuma: len = data length, buf = data byte(s)      
        rxId2 = CAN2.getCanId(); // mesaj kimliği al
        if(rxId2 == 0x31C && rxBuf2[2]==0x80){
          CAN1.sendMsgBuf(0x433, 0, 8, rxBuf2);
          time3 = millis();
       }
    }
  }
