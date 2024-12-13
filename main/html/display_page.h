static const char display_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>ESP32 Battery & LED Control</title>
    <link rel='stylesheet' type='text/css' href='/styles.css'>
</head>

<body class='page-bg'>
    <!-- Container for the whole page -->
    <div class='container'>
        <!-- Battery Information Section -->
        <div class='section'>
            <h1 class='section-title'>Battery Information</h1>
            <div class='info'>
                <div class='info-item'>
                    <span>State of charge:</span>
                    <span><span id='charge'></span> %</span>
                </div>
                <div class='info-item'>
                    <span>Voltage:</span>
                    <span><span id='voltage'></span> V</span>
                </div>
                <div class='info-item'>
                    <span>Current:</span>
                    <span><span id='current'></span> A</span>
                </div>
                <div class='info-item'>
                    <span>Temperature:</span>
                    <span><span id='temperature'></span> °C</span>
                </div>
            </div>
        </div>

        <!-- ESP32 LED Control Section -->
        <div class='section'>
            <h2 class='section-title'>ESP32 LED Control</h2>
            <div class='button-container'>
                <button class='btn-toggle' onclick="fetch('/toggle').then(() => alert('LED Toggled'))">
                    Toggle LED
                </button>
            </div>
        </div>

        <!-- Links Section -->
        <div class='section'>
            <div class='links'>
                <img src='/image/aceon2.png' alt='Company Logo' class='logo'>
                <div class='link-list'>
                    <a href='/change' class='link'>Change battery pack properties</a>
                    <a href='/connect' class='link'>Connect to WiFi access point</a>
                    <a href='/nearby' class='link'>Nearby battery packs</a>
                    <a href='/about' class='link'>About AceOn</a>
                    <a href='/device' class='link'>Unit Layout</a>
                </div>
            </div>
        </div>
    </div>
</body>

<script>
      document.addEventListener('DOMContentLoaded', function() {
      let socket = new WebSocket('ws://' + location.host + '/ws');
      socket.onopen = function() {
          console.log('WebSocket connection established');
      };
      socket.onmessage = function(event) {
          try {
              let data = JSON.parse(event.data);
              document.getElementById('charge').innerHTML = data.charge;
              document.getElementById('voltage').innerHTML = data.voltage.toFixed(2);
              document.getElementById('current').innerHTML = data.current.toFixed(2);
              document.getElementById('temperature').innerHTML = data.temperature.toFixed(2);
          } catch (error) {
              console.error('Error parsing JSON:', error);
          }
      };
      socket.onerror = function(error) {
          console.error('WebSocket error:', error);
      };
      socket.onclose = function() {
          console.log('WebSocket connection closed');
      };
    })
</script>
</html>
)rawliteral";
