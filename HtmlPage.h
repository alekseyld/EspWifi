// Web страница панели управления
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html lang="ru-RU">
<head>
<meta charset="UTF-8" name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
<title>Панель управления</title>
<style>
body { background-color: #fff; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }
.circle{float: left;width: 20px;height: 20px;-webkit-border-radius: 10px;-moz-border-radius: 10px;border-radius: 25px;background: red;
}
button{height:50px; width:60%;}
td{text-align: center;}
.off_td{color: white; background: #FF5252;}
.on_td{color: white; background: green;}
</style>
<script>
var ids = ['pump', 'snakefan', 'vapofan', 'meshalka', 'light'];
var temps = ['temp1'];
var height = ['height'];
var websock;
function start() {
  websock = new WebSocket('ws://' + window.location.hostname + ':81/');
  websock.onopen = function(evt) { console.log('websock open'); };
  websock.onclose = function(evt) { console.log('websock close'); };
  websock.onerror = function(evt) { console.log(evt); };
  websock.onmessage = function(evt) {
  var data = evt.data.split(' ');
  
    if (data[1] === 'on' || data[1] === 'off') {
      var e = document.getElementById(ids[data[0]] + '_c');
      if (data[1] === 'on') {
        e.className = "on_td";
        e.textContent = "Включено";
      } else {
        e.className = "off_td";
        e.textContent = "Отключено";
    }
    } else if (Number.isInteger(parseInt(data[1]))) {
      var e = document.getElementById(temps[data[0]] + '_c');
      e.textContent = data[1];
    } else {
      var e = document.getElementById(height[data[0]] + '_c');
      e.textContent = data[1];
    }
  };
}
function buttonclick(e) {
  for (var i = 0; i < ids.length; i++) {
    if (ids[i] === e.id) {
      websock.send(i);
      return;
    }
  }  
}
</script>
</head>
<body onload="javascript:start();">
<h1>Панель управления</h1>
<table border="1" width="100%" cellpadding="5">
  <caption><b>Релейное управление</b></caption>
  <tr>
  <th>Название узла</th>
    <th>Переключатель</th>
    <th>Состояние</th>
  </tr>
  <tr>
    <td>Насос</td>
      <td><button id="pump" type="button" onclick="buttonclick(this);">Включить</button></td>
      <td id="pump_c" class="off_td">Отключено</td>
  </tr>
  <tr>
      <td>Вентилятор змеевик</td>
    <td><button id="snakefan" type="button" onclick="buttonclick(this);">Включить</button></td>
      <td id="snakefan_c" class="off_td">Отключено</td>
  </tr>
  <tr>
      <td>Вентилятор испаритель</td>
    <td><button id="vapofan" type="button" onclick="buttonclick(this);">Включить</button></td>
      <td id="vapofan_c" class="off_td">Отключено</td>
  </tr>
  <tr>
      <td>Мешалка</td>
    <td><button id="meshalka" type="button" onclick="buttonclick(this);">Включить</button></td>
      <td id="meshalka_c" class="off_td">Отключено</td>
  </tr>
  <tr>
    <td>Подсветка</td>
      <td><button id="light" type="button" onclick="buttonclick(this);">Включить</button></td>
      <td id="light_c" class="off_td">Отключено</td>
  </tr>
</table>
<br>
<table border="1" width="50%" cellpadding="5">
  <caption><b>Данные датчиков</b></caption>
  <tr>
    <th>Название узла</th>
    <th>Значение</th>
  </tr>
  <tr>
      <td>Температура 1</td>
      <td id="temp1_c">25 С</td>
  </tr>
  <tr>
      <td>Датчик уровня 1</td>
      <td id="height_c">Достигнут</td>
  </tr>
</table>
</body>
</html>
)rawliteral";