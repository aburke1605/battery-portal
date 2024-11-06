static const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>ESP32 Webpage</title></head>
<body><h1>Welcome to the ESP32 Web Server!</h1></body>
<p>time: <span id='time'></span></p>
<script>
  Date.prototype.timeNow = function () {
    return ((this.getHours() < 10)?'0':'') + this.getHours() +':'+ ((this.getMinutes() < 10)?'0':'') + this.getMinutes() +':'+ ((this.getSeconds() < 10)?'0':'') + this.getSeconds();
  }
  var time = new Date().timeNow() + ' (LastSync)';
  document.getElementById('time').innerHTML = time;
</script>
</html>)rawliteral";