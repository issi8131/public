#include <HTTPClient.h>
#include <M5StickC.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "imgs.c" //ここで作成した画像データを読み込み

//ユーザで設定する定数
const char* MY_SSID     = "XXXX";// your network SSID (name of wifi network)
const char* MY_PASSWORD = "XXXX";// your network password
const String CITY_CODE = "270000";//天気を取得する地域コード(270000は大阪全域)
const unsigned int UPDATE_INTERVAL = 3600;//天気情報更新間隔[s]

//定数
const byte SUNNY = 0;
const byte CLOUDY = 1;
const byte RAINY = 2;
const int LOOP_DELAY = 500;//1~1000[ms]

//グローバル変数
byte gAMWeather;//午前の天気
byte gPMWeather;//午後の天気
String gDate;//画面に表示する日付文字列
String gMaxTemp;//最高気温
String gMinTemp;//最低気温
byte gDaySelector;//予報対象日(0:今日、1:明日、2:明後日)
unsigned int gLoopCounter;//電源投入時を基準に1ループごとにカウントアップ

void updateWeather() { //Webから天気を取ってgAMWeather,gPMWeatherを更新
  //HTTP通信が失敗する場合に備えてフェイルセーフで変数を更新
  gAMWeather = RAINY;
  gPMWeather = RAINY;
  gDate = "NO DATA";

  String url = "http://weather.livedoor.com/forecast/webservice/json/v1?city=" + CITY_CODE;
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();//GETメソッドで接続
  if (httpCode == HTTP_CODE_OK) {//正常にGETができたら
    String payload = http.getString();
    payload.replace("\\", "¥"); //JSONレスポンス内の"\"をエスケープしないとデシリアライズが失敗する
    DynamicJsonDocument jsonDoc(10000);//レスポンスデータのJSONオブジェクトを格納する領域を確保(レスポンスデータの想定最大サイズに合わせて大きめ)
    deserializeJson(jsonDoc, payload); //HTTPのレスポンス文字列をJSONオブジェクトに変換
    JsonVariant jvToday = jsonDoc["forecasts"][gDaySelector];//forecastsプロパティの0番目の要素が今日の天気に関するプロパティ
    gDate = jvToday["date"].as<String>();//jvToday内のdateプロパティから日付文字列を取得
    //天気情報の更新
    String telop = jvToday["telop"].as<String>();//予報文字列を取得("晴"、"曇り"、"曇のち雨"等)、ただしUnicode形式
    int isFollow = telop.indexOf("¥u306e¥u3061");//予報文字列内の"のち"の位置を取得(なければ-1)
    if (isFollow < 0) { //"のち"がないなら午前も午後も同じ天気にする
      gAMWeather = gPMWeather = decodeStr2Weather(telop);
    }
    else { //"のち"があるなら前後の文字列をそれぞれ午前・午後の天気にする
      telop.replace("¥u306e¥u3061", ""); //"のち"を除去
      gAMWeather = decodeStr2Weather(telop.substring(0, isFollow));
      gPMWeather = decodeStr2Weather(telop.substring(isFollow, telop.length()));
    }
    //気温情報の更新
    JsonVariant jvMaxTemp = jvToday["temperature"]["max"];//最高気温のプロパティを抽出
    JsonVariant jvMinTemp = jvToday["temperature"]["min"];//最低気温のプロパティを抽出
    if (!jvMaxTemp.isNull()) gMaxTemp = jvMaxTemp["celsius"].as<String>();//最高気温文字列を取得しグローバル変数を更新
    if (!jvMinTemp.isNull()) gMinTemp = jvMinTemp["celsius"].as<String>();//最低気温文字列を取得しグローバル変数を更新
    
    jsonDoc.clear();//JSONオブジェクトの領域をメモリから解放
  }
  http.end();
}

byte decodeStr2Weather(String str) { //文字列を天気定数に変換
  if (str.indexOf("¥u96e8") >= 0)return RAINY;//"雨"の字が含まれるなら
  else if (str.indexOf("¥u66c7") >= 0)return CLOUDY;//"曇"の字が含まれるなら
  else if (str.indexOf("¥u6674") >= 0)return SUNNY;//"晴"の字が含まれるなら
  else return RAINY;//上記の字が含まれないならフェイルセーフとして雨を返す
}

void drawWeather() { //gAMWeather,gPMWeatherの値に基づき画面に天気を描画
  M5.Lcd.fillScreen(0x3186);//画面を黒で塗りつぶし
  M5.Lcd.setTextDatum(0);//文字描画の際の原点を左上に設定
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE, 0x3186);//文字色を白、背景色を黒に設定
  M5.Lcd.drawString(gDate, 4, 2);//日付文字列を描画
  M5.Lcd.drawBitmap(8, 12, 64, 64, w_icon[gAMWeather]);//AMの天気アイコンを描画
  M5.Lcd.drawBitmap(88, 12, 64, 64, w_icon[gPMWeather]);//PMの天気アイコンを描画
}

void drawTemperature() {//最高気温、最低気温を表示
  M5.Lcd.fillScreen(0x3186);//画面を黒で塗りつぶし
  M5.Lcd.setTextDatum(0);//文字描画の際の原点を左上に設定
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE, 0x3186);//文字色を白、背景色を黒に設定
  M5.Lcd.drawString("temperature", 4, 2);//文字列を描画

  M5.Lcd.setTextDatum(4);//文字描画の際の原点を文字の中央に設定
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(BLUE, 0x3186);//文字色を青、背景色を黒に設定
  M5.Lcd.drawString(gMinTemp, 40, 42);
  M5.Lcd.setTextColor(RED, 0x3186);//文字色を青、背景色を黒に設定
  M5.Lcd.drawString(gMaxTemp, 120, 42);
}

void setup() {
  M5.begin();//M5StickCオブジェクトを初期化
  M5.Axp.ScreenBreath(8);//画面輝度を設定(7~15)
  M5.Lcd.setRotation(3);//画面を横向きに

  M5.Lcd.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(MY_SSID, MY_PASSWORD);//WiFiへの接続開始
  while (WiFi.status() != WL_CONNECTED) {
    M5.Lcd.print(".");
    delay(250);//接続待ち遅延
  }
  M5.Lcd.println("Connected!");
  delay(1500);

  gLoopCounter = 0;//ループカウンタを初期化
  gDaySelector = 0;
  gMaxTemp = "--";//最高気温
  gMinTemp = "--";//最低気温
}

void loop() {
  M5.update();//ボタン状態を更新
  if (M5.BtnA.wasPressed()){//ボタンを押して離したら
    gDaySelector = ++gDaySelector%3;//取得日を今日明日明後日とループ
    gLoopCounter = 0;
  }
  
  //初回ループまたは指定した周期ごとに天気情報を更新
  if (gLoopCounter % (1000/LOOP_DELAY * UPDATE_INTERVAL) == 0)
    updateWeather();//Webから天気を取得
  
  //天気と気温を表示(交互)
  if (gLoopCounter % (1000/LOOP_DELAY * 5) == 0 ) //5[s]のうち3[s]間は天気表示が残る
    drawWeather();//天気を描画
  else if (gLoopCounter % (1000/LOOP_DELAY * 5) == 1000/LOOP_DELAY * 3 ) //5[s]のうち2[s]間は気温表示が残る
    drawTemperature();//気温描画

  //画面中央の▲をLOOP_DELAY*2[ms]周期で点滅表示
  if (gLoopCounter % 2 == 0)
    M5.Lcd.fillTriangle(74, 40 - 16, 74, 40 + 16, 86, 40, 0x1DE4); //緑色
  else
    M5.Lcd.fillTriangle(74, 40 - 16, 74, 40 + 16, 86, 40, 0x3186); //黒色

  gLoopCounter++;//符号なしなのでオーバーフローしてもok
  delay(LOOP_DELAY);
}
