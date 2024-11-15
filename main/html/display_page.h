/*
static const char display_html[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>ESP32 Webpage</title></head>

<body><h1>Welcome to the ESP32 Web Server!</h1></body>
<p>time: <span id='time'></span></p>

<body>
  <div class='container'>
    <form action='/page2' method='GET'>
      <label>Username : </label>
      <input type='text' placeholder='Enter Username' name='username' required>
      <button type='submit'>Go</button>
    </form>
  </div>
</body>

<script>
  const socket = new WebSocket('ws://' + window.location.hostname + '/ws');
  socket.onopen = function() {
    console.log('WebSocket connection established');
    // You can send messages or request data here if needed
  };
  socket.onmessage = function(event) {
    console.log('Message from server: ' + event.data);
    // Process received data here
  };
  Date.prototype.timeNow = function () {
    return ((this.getHours() < 10)?'0':'') + this.getHours() +':'+ ((this.getMinutes() < 10)?'0':'') + this.getMinutes() +':'+ ((this.getSeconds() < 10)?'0':'') + this.getSeconds();
  }
  var time = new Date().timeNow() + ' (LastSync)';
  document.getElementById('time').innerHTML = time;
</script>
</html>)rawliteral";
*/

static const char display_html[] = R"rawliteral(
<!DOCTYPE html>
<html>

<head><title>Time Display</title></head>
<body>
  <h1>Live Time</h1>
  <div id='time'>Connecting...</div>
  <script>
    let socket = new WebSocket('ws://' + location.host + '/ws');
    socket.onopen = function() {
        console.log('WebSocket connection established');
    };
    socket.onmessage = function(event) {
        document.getElementById('time').textContent = event.data;
    };
    socket.onerror = function(error) {
        console.error('WebSocket error:', error);
    };
    socket.onclose = function() {
        console.log('WebSocket connection closed');
    };
  </script>
</body>

</html>
)rawliteral";