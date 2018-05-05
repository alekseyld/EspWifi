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
var ids = ['light', 'pump', 'compressor', 'oilspill'];
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
    <td>Подсветка</td>
      <td><button id="light" type="button" onclick="buttonclick(this);">Включить</button></td>
      <td id="light_c" class="off_td">Отключено</td>
  </tr>
  <tr>
      <td>Насос</td>
    <td><button id="pump" type="button" onclick="buttonclick(this);">Включить</button></td>
      <td id="pump_c" class="off_td">Отключено</td>
  </tr>
  <tr>
      <td>Компрессор</td>
    <td><button id="compressor" type="button" onclick="buttonclick(this);">Включить</button></td>
      <td id="compressor_c" class="off_td">Отключено</td>
  </tr>
  <tr>
      <td>Качалка</td>
    <td><button id="oilspill" type="button" onclick="buttonclick(this);">Включить</button></td>
      <td id="oilspill_c" class="off_td">Отключено</td>
  </tr>
</table>
</body>
</html>
)rawliteral";
